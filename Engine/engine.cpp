
#include "engine.h"
#include "JuceHeader.h"
#include <thread>
#include <mutex>
#include <algorithm>
// Core implementation

#pragma region Core

Engine::Core::Core() {
    formatManager.registerBasicFormats();
    pluginFormatManager.addDefaultFormats();

    LOG("Available plugin formats:");
    for (auto* format : pluginFormatManager.getFormats()) {
        LOG(format->getName().toStdString());
    }

    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    if (!midiOutputs.isEmpty()) {
        midiOutput = juce::MidiOutput::openDevice(midiOutputs[0].identifier);
    }

    // Резервируем место для треков
    tracks.reserve(6);

    // Создаем 4 аудиотрека (индексы 0–3)
    for (int i = 0; i < 4; i++) {
        Track track;
        track.isMidiTrack = false;
        tracks.emplace_back(std::move(track));
        juce::File file("C:\\Users\\llvvv\\source\\repos\\Studio\\StudioMain\\Misc\\Step5.wav");
        loadAudioClip(i, file, 4.0, true); // Аудиоклип с началом в 4 бита
        LOG_SUCCESS("Added audio track " << i << " with audio clip at startBeats=4.0");
    }

    // Создаем MIDI-трек 4 (индекс 4)
    {
        Track track;
        track.isMidiTrack = true;
        tracks.emplace_back(std::move(track));

        juce::MidiMessageSequence sequence;
        sequence.addEvent(juce::MidiMessage::noteOn(1, 61, 0.8f), 0.0);  // C4
        sequence.addEvent(juce::MidiMessage::noteOff(1, 61), 1.0);
        sequence.addEvent(juce::MidiMessage::noteOn(1, 64, 0.7f), 1.0);  // E4
        sequence.addEvent(juce::MidiMessage::noteOff(1, 64), 2.0);
        sequence.addEvent(juce::MidiMessage::noteOn(1, 67, 0.9f), 2.0);  // G4
        sequence.addEvent(juce::MidiMessage::noteOff(1, 67), 3.0);
        loadMidiClip(4, sequence, 5.0); // MIDI-клип с началом в 5 битов
        LOG_SUCCESS("Added MIDI track 4 with MIDI clip at startBeats=5.0");
    }

    // Создаем MIDI-трек 5 (индекс 5)
    {
        Track track;
        track.isMidiTrack = true;
        tracks.emplace_back(std::move(track));

        // Первый MIDI-клип (clipIndex=0)
        juce::MidiMessageSequence sequence;
        sequence.addEvent(juce::MidiMessage::noteOn(1, 63, 0.8f), 0.0);  // D4
        sequence.addEvent(juce::MidiMessage::noteOff(1, 63), 1.0);
        sequence.addEvent(juce::MidiMessage::noteOn(1, 64, 0.7f), 1.0);  // E4
        sequence.addEvent(juce::MidiMessage::noteOff(1, 64), 2.0);
        sequence.addEvent(juce::MidiMessage::noteOn(1, 62, 0.9f), 2.0);  // D4
        sequence.addEvent(juce::MidiMessage::noteOff(1, 62), 3.0);
        loadMidiClip(5, sequence, 0.0); // MIDI-клип с началом в 0 битов
        LOG_SUCCESS("Added MIDI track 5 with MIDI clip at startBeats=0.0");

        // Второй MIDI-клип (clipIndex=1, пустой)
        juce::MidiMessageSequence emptySequence;
        loadMidiClip(5, emptySequence, 8.0); // Пустой клип с началом в 8 битов
        LOG_SUCCESS("Added empty MIDI clip to track 5 at startBeats=8.0, clipIndex=1");
    }

    // Добавляем плагины к MIDI-трекам
    addPluginToTrack(4, "C:\\Users\\llvvv\\source\\repos\\Studio\\Plugins\\Just a Sample.vst3");
    addPluginToTrack(5, "C:\\Users\\llvvv\\source\\repos\\Studio\\Plugins\\Just a Sample.vst3");

    audioSourcePlayer.setSource(this);
}

Engine::Core::~Core() {
    stop();
    audioSourcePlayer.setSource(nullptr);
    midiOutput.reset();
}

void Engine::Core::startAudio(juce::AudioDeviceManager& deviceManager) {
    LOG("Starting audio...");
    deviceManager.addAudioCallback(&audioSourcePlayer);
}

void Engine::Core::stopAudio(juce::AudioDeviceManager& deviceManager) {
    deviceManager.removeAudioCallback(&audioSourcePlayer);
}

void Engine::Core::prepareToPlay(int samplesPerBlock, double newSampleRate) {

    sampleRate = newSampleRate;
    transportPlaying = false;

    LOG("prepareToPlay called with sampleRate: " << newSampleRate);

    pluginBuffer.setSize(2, samplesPerBlock); // Стерео по умолчанию

    for (auto& track : tracks) {
        if (track.isMidiTrack) continue;

        for (auto& clip : track.clips) {
            if (auto* audioClip = dynamic_cast<AudioClip*>(clip.get())) {
                if (audioClip->useRAM) {
                    loadClipToRAM(*audioClip);
                }
            }
        }

        // Подготовка плагинов
        for (auto& pluginInstance : track.plugins) {
            if (pluginInstance->plugin) {
                pluginInstance->plugin->prepareToPlay(newSampleRate, samplesPerBlock);
            }
        }
    }
}

void Engine::Core::releaseResources() {
    activeClips.clear();
}

void Engine::Core::getNextAudioBlock(const juce::AudioSourceChannelInfo& info) {
    const juce::ScopedLock sl(lock);

    if (!transportPlaying) {
        info.clearActiveBufferRegion();
        return;
    }

    const double blockDuration = info.numSamples / sampleRate;
    const double startTime = position;
    const double endTime = startTime + blockDuration;
    const double startBeats = positionInBeats;
    const double blockDurationBeats = secondsToBeats(blockDuration);
    const double endBeats = startBeats + blockDurationBeats;

    info.clearActiveBufferRegion();

    for (auto& track : tracks) {
        pluginBuffer.setSize(info.buffer->getNumChannels(), info.numSamples);
        pluginBuffer.clear();
        bool hasAudio = false;

        for (auto& active : activeClips) {
            if (active.track != &track) continue;

            const ClipBase* clipToProcess = active.clip;
            if (auto* cloneClip = dynamic_cast<const CloneClip*>(active.clip)) {
                clipToProcess = cloneClip->masterClip; // Используем мастер-клип для данных
            }

            if (auto* audioClip = dynamic_cast<const AudioClip*>(clipToProcess)) {
                if (!audioClip->muted && !active.track->muted) {
                    if (audioClip->useRAM) {
                        const int startSample = static_cast<int>((startTime - active.clip->startTime) * sampleRate);
                        const int numSamples = juce::jmin(
                            info.numSamples,
                            audioClip->buffer.getNumSamples() - startSample
                        );
                        if (startSample >= 0 && numSamples > 0 && startSample < audioClip->buffer.getNumSamples()) {
                            for (int channel = 0; channel < pluginBuffer.getNumChannels(); ++channel) {
                                pluginBuffer.addFrom(
                                    channel, 0, audioClip->buffer,
                                    channel % audioClip->buffer.getNumChannels(),
                                    startSample, numSamples,
                                    active.track->gain * audioClip->gain
                                );
                            }
                            hasAudio = true;
                        }
                    }
                    else if (active.source != nullptr) {
                        juce::AudioSourceChannelInfo tempInfo(&pluginBuffer, 0, info.numSamples);
                        active.source->getNextAudioBlock(tempInfo);
                        for (int channel = 0; channel < pluginBuffer.getNumChannels(); ++channel) {
                            pluginBuffer.applyGain(channel, 0, info.numSamples, active.track->gain * audioClip->gain);
                        }
                        hasAudio = true;
                    }
                }
            }
        }

        juce::MidiBuffer midiBuffer;
        for (auto& active : activeClips) {
            if (active.track != &track) continue;

            const ClipBase* clipToProcess = active.clip;
            if (auto* cloneClip = dynamic_cast<const CloneClip*>(active.clip)) {
                clipToProcess = cloneClip->masterClip;
            }

            if (auto* midiClip = dynamic_cast<const MidiClip*>(clipToProcess)) {
                for (const auto& event : midiClip->midiSequence) {
                    double eventTime = active.clip->startTime + event->message.getTimeStamp();
                    const double epsilon = 0.01;
                    if (eventTime >= startTime - epsilon && eventTime < endTime) {
                        int sampleOffset = static_cast<int>((eventTime - startTime) * sampleRate);
                        if (sampleOffset < 0) {
                            sampleOffset = 0;
                        }
                        midiBuffer.addEvent(event->message, sampleOffset);
                    }
                }
            }
        }

        if (!track.muted && (hasAudio || !midiBuffer.isEmpty() || !track.plugins.empty())) {
            juce::AudioBuffer<float> processedBuffer = pluginBuffer;
            for (auto& pluginInstance : track.plugins) {
                if (!pluginInstance->bypass && pluginInstance->plugin) {
                    pluginInstance->plugin->processBlock(processedBuffer, midiBuffer);
                }
            }
            for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
                info.buffer->addFrom(
                    channel, info.startSample, processedBuffer,
                    channel % processedBuffer.getNumChannels(),
                    0, info.numSamples, track.gain
                );
            }
        }
    }

    position += blockDuration;
    positionInBeats = secondsToBeats(position);
    updateActiveClips();
}

void Engine::Core::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) {
    // Пример обработки входящих сообщений
    if (message.isNoteOn()) {
        LOG("MIDI Note On: " << message.getNoteNumber());
    }
}

void Engine::Core::addCloneClip(int trackIndex, int masterClipIndex, double startBeats) {
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Invalid track index: " << trackIndex);
        return;
    }
    if (masterClipIndex < 0 || masterClipIndex >= tracks[trackIndex].clips.size()) {
        LOG_ERROR("Invalid master clip index: " << masterClipIndex);
        return;
    }

    // Получаем указатель на клип
    ClipBase* clip = tracks[trackIndex].clips[masterClipIndex].get();
    if (!clip) {
        LOG_ERROR("Null clip at trackIndex=" << trackIndex << ", masterClipIndex=" << masterClipIndex);
        return;
    }

    // Проверяем, является ли клип клоном
    ClipBase* masterClip = clip;
    if (auto* cloneClip = dynamic_cast<CloneClip*>(clip)) {
        masterClip = cloneClip->masterClip;
        LOG("Requested clone from clone clip with clipID=" << clip->clipID
            << ", using its master clip with clipID=" << masterClip->clipID);
    }

    // Проверяем, что мастер-клип валиден
    if (!masterClip) {
        LOG_ERROR("Invalid master clip for clone creation at trackIndex=" << trackIndex
            << ", masterClipIndex=" << masterClipIndex);
        return;
    }

    // Создаем новый клон
    auto newCloneClip = std::make_unique<CloneClip>(masterClip, startBeats);
    newCloneClip->startTime = beatsToSeconds(startBeats);
    newCloneClip->clipID = newCloneClip->generateClipID();
    newCloneClip->masterClipID = masterClip->clipID;

    // Логируем до перемещения
    LOG("Added clone clip with clipID=" << newCloneClip->clipID
        << ", startBeats=" << startBeats << ", masterClipID=" << masterClip->clipID);

    // Добавляем клон в трек
    tracks[trackIndex].clips.push_back(std::move(newCloneClip));

    updateActiveClips();
}

void Engine::Core::RenderToFile(std::string& outputPath) {
    LOG("Starting render to file: " << outputPath);

    // 1. Определяем максимальную длительность проекта
    double projectDuration = 0.0;
    for (const auto& track : tracks) {
        for (const auto& clip : track.clips) {
            double clipEndTime = clip->startTime + clip->duration;
            projectDuration = juce::jmax(projectDuration, clipEndTime);
        }
    }

    if (projectDuration <= 0.0) {
        LOG_ERROR("Project duration is 0, nothing to render!");
        return;
    }

    LOG("Project duration: " << projectDuration << " seconds");

    // 2. Настраиваем параметры рендера
    const int samplesPerBlock = 512; // Размер блока для рендера
    const double renderSampleRate = sampleRate > 0 ? sampleRate : 44100.0;
    const int numChannels = 2; // Стерео
    const int totalSamples = static_cast<int>(projectDuration * renderSampleRate);

    // 3. Создаём WAV-файл
    juce::File outputFile(outputPath);
    if (outputFile.existsAsFile()) {
        outputFile.deleteFile();
    }

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset(wavFormat.createWriterFor(
        new juce::FileOutputStream(outputFile),
        renderSampleRate,
        numChannels,
        16, // 16-битный WAV
        {}, // Метаданные (пустые)
        0   // Качество (для WAV не используется)
    ));

    if (!writer) {
        LOG_ERROR("Failed to create WAV writer for file: " << outputPath);
        return;
    }

    // 4. Подготавливаем буферы
    juce::AudioBuffer<float> renderBuffer(numChannels, samplesPerBlock);
    juce::AudioSourceChannelInfo bufferInfo(&renderBuffer, 0, samplesPerBlock);
    juce::MidiBuffer midiBuffer;

    // 5. Сбрасываем позицию воспроизведения
    double originalPosition = position;
    double originalPositionInBeats = positionInBeats;
    position = 0.0;
    positionInBeats = 0.0;
    updateActiveClips();

    // 6. Рендерим
    int samplesRendered = 0;
    while (samplesRendered < totalSamples) {
        int samplesThisBlock = juce::jmin(samplesPerBlock, totalSamples - samplesRendered);
        bufferInfo.numSamples = samplesThisBlock;

        // Очистка буфера перед обработкой
        renderBuffer.clear();

        const double blockDuration = samplesThisBlock / renderSampleRate;
        const double startTime = position;
        const double endTime = startTime + blockDuration;
        const double startBeats = positionInBeats;
        const double blockDurationBeats = secondsToBeats(blockDuration);
        const double endBeats = startBeats + blockDurationBeats;

        // Обрабатываем каждый трек
        for (auto& track : tracks) {
            // Обрабатываем аудиоклипы
            for (auto& active : activeClips) {
                if (active.track != &track) continue;
                if (auto* audioClip = dynamic_cast<const AudioClip*>(active.clip)) {
                    if (!audioClip->muted && !active.track->muted) {
                        if (audioClip->useRAM) {
                            const int startSample = static_cast<int>((startTime - audioClip->startTime) * renderSampleRate);
                            const int numSamples = juce::jmin(
                                samplesThisBlock,
                                audioClip->buffer.getNumSamples() - startSample
                            );

                            if (startSample >= 0 && numSamples > 0 && startSample < audioClip->buffer.getNumSamples()) {
                                for (int channel = 0; channel < numChannels; ++channel) {
                                    renderBuffer.addFrom(
                                        channel, 0, audioClip->buffer,
                                        channel % audioClip->buffer.getNumChannels(),
                                        startSample, numSamples,
                                        active.track->gain * audioClip->gain
                                    );
                                }
                            }
                        }
                        else if (active.source != nullptr) {
                            juce::AudioSourceChannelInfo tempInfo(&renderBuffer, 0, samplesThisBlock);
                            active.source->getNextAudioBlock(tempInfo);
                            for (int channel = 0; channel < numChannels; ++channel) {
                                renderBuffer.applyGain(channel, 0, samplesThisBlock, active.track->gain * audioClip->gain);
                            }
                        }
                    }
                }
            }

            // Собираем MIDI-сообщения для текущего трека
            midiBuffer.clear();
            for (auto& active : activeClips) {
                if (active.track != &track) continue;
                if (auto* midiClip = dynamic_cast<const MidiClip*>(active.clip)) {
                    for (const auto& event : midiClip->midiSequence) {
                        double eventTime = midiClip->startTime + event->message.getTimeStamp();
                        const double epsilon = 0.01;
                        if (eventTime >= startTime - epsilon && eventTime < endTime) {
                            int sampleOffset = static_cast<int>((eventTime - startTime) * renderSampleRate);
                            if (sampleOffset < 0) {
                                sampleOffset = 0;
                            }
                            midiBuffer.addEvent(event->message, sampleOffset);
                        }
                    }
                }
            }

            if (track.muted || track.plugins.empty()) continue;

            pluginBuffer.setSize(numChannels, samplesThisBlock);
            pluginBuffer.clear();

            for (auto& pluginInstance : track.plugins) {
                if (!pluginInstance->bypass && pluginInstance->plugin) {
                    pluginInstance->plugin->processBlock(pluginBuffer, midiBuffer);
                    for (int channel = 0; channel < numChannels; ++channel) {
                        pluginBuffer.addFrom(
                            channel, 0, renderBuffer,
                            channel, 0, samplesThisBlock, 1.0f
                        );
                        renderBuffer.copyFrom(
                            channel, 0, pluginBuffer,
                            channel, 0, samplesThisBlock
                        );
                    }
                }
            }
        }

        writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesThisBlock);

        position += blockDuration;
        positionInBeats = secondsToBeats(position);
        updateActiveClips();
        samplesRendered += samplesThisBlock;
    }

    writer->flush();
    writer.reset();

    position = originalPosition;
    positionInBeats = originalPositionInBeats;
    updateActiveClips();

    LOG_SUCCESS("Render completed successfully to: " << outputPath);
}

void Engine::Core::processMidiBlocks(const juce::AudioSourceChannelInfo& info,
    double startTime, double endTime) {
    juce::MidiBuffer midiBuffer;

    for (auto& active : activeClips) {
        if (auto* midiClip = dynamic_cast<const MidiClip*>(active.clip)) {
            for (const auto& event : midiClip->midiSequence) {
                double eventTime = midiClip->startTime + event->message.getTimeStamp();

                if (eventTime >= startTime && eventTime < endTime) {
                    int sampleOffset = static_cast<int>((eventTime - startTime) * sampleRate);
                    midiBuffer.addEvent(event->message, sampleOffset);
                    LOG("MIDI event added: Note " << event->message.getNoteNumber() << " at time " << eventTime);
                }
            }
        }
    }
    LOG("MIDI buffer events: " << midiBuffer.getNumEvents());
    if (midiOutput && !midiBuffer.isEmpty()) {
        // Современный способ итерации по MidiBuffer
        for (const auto& metadata : midiBuffer) {
            midiOutput->sendMessageNow(metadata.getMessage());
        }
    }
}

void Engine::Core::play() {
    const juce::ScopedLock sl(lock);
    transportPlaying = true;
    LOG("Playback STARTED at position: " << position << " seconds (" << positionInBeats << " beats)");
    updateActiveClips();
}

void Engine::Core::stop() {

    const juce::ScopedLock sl(lock);
    transportPlaying = false;
    activeClips.clear();

    if (midiOutput) {
        for (int channel = 1; channel <= 16; ++channel) {
            midiOutput->sendMessageNow(juce::MidiMessage::allNotesOff(channel));
        }
    }
    LOG("Playback STOPPED at position: " << position << " seconds (" << positionInBeats << " beats)");

}

void Engine::Core::setPosition(double newPosition) {
    const juce::ScopedLock sl(lock);
    position = newPosition;
    positionInBeats = secondsToBeats(newPosition);
    updateActiveClips();
    {
        const juce::ScopedLock noteSl(noteLock);
        activeNotes.clear(); // Сбрасываем все активные ноты при премотке
    }

    juce::MidiBuffer clearBuffer;
    for (int channel = 1; channel <= 16; ++channel) {
        clearBuffer.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
    }

    for (auto& track : tracks) {
        for (auto& pluginInstance : track.plugins) {
            if (pluginInstance->plugin) {
                juce::AudioBuffer<float> tempBuffer(2, 512); // Временный буфер
                tempBuffer.clear();
                pluginInstance->plugin->processBlock(tempBuffer, clearBuffer);
            }
        }
    }
    LOG("Playhead moved to: " << position << " seconds, all notes off sent");
}

void Engine::Core::loadAudioClip(int trackIndex, const juce::File& file, double startBeats, bool loadToRAM) {
    if (trackIndex < 0 || trackIndex >= tracks.size()) return;

    auto clip = std::make_unique<AudioClip>();
    clip->file = file;
    clip->startBeats = startBeats;
    clip->startTime = beatsToSeconds(startBeats);
    clip->clipID = clip->generateClipID(); // Уникальный ID

    juce::AudioFormatReader* reader = formatManager.createReaderFor(file);
    if (reader) {
        clip->duration = reader->lengthInSamples / reader->sampleRate;
        clip->durationBeats = secondsToBeats(clip->duration);

        if (loadToRAM) {
            clip->buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&clip->buffer, 0, (int)reader->lengthInSamples, 0, true, true);
            clip->useRAM = true;

            std::thread([clip = clip.get()]() {
                int numSamples = clip->buffer.getNumSamples();
                int numChannels = clip->buffer.getNumChannels();

                const int minSamplesPerPoint = 100;
                const int maxSamplesPerPoint = 1000;
                const int maxSampleCount = 10000;
                int sampleCount = numSamples / minSamplesPerPoint;
                sampleCount = std::max(1, std::min(sampleCount, maxSampleCount));
                if (numSamples / sampleCount > maxSamplesPerPoint) {
                    sampleCount = numSamples / maxSamplesPerPoint;
                }

                int step = numSamples / sampleCount;
                if (step < 1) step = 1;

                std::vector<float> waveformData(sampleCount);
                for (int i = 0; i < sampleCount && i * step < numSamples; ++i) {
                    float maxAmplitude = 0.0f;
                    for (int j = 0; j < step; ++j) {
                        int sampleIdx = i * step + j;
                        float amplitude = 0.0f;
                        for (int c = 0; c < numChannels; ++c) {
                            if (sampleIdx < numSamples) {
                                amplitude += std::abs(clip->buffer.getSample(c, sampleIdx));
                            }
                        }
                        amplitude /= numChannels;
                        maxAmplitude = std::max(maxAmplitude, amplitude);
                    }
                    waveformData[i] = maxAmplitude;
                }

                juce::CriticalSection lock;
                const juce::ScopedLock sl(lock);
                clip->waveformData = std::move(waveformData);
                }).detach();
        }
        delete reader;
    }

    tracks[trackIndex].clips.push_back(std::move(clip));
}

void Engine::Core::loadMidiClip(int trackIndex, const juce::MidiMessageSequence& sequence, double startBeats) {
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Invalid track index: " << trackIndex);
        return;
    }
    if (!tracks[trackIndex].isMidiTrack && !tracks[trackIndex].clips.empty()) {
        LOG_ERROR("This track is not a MIDI track and contains clips!");
        return;
    }

    auto newClip = std::make_unique<MidiClip>();
    newClip->midiSequence = sequence;
    newClip->startTime = beatsToSeconds(startBeats);
    newClip->startBeats = startBeats;
    newClip->clipID = newClip->generateClipID();
    LOG("StartBeat for new clip set: " << startBeats);
    LOG("StartTime for new clip set: " << newClip->startTime);

    double endTime = 0.0;
    for (int i = 0; i < sequence.getNumEvents(); i++) {
        auto event = sequence.getEventPointer(i);
        endTime = juce::jmax(endTime, event->message.getTimeStamp());
    }

    if (endTime == 0.0) {
        newClip->durationBeats = 4.0; // Пустой клип: 4 бита
        newClip->duration = beatsToSeconds(4.0);
    }
    else {
        newClip->duration = endTime + 0.1;
        newClip->durationBeats = secondsToBeats(newClip->duration);
    }

    tracks.at(trackIndex).clips.push_back(std::move(newClip));
    updateActiveClips();
  //  LOG_SUCCESS("Loaded MIDI clip: trackIndex=" << trackIndex << ", startBeats=" << startBeats << ", durationBeats=" << newClip->durationBeats);
}

void Engine::Core::addPluginToTrack(int trackIndex, const juce::String& pluginPath) {
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Invalid track index: " << trackIndex);
        return;
    }

    juce::File pluginFile(pluginPath);
    if (!pluginFile.existsAsFile()) {
        LOG_ERROR("Plugin file does not exist or is not accessible: " << pluginPath.toStdString());
        return;
    }

    if (!pluginPath.endsWithIgnoreCase(".vst3")) {
        LOG_ERROR("Only VST3 plugins are supported: " << pluginPath.toStdString());
        return;
    }

    juce::AudioPluginFormat* vst3Format = nullptr;
    for (auto* format : pluginFormatManager.getFormats()) {
        if (format->getName() == "VST3") {
            vst3Format = format;
            break;
        }
    }

    if (!vst3Format) {
        LOG_ERROR("VST3 format not found in pluginFormatManager!");
        return;
    }

    juce::OwnedArray<juce::PluginDescription> typesFound;
    pluginList.scanAndAddFile(pluginPath, false, typesFound, *vst3Format);
    LOG("Found " << typesFound.size() << " plugins in " << pluginPath.toStdString());

    if (typesFound.isEmpty()) {
        LOG_ERROR("No plugins found in file: " << pluginPath.toStdString());
        return;
    }

    juce::PluginDescription desc = *typesFound[0];
    juce::String error;
    std::unique_ptr<juce::AudioPluginInstance> plugin = pluginFormatManager.createPluginInstance(
        desc, sampleRate, 512, error
    );

    if (!plugin) {
        LOG_ERROR("Failed to load plugin: " << error.toStdString());
        return;
    }

    plugin->enableAllBuses();
    plugin->prepareToPlay(sampleRate, 512);

    auto pluginInstance = std::make_unique<PluginInstance>();
    pluginInstance->plugin = std::move(plugin);
    pluginInstance->Path = pluginPath.toStdString();
    tracks[trackIndex].plugins.push_back(std::move(pluginInstance));

    LOG_SUCCESS("Plugin loaded successfully: " << pluginPath.toStdString() << ", Instance address: " << (void*)tracks[trackIndex].plugins.back()->plugin.get());
}

juce::AudioProcessorEditor* Engine::Core::getPluginEditor(int trackIndex, int pluginIndex) {
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        pluginIndex < 0 || pluginIndex >= tracks[trackIndex].plugins.size()) {
        LOG_ERROR("Invalid track or plugin index");
        return nullptr;
    }

    auto& pluginInstance = tracks[trackIndex].plugins[pluginIndex];
    if (pluginInstance->plugin && !pluginInstance->editor) {
        pluginInstance->editor = pluginInstance->plugin->createEditorIfNeeded();
        LOG("Editor created for plugin at address: " << (void*)pluginInstance->plugin.get() << ", Editor address: " << (void*)pluginInstance->editor);
    }
    return pluginInstance->editor;
}

void Engine::Core::togglePluginBypass(int trackIndex, int pluginIndex) {
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        pluginIndex < 0 || pluginIndex >= tracks[trackIndex].plugins.size()) {
        LOG_ERROR("Invalid track or plugin index");
        return;
    }
    tracks[trackIndex].plugins[pluginIndex]->bypass = !tracks[trackIndex].plugins[pluginIndex]->bypass;
}

void Engine::Core::removePluginFromTrack(int trackIndex, int pluginIndex) {
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        pluginIndex < 0 || pluginIndex >= tracks[trackIndex].plugins.size()) {
        LOG_ERROR("Invalid track or plugin index");
        return;
    }
    tracks[trackIndex].plugins.erase(tracks[trackIndex].plugins.begin() + pluginIndex);
}

void Engine::Core::moveClip(int trackIndex, int clipIndex, double startBeats) {
    if (trackIndex >= 0 && trackIndex < tracks.size() &&
        clipIndex >= 0 && clipIndex < tracks.at(trackIndex).clips.size()) {
        const juce::ScopedLock sl(lock);

        juce::MidiBuffer clearBuffer;
        for (int channel = 1; channel <= 16; ++channel) {
            clearBuffer.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
        }

        for (auto& pluginInstance : tracks[trackIndex].plugins) {
            if (pluginInstance->plugin) {
                juce::AudioBuffer<float> tempBuffer(2, 512); // Временный буфер
                tempBuffer.clear();
                pluginInstance->plugin->processBlock(tempBuffer, clearBuffer);
            }
        }


        tracks.at(trackIndex).clips[clipIndex]->startTime = beatsToSeconds(startBeats);
        tracks.at(trackIndex).clips[clipIndex]->startBeats = startBeats;
        updateActiveClips();
    }
}

void Engine::Core::changeMidiclipDuration(int trackIndex, int clipIndex, double newDurationBeats) {
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        clipIndex < 0 || clipIndex >= tracks[trackIndex].clips.size()) {
        LOG_ERROR("Invalid track or clip index: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
        return;
    }

    auto& clip = tracks[trackIndex].clips[clipIndex];
    if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
        double minDurationBeats = midiClip->minDurationBeats; // Обычно 1 бит
        if (newDurationBeats < minDurationBeats) {
            LOG_WARN("Requested duration " << newDurationBeats << " is less than minimum " << minDurationBeats << ". Using minimum.");
            newDurationBeats = minDurationBeats;
        }

        midiClip->durationBeats = newDurationBeats;
        midiClip->duration = beatsToSeconds(newDurationBeats);
        updateActiveClips();
        LOG_SUCCESS("Changed MIDI clip duration: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex
            << ", newDurationBeats=" << newDurationBeats << ", newDurationSeconds=" << midiClip->duration);
    }
    else {
        LOG_ERROR("Clip is not a MIDI clip");
    }
}

void Engine::Core::changeAudioclipDuration(int trackIndex, int clipIndex, double newDurationBeats)
{
    auto& clip = tracks[trackIndex].clips[clipIndex];
    auto* audioClip = dynamic_cast<AudioClip*>(clip.get());
    if (!audioClip) {
        LOG_ERROR("Clip at trackIndex=" << trackIndex << ", clipIndex=" << clipIndex << " is not an audio clip");
        return;
    }

    // Загружаем аудиоданные, если они еще не в памяти
    if (!audioClip->useRAM) {
        loadClipToRAM(*audioClip);
        audioClip->useRAM = true;
    }

    double newDurationSeconds = beatsToSeconds(newDurationBeats);

    if (newDurationBeats <= 0.0) {
        LOG_ERROR("Invalid new duration: " << newDurationBeats << " beats");
        return;
    }

    // Исходная длительность аудиоклипа
    double originalDuration = audioClip->buffer.getNumSamples() / sampleRate;

    int newSampleCount = static_cast<int>(newDurationSeconds * sampleRate);
    int originalSampleCount = audioClip->buffer.getNumSamples();
    int numChannels = audioClip->buffer.getNumChannels();

    juce::AudioBuffer<float> newBuffer(numChannels, newSampleCount);

    if (newDurationSeconds > originalDuration) {
        // Зацикливание аудио
        int samplesToCopy = originalSampleCount;
        int targetSample = 0;
        while (targetSample < newSampleCount) {
            int samplesThisLoop = juce::jmin(samplesToCopy, newSampleCount - targetSample);
            for (int channel = 0; channel < numChannels; ++channel) {
                newBuffer.copyFrom(channel, targetSample, audioClip->buffer, channel, 0, samplesThisLoop);
            }
            targetSample += samplesThisLoop;
            // Если нужно продолжить зацикливание, начинаем сначала
            if (targetSample < newSampleCount) {
                samplesToCopy = juce::jmin(originalSampleCount, newSampleCount - targetSample);
            }
        }
        LOG("Audio clip looped to new duration: " << newDurationSeconds << " seconds (" << newDurationBeats << " beats)");
    }
    else {
        // Обрезка аудио
        for (int channel = 0; channel < numChannels; ++channel) {
            newBuffer.copyFrom(channel, 0, audioClip->buffer, channel, 0, newSampleCount);
        }
        LOG("Audio clip truncated to new duration: " << newDurationSeconds << " seconds (" << newDurationBeats << " beats)");
    }

    audioClip->buffer = std::move(newBuffer);
    audioClip->duration = newDurationSeconds;
    audioClip->durationBeats = newDurationBeats;

    // Пересчитываем данные формы волны
    int sampleCount = newSampleCount / 100; // Примерное количество точек
    sampleCount = juce::jmin(sampleCount, 10000); // Ограничение на максимальное количество точек
    int step = sampleCount > 0 ? newSampleCount / sampleCount : 1;

    std::vector<float> waveformData(sampleCount);
    for (int i = 0; i < sampleCount && i * step < newSampleCount; ++i) {
        float maxAmplitude = 0.0f;
        for (int j = 0; j < step; ++j) {
            int sampleIdx = i * step + j;
            float amplitude = 0.0f;
            for (int c = 0; c < numChannels; ++c) {
                if (sampleIdx < newSampleCount) {
                    amplitude += std::abs(audioClip->buffer.getSample(c, sampleIdx));
                }
            }
            amplitude /= numChannels;
            maxAmplitude = std::max(maxAmplitude, amplitude);
        }
        waveformData[i] = maxAmplitude;
    }

    audioClip->waveformData = std::move(waveformData);
    LOG("Waveform data recalculated for audio clip, new sample count: " << newSampleCount);
}

void Engine::Core::updateActiveClips() {
    activeClips.clear();
    LOG("Updating active clips at position: " << position << " seconds (" << positionInBeats << " beats)");

    for (auto& track : tracks) {
        if (track.muted) continue;

        for (auto& clip : track.clips) {
            if (clip->isActive(position)) {
                ActiveClip active;
                active.clip = clip.get();
                active.track = &track;

                if (auto* cloneClip = dynamic_cast<CloneClip*>(clip.get())) {
                    // Если это клон, используем источник данных из мастер-клипа
                    if (auto* audioClip = dynamic_cast<AudioClip*>(cloneClip->masterClip)) {
                        if (!audioClip->useRAM) {
                            if (auto reader = formatManager.createReaderFor(audioClip->file)) {
                                auto readerPtr = std::unique_ptr<juce::AudioFormatReader>(reader);
                                active.source = std::make_unique<juce::AudioFormatReaderSource>(
                                    readerPtr.release(), true);
                                active.source->prepareToPlay(512, sampleRate);
                                juce::int64 readPosition = static_cast<juce::int64>((position - clip->startTime) * sampleRate);
                                active.source->setNextReadPosition(readPosition);
                                LOG("Added non-RAM clone audio clip at startTime: " << clip->startTime);
                            }
                            else {
                                LOG_ERROR("Failed to create reader for file: " << audioClip->file.getFullPathName().toStdString());
                            }
                        }
                    }
                }
                else if (auto* audioClip = dynamic_cast<AudioClip*>(clip.get())) {
                    if (!audioClip->useRAM) {
                        if (auto reader = formatManager.createReaderFor(audioClip->file)) {
                            auto readerPtr = std::unique_ptr<juce::AudioFormatReader>(reader);
                            active.source = std::make_unique<juce::AudioFormatReaderSource>(
                                readerPtr.release(), true);
                            active.source->prepareToPlay(512, sampleRate);
                            juce::int64 readPosition = static_cast<juce::int64>((position - audioClip->startTime) * sampleRate);
                            active.source->setNextReadPosition(readPosition);
                            LOG("Added non-RAM audio clip at startTime: " << audioClip->startTime);
                        }
                        else {
                            LOG_ERROR("Failed to create reader for file: " << audioClip->file.getFullPathName().toStdString());
                        }
                    }
                }
                else if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
                    LOG("Added MIDI clip at startTime: " << midiClip->startTime);
                }
                activeClips.add(std::move(active));
            }
        }
    }
    LOG("Total active clips: " << activeClips.size());
}

void Engine::Core::loadClipToRAM(AudioClip& clip) {
    if (auto reader = formatManager.createReaderFor(clip.file)) {
        clip.buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&clip.buffer, 0, (int)reader->lengthInSamples, 0, true, true);
    }
}

void Engine::Core::addMidiNote(int trackIndex, int clipIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        clipIndex < 0 || clipIndex >= tracks[trackIndex].clips.size()) {
        LOG_ERROR("Invalid track or clip index: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
        return;
    }
    if (noteNumber < 0 || noteNumber > 127 || startBeats < 0 || durationBeats <= 0 || velocity < 0 || velocity > 1.0f || channel < 1 || channel > 16) {
        LOG_ERROR("Invalid note parameters: noteNumber=" << noteNumber << ", startBeats=" << startBeats
            << ", durationBeats=" << durationBeats << ", velocity=" << velocity << ", channel=" << channel);
        return;
    }

    auto& clip = tracks[trackIndex].clips[clipIndex];
    if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
        double startTimeSeconds = beatsToSeconds(startBeats);
        double endTimeSeconds = beatsToSeconds(startBeats + durationBeats);

        midiClip->midiSequence.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, velocity), startTimeSeconds);
        midiClip->midiSequence.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), endTimeSeconds);

        // Обновляем длительность клипа
        midiClip->durationBeats = juce::jmax(midiClip->durationBeats, startBeats + durationBeats);
        midiClip->duration = beatsToSeconds(midiClip->durationBeats);

        updateActiveClips();
        LOG("Added MIDI note: noteNumber=" << noteNumber << ", startBeats=" << startBeats
            << ", durationBeats=" << durationBeats << ", velocity=" << velocity
            << ", sequence size=" << midiClip->midiSequence.getNumEvents());
    }
    else {
        LOG_ERROR("Clip is not a MIDI clip");
    }
}
void Engine::Core::deleteMidiNote(int trackIndex, int clipIndex, int noteIndex) {
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        clipIndex < 0 || clipIndex >= tracks[trackIndex].clips.size()) {
        LOG_ERROR("Invalid track or clip index: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
        return;
    }

    auto& clip = tracks[trackIndex].clips[clipIndex];
    if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
        std::vector<std::pair<double, juce::MidiMessage>> noteEvents;
        std::map<std::pair<int, int>, double> noteOnTimes; // (channel, noteNumber) -> startTime

        // Собираем все события
        for (const auto& event : midiClip->midiSequence) {
            noteEvents.emplace_back(event->message.getTimeStamp(), event->message);
            if (event->message.isNoteOn()) {
                noteOnTimes[{event->message.getChannel(), event->message.getNoteNumber()}] = event->message.getTimeStamp();
            }
        }

        if (noteIndex * 2 >= noteEvents.size()) {
            LOG_ERROR("Invalid note index: " << noteIndex);
            return;
        }

        // Удаляем Note On и Note Off для указанной ноты
        juce::MidiMessageSequence newSequence;
        int currentNoteIndex = -1;
        std::map<std::pair<int, int>, double> activeNotes;
        for (const auto& eventPair : noteEvents) {
            const auto& msg = eventPair.second;
            if (msg.isNoteOn()) {
                auto key = std::make_pair(msg.getChannel(), msg.getNoteNumber());
                activeNotes[key] = msg.getTimeStamp();
            }
            else if (msg.isNoteOff()) {
                auto key = std::make_pair(msg.getChannel(), msg.getNoteNumber());
                if (activeNotes.find(key) != activeNotes.end()) {
                    currentNoteIndex++;
                    if (currentNoteIndex != noteIndex) {
                        newSequence.addEvent(juce::MidiMessage::noteOn(key.first, key.second, noteEvents[currentNoteIndex].second.getVelocity() / 127.0f), activeNotes[key]);
                        newSequence.addEvent(msg, msg.getTimeStamp());
                    }
                    activeNotes.erase(key);
                }
            }
            else {
                newSequence.addEvent(msg, eventPair.first);
            }
        }

        midiClip->midiSequence = newSequence;

        // Пересчитываем длительность клипа
        double maxEndTime = 0.0;
        for (const auto& event : midiClip->midiSequence) {
            maxEndTime = juce::jmax(maxEndTime, event->message.getTimeStamp());
        }
        midiClip->duration = maxEndTime + 0.1;
        midiClip->durationBeats = secondsToBeats(midiClip->duration);

        updateActiveClips();
        LOG("Deleted MIDI note at index: " << noteIndex);
    }
    else {
        LOG_ERROR("Clip is not a MIDI clip");
    }
}

void Engine::Core::updateMidiNote(int trackIndex, int clipIndex, int noteIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        clipIndex < 0 || clipIndex >= tracks[trackIndex].clips.size()) {
        LOG_ERROR("Invalid track or clip index");
        return;
    }
    if (noteNumber < 0 || noteNumber > 127 || startBeats < 0 || durationBeats <= 0 || velocity < 0 || velocity > 1.0f || channel < 1 || channel > 16) {
        LOG_ERROR("Invalid note parameters");
        return;
    }

    auto& clip = tracks[trackIndex].clips[clipIndex];
    if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
        // Удаляем старую ноту
        deleteMidiNote(trackIndex, clipIndex, noteIndex);
        // Добавляем новую
        double startTimeSeconds = beatsToSeconds(startBeats);
        double endTimeSeconds = beatsToSeconds(startBeats + durationBeats);
        midiClip->midiSequence.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, velocity), startTimeSeconds);
        midiClip->midiSequence.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), endTimeSeconds);

        // Обновляем длительность клипа
        double maxEndTime = 0.0;
        for (const auto& event : midiClip->midiSequence) {
            maxEndTime = juce::jmax(maxEndTime, event->message.getTimeStamp());
        }
        midiClip->duration = maxEndTime + 0.1;
        midiClip->durationBeats = secondsToBeats(midiClip->duration);

        updateActiveClips();
        LOG("Updated MIDI note at index: " << noteIndex);
    }
}

void Engine::Core::setBPM(double newBPM) {
    if (newBPM > 0.0) {
        bpm = newBPM;
        LOG("BPM updated to: " << bpm);
        updateActiveClips(); // Пересчитываем активные клипы, если нужно
    }
    else {
        LOG_ERROR("Invalid BPM value: " << newBPM);
    }
}

void Engine::Core::setTimeSignature(int numerator, int denominator) {
    if (numerator > 0 && denominator > 0 && (denominator & (denominator - 1)) == 0) { // Проверка, что знаменатель - степень двойки
        timeSignatureNumerator = numerator;
        timeSignatureDenominator = denominator;
        LOG("Time signature updated to: " << numerator << "/" << denominator);
        updateActiveClips(); // Пересчитываем активные клипы, если нужно
    }
    else {
        LOG_ERROR("Invalid time signature: " << numerator << "/" << denominator);
    }
}

double Engine::Core::secondsToBeats(double seconds) const {
    return seconds * (bpm / 60.0);
}

double Engine::Core::beatsToSeconds(double beats) const {
    return beats * (60.0 / bpm);
}

double Engine::Core::secondsToMeasures(double seconds) const {
    double beats = secondsToBeats(seconds);
    return beats / timeSignatureNumerator;
}

double Engine::Core::measuresToSeconds(double measures) const {
    double beats = measures * timeSignatureNumerator;
    return beatsToSeconds(beats);
}

#pragma endregion

// Engine implementation

#pragma region Engine

Engine::Engine() {
    // 1. Инициализация аудиосистемы (явно указываем типы)
    audioSourcePlayer.setSource(&core);
    deviceManager.addAudioCallback(&audioSourcePlayer);

    for (auto deviceType : deviceManager.getAvailableDeviceTypes()) {
        for (auto device : deviceType->getDeviceNames()) {
            LOG(" - " << device);
        }
    }

    // 2. Получение списка устройств (без auto)
    juce::StringArray outputDevices;
    juce::AudioIODeviceType* deviceType = deviceManager.getCurrentDeviceTypeObject();

    if (deviceType != nullptr) {
        deviceType->scanForDevices(); // Явно обновляем список
        outputDevices = deviceType->getDeviceNames(false); // false для output
    }

    // 3. Вывод в консоль с явными типами
    std::cout << "\n=== Devices: ===\n";
    if (outputDevices.isEmpty()) {
        std::cout << "Devices did not found!\n";
    }
    else {
        for (int i = 0; i < outputDevices.size(); ++i) {
            std::cout << "[" << i << "] " << outputDevices[i].toStdString() << "\n";
        }

        // 4. Ввод с проверкой (явные типы)
        int selectedIndex = 0;
        std::cout << "\nchoose device: ";

        std::string input;
        std::getline(std::cin, input);

        if (!input.empty()) {
            try {
                selectedIndex = std::stoi(input);
                if (selectedIndex < 0 || selectedIndex >= outputDevices.size()) {
                    throw std::out_of_range("Err");
                }
            }
            catch (...) {
                std::cout << "Bad input.\n";
                selectedIndex = 0;
            }
        }

        // 5. Установка устройства (явный тип)
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.outputDeviceName = outputDevices[selectedIndex];

        const juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
        if (error.isNotEmpty()) {
            std::cerr << "Ошибка выбора устройства: " << error.toStdString() << "\n";
        }
        else {
            std::cout << "Успешно выбрано: " << outputDevices[selectedIndex].toStdString() << "\n";
        }
    }

    // 6. Инициализация с явными типами
    juce::AudioIODevice* audioDevice = deviceManager.getCurrentAudioDevice();
    if (audioDevice != nullptr) {
        const int bufferSize = audioDevice->getCurrentBufferSizeSamples();
        const double sampleRate = audioDevice->getCurrentSampleRate();
        core.prepareToPlay(bufferSize, sampleRate);
    }

    // 7. Убрали настройку MIDI, так как внешние MIDI не нужны
}

Engine::~Engine() {
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
}

void Engine::AddAudioClip(int trackInd, const std::string& path, double startBeats, bool loadToRAM) {
    juce::File audioFile(juce::String(path).replace("\\", "/").replace("//", "/"));

    if (!audioFile.existsAsFile()) {
        LOG_ERROR("File does not exist or is not accessible: " << path);
        return;
    }

    core.loadAudioClip(trackInd, audioFile, startBeats, loadToRAM);
}

bool Engine::AddMidiClip(int trackInd, double startBeats) {
    if (trackInd < 0 || trackInd >= core.tracks.size()) {
        LOG_ERROR("Invalid track index: " << trackInd);
        return false;
    }
    if (!core.tracks[trackInd].isMidiTrack && !core.tracks[trackInd].clips.empty()) {
        LOG_WARN("Track is not a MIDI track and contains clips");
        return false;
    }

    juce::MidiMessageSequence sequence; // Пустая последовательность
    core.loadMidiClip(trackInd, sequence, startBeats);
    return true;
}

void Engine::StopMix() { core.stop(); }

void Engine::PlayMix() { core.play(); }

void Engine::MoveClip(int trackIndex, int clipIndex, double startBeats) {
    core.moveClip(trackIndex, clipIndex, startBeats);
}

void Engine::SetPlayheadPosition(double position) { 
    core.setPosition(core.beatsToSeconds(position));
}

bool Engine::IsPlaying() { juce::ScopedLock sl(core.lock); return core.isPlaying(); }

void Engine::SendMidiMessage(const juce::MidiMessage& message) {
    if (core.midiOutput) {
        core.midiOutput->sendMessageNow(message);
    }
}

const std::vector<Engine::Track>& Engine::GetdataBase() const
{
    juce::ScopedLock lock(core.lock); // Защита от гонок
    return core.tracks;
}

void Engine::configureMidiDevices() {
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();

    if (midiOutputs.isEmpty()) {
        LOG_ERROR("No MIDI output devices found!");
        return;
    }

    std::cout << "\n=== Available MIDI Outputs ===\n";
    for (size_t i = 0; i < midiOutputs.size(); ++i) {
        std::cout << "[" << i << "] " << midiOutputs[i].name.toStdString() << "\n";
    }

    int selectedIndex = 0;
    std::cout << "Choose MIDI device: ";
    std::string input;
    std::getline(std::cin, input);

    try {
        selectedIndex = std::stoi(input);
        if (selectedIndex < 0 || selectedIndex >= midiOutputs.size()) {
            throw std::out_of_range("Invalid selection");
        }
    }
    catch (...) {
        std::cout << "Bad input. Defaulting to 0.\n";
        selectedIndex = 0;
    }

    // Закрываем старый выход и открываем новый
    core.midiOutput.reset();
    core.midiOutput = juce::MidiOutput::openDevice(midiOutputs[selectedIndex].identifier);

    if (core.midiOutput) {
        std::cout << "Selected MIDI Output: " << midiOutputs[selectedIndex].name.toStdString() << "\n";
    }
    else {
        std::cerr << "Failed to open MIDI Output!\n";
    }
}

double& Engine::Position() {
    return core.positionInBeats;  
}

void Engine::RenderToFile(std::string& Path)
{
    juce::ScopedLock s1(core.lock);
    core.RenderToFile(Path);
}

void Engine::SaveProject(const std::string& Path)
{
    juce::ScopedLock s1(core.lock);
    saver.SaveProject(Path);
}

bool Engine::LoadProject(const std::string& Path) {
    juce::ScopedLock sl(core.lock);
    return saver.LoadProject(Path);
}

double Engine::GetPlayheadPosition() const {
    juce::ScopedLock sl(core.lock);
    return core.positionInBeats; // Возвращаем позицию в битах
}

double Engine::GetBPM() const {
    juce::ScopedLock sl(core.lock);
    return core.bpm;
}

void Engine::SetBPM(double newBPM) {
    juce::ScopedLock sl(core.lock);
    core.setBPM(newBPM);
}

int Engine::AddAudioTrack() {
    core.tracks.emplace_back(); // Добавляем новую дорожку
    int newTrackIndex = static_cast<int>(core.tracks.size()) - 1;
    core.tracks[newTrackIndex].isMidiTrack = false; // Это аудиодорожка
    LOG("Added audio track at index " << newTrackIndex);
    return newTrackIndex;
}

int Engine::AddMidiTrack() {
    core.tracks.emplace_back(); // Добавляем новую дорожку
    int newTrackIndex = static_cast<int>(core.tracks.size()) - 1;
    core.tracks[newTrackIndex].isMidiTrack = true; // Это MIDI-дорожка
    LOG("Added MIDI track at index " << newTrackIndex);
    return newTrackIndex;
}

int Engine::AddSamplerTrack() {
    core.tracks.emplace_back(); // Добавляем новую дорожку
    int newTrackIndex = static_cast<int>(core.tracks.size()) - 1;
    core.tracks[newTrackIndex].isMidiTrack = true; // Это MIDI-дорожка
    core.tracks[newTrackIndex].isSamplerTrack = true;
    LOG("Added MIDI track at index " << newTrackIndex);
    return newTrackIndex;
}

void Engine::DeleteTrack(int trackIndex) {
    juce::ScopedLock sl(core.lock);
    if (trackIndex < core.tracks.size()) {
        core.tracks.erase(core.tracks.begin() + trackIndex);
    }
    else {
        LOG_ERROR("Index out of range");
    }
}

void Engine::DeleteClip(int trackIndex, int clipIndex) {
    juce::ScopedLock sl(core.lock);

    if (trackIndex < 0 || trackIndex >= core.tracks.size() ||
        clipIndex < 0 || clipIndex >= core.tracks[trackIndex].clips.size()) {
        LOG_ERROR("Index out of range: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
        return;
    }

    auto& track = core.tracks[trackIndex];
    auto& clipToDelete = track.clips[clipIndex];

    // Проверяем, является ли клип мастер-клипом (не клоном)
    if (dynamic_cast<Engine::CloneClip*>(clipToDelete.get())) {
        // Если это клон, просто удаляем его
        track.clips.erase(track.clips.begin() + clipIndex);
        LOG("Deleted clone clip at trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
    }
    else {
        // Это мастер-клип, ищем его клоны по clipID
        std::string masterClipID = clipToDelete->clipID;
        std::vector<int> cloneIndices;
        for (int i = 0; i < track.clips.size(); ++i) {
            if (i == clipIndex) continue; // Пропускаем сам клип
            if (auto* cloneClip = dynamic_cast<Engine::CloneClip*>(track.clips[i].get())) {
                if (cloneClip->masterClipID == masterClipID) {
                    cloneIndices.push_back(i);
                }
            }
        }

        if (!cloneIndices.empty()) {
            // Если есть клоны, переносим startTime и startBeats первого клона на мастер-клип
            int firstCloneIndex = cloneIndices[0];
            auto& firstClone = track.clips[firstCloneIndex];

            clipToDelete->startTime = firstClone->startTime;
            clipToDelete->startBeats = firstClone->startBeats;
            LOG("Transferred startTime=" << firstClone->startTime << " and startBeats=" << firstClone->startBeats
                << " from clone at index " << firstCloneIndex << " to master at index " << clipIndex
                << " (clipID: " << masterClipID << ")");

            // Удаляем первый клон
            track.clips.erase(track.clips.begin() + firstCloneIndex);
            LOG("Deleted clone clip at index " << firstCloneIndex);

            // Обновляем индексы оставшихся клонов, если они были после удаленного клона
            for (size_t i = 1; i < cloneIndices.size(); ++i) {
                int cloneIndex = cloneIndices[i];
                if (cloneIndex > firstCloneIndex) {
                    --cloneIndex; // Учитываем сдвиг после удаления первого клона
                }
                LOG("Clone at index " << cloneIndex << " continues to reference master with clipID: " << masterClipID);
            }
        }
        else {
            // Если клонов нет, удаляем мастер-клип
            track.clips.erase(track.clips.begin() + clipIndex);
            LOG("Deleted master clip at trackIndex=" << trackIndex << ", clipIndex=" << clipIndex
                << " (clipID: " << masterClipID << ") with no clones");
        }
    }

    // Обновляем активные клипы
    core.updateActiveClips();
}

void Engine::ChangeDuration(int trackIndex, int clipIndex, double newDurationBeats)
{
    juce::ScopedLock sl(core.lock);

    if (trackIndex < 0 || trackIndex >= core.tracks.size() ||
        clipIndex < 0 || clipIndex >= core.tracks[trackIndex].clips.size()) {
        LOG_ERROR("Index out of range: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
        return;
    }

    if (newDurationBeats <= 0.0) {
        LOG_ERROR("Invalid duration: " << newDurationBeats << " beats, duration must be positive");
        return;
    }

    // Преобразуем длительность из битов в секунды
    double bpm = core.bpm; // Предполагается, что bpm доступен в Core
    if (bpm <= 0.0) {
        LOG_ERROR("Invalid BPM: " << bpm);
        return;
    }
    double newDurationSeconds = core.beatsToSeconds(newDurationBeats); // 1 бит = 60/BPM секунд

    auto& track = core.tracks[trackIndex];
    auto& clip = track.clips[clipIndex];

    if (auto* cloneClip = dynamic_cast<Engine::CloneClip*>(clip.get())) {
        // Если это клонированный клип, изменяем длительность мастер-клипа
        if (cloneClip->masterClip) {
            int masterTrackIndex = -1;
            int masterClipIndex = -1;
            // Ищем мастер-клип в треках
            for (size_t t = 0; t < core.tracks.size(); ++t) {
                for (size_t c = 0; c < core.tracks[t].clips.size(); ++c) {
                    if (core.tracks[t].clips[c]->clipID == cloneClip->masterClipID) {
                        masterTrackIndex = t;
                        masterClipIndex = c;
                        break;
                    }
                }
                if (masterTrackIndex != -1) break;
            }
            if (masterTrackIndex == -1 || masterClipIndex == -1) {
                LOG_ERROR("Master clip not found for clone with masterClipID: " << cloneClip->masterClipID);
                return;
            }
            // Изменяем длительность мастер-клипа
            if (dynamic_cast<Engine::AudioClip*>(cloneClip->masterClip)) {
                core.changeAudioclipDuration(masterTrackIndex, masterClipIndex, newDurationBeats); // Передаем в битах
            }
            else if (dynamic_cast<Engine::MidiClip*>(cloneClip->masterClip)) {
                core.changeMidiclipDuration(masterTrackIndex, masterClipIndex, newDurationBeats); // Передаем в битах
            }
            // Обновляем длительность клонированного клипа
            cloneClip->duration = cloneClip->masterClip->duration;
            cloneClip->durationBeats = cloneClip->masterClip->durationBeats;
            LOG("Updated clone clip duration to " << newDurationBeats << " beats for masterClipID: " << cloneClip->masterClipID);
        }
        else {
            LOG_ERROR("Clone clip has no valid master clip");
            return;
        }
    }
    else if (auto* audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
        // Если это аудиоклип, изменяем его длительность
        core.changeAudioclipDuration(trackIndex, clipIndex, newDurationBeats);
    }
    else if (auto* midiClip = dynamic_cast<Engine::MidiClip*>(clip.get())) {
        // Если это MIDI-клип, изменяем его длительность
        core.changeMidiclipDuration(trackIndex, clipIndex, newDurationBeats);
    }

    core.updateActiveClips();
}

void Engine::AddCloneClip(int trackIndex, int masterClipIndex, double startBeats)
{
    juce::ScopedLock sl(core.lock);
    core.addCloneClip(trackIndex, masterClipIndex, startBeats);
}

void Engine::AddPluginToTrack(int trackIndex, const std::string& pluginPath) {
	juce::ScopedLock sl(core.lock);
	core.addPluginToTrack(trackIndex, juce::String(pluginPath));
}

void Engine::RemovePluginFromTrack(int trackIndex, int pluginIndex) {
	juce::ScopedLock sl(core.lock);
	core.removePluginFromTrack(trackIndex, pluginIndex);
}

void Engine::TogglePluginBypass(int trackIndex, int pluginIndex) {
	juce::ScopedLock sl(core.lock);
	core.togglePluginBypass(trackIndex, pluginIndex);
}

juce::AudioProcessorEditor* Engine::GetPluginEditor(int trackIndex, int pluginIndex) {
	juce::ScopedLock sl(core.lock);
	return core.getPluginEditor(trackIndex, pluginIndex);
}

void Engine::AddMidiNote(int trackIndex, int clipIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    juce::ScopedLock sl(core.lock);
    core.addMidiNote(trackIndex, clipIndex, noteNumber, startBeats, durationBeats, velocity, channel);
}

void Engine::DeleteMidiNote(int trackIndex, int clipIndex, int noteIndex) {
    juce::ScopedLock sl(core.lock);
    core.deleteMidiNote(trackIndex, clipIndex, noteIndex);
}

void Engine::UpdateMidiNote(int trackIndex, int clipIndex, int noteIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    juce::ScopedLock sl(core.lock);
    core.updateMidiNote(trackIndex, clipIndex, noteIndex, noteNumber, startBeats, durationBeats, velocity, channel);
}

double Engine::SecondsToBeats(double seconds) const
{
    return core.secondsToBeats(seconds);
}

double Engine::BeatsToSeconds(double beats) const
{
    return core.beatsToSeconds(beats);
}

#pragma endregion

// Saver implementation

#pragma region Saver 

void Engine::Saver::SaveProject(const std::string filePath)
{
    juce::var projectJson = juce::var(new juce::DynamicObject());

    //Глобальные настройки
	projectJson.getDynamicObject()->setProperty("bpm", m_core.bpm);
    projectJson.getDynamicObject()->setProperty("position", m_core.position);

    juce::Array<juce::var> tracksArray;

    //Дорожки
    for (const auto& track : m_core.tracks) {
        juce::DynamicObject::Ptr trackJson = new juce::DynamicObject();

        trackJson->setProperty("isMidiTrack", track.isMidiTrack);
        trackJson->setProperty("isSamplerTrack", track.isSamplerTrack);
        trackJson->setProperty("gain", track.gain);
        trackJson->setProperty("muted", track.muted);

        juce::Array<juce::var> clipsArray;

        for (const auto& clip : track.clips) {
            juce::DynamicObject::Ptr clipJson = new juce::DynamicObject();
            clipJson->setProperty("startBeats", clip->startBeats); 
            clipJson->setProperty("durationBeats", clip->durationBeats); 
            clipJson->setProperty("isMidiTrack", track.isMidiTrack);
            clipJson->setProperty("isSamplerTrack", track.isSamplerTrack);
            clipJson->setProperty("gain", track.gain);
            clipJson->setProperty("muted", track.muted);

            if (auto* audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
                clipJson->setProperty("type", "audio");
                clipJson->setProperty("filePath", juce::var(juce::String(audioClip->file.getFullPathName().toStdString())));
                clipJson->setProperty("useRAM", audioClip->useRAM);
            }
            else if (auto* midiClip = dynamic_cast<Engine::MidiClip*>(clip.get())) {
                clipJson->setProperty("type", "midi");

                juce::MidiFile midiFile;
                midiFile.addTrack(midiClip->midiSequence); 
                midiFile.setTicksPerQuarterNote(960); 

                // Записываем MIDI-данные в поток
                juce::MemoryOutputStream midiStream;
                if (!midiFile.writeTo(midiStream)) {
                    LOG_ERROR("Failed to write MIDI sequence to stream");
                    continue;
                }

                juce::String midiBase64 = juce::Base64::toBase64(midiStream.getData(), midiStream.getDataSize());
                clipJson->setProperty("midiData", midiBase64);
            }
            clipsArray.add(juce::var(clipJson));
        }

        trackJson->setProperty("clips", clipsArray);

        juce::Array<juce::var> pluginsArray;
        for (const auto& plugin : track.plugins) {
            juce::DynamicObject::Ptr pluginJson = new juce::DynamicObject();
            pluginJson->setProperty("pluginPath", plugin->plugin ? juce::var(juce::String(plugin->Path)) : "");
            LOG("Plugin path: " << plugin->Path);
            pluginJson->setProperty("bypass", plugin->bypass);

            if (plugin->plugin) {
                juce::MemoryBlock state;
                plugin->plugin->getStateInformation(state);

                if (state.getSize() > 0) {
                    plugin->state = state;

                    juce::String stateBase64 = juce::Base64::toBase64(state.getData(), state.getSize());
                    pluginJson->setProperty("state", stateBase64);
                    LOG("Saved plugin state for " << plugin->plugin->getName().toStdString() << ", size: " << state.getSize() << " bytes");
                }
                else {
                    LOG_ERROR("Failed to get plugin state for " << plugin->plugin->getName().toStdString());
                }
            }
            pluginsArray.add(juce::var(pluginJson));
        }

        trackJson->setProperty("plugins", pluginsArray);
        tracksArray.add(juce::var(trackJson));

    }

    projectJson.getDynamicObject()->setProperty("tracks", tracksArray);

    juce::File projectFile(filePath);
    if (projectFile.existsAsFile()) projectFile.deleteFile();

    juce::FileOutputStream OS(projectFile);
    if (!OS.openedOk()) {
        LOG_ERROR("Failed to open file for saving: " << filePath);
        return;
    }

    juce::JSON::writeToStream(OS, projectJson, true);
    OS.flush();

    LOG_SUCCESS("roject saved successfully to : " << filePath);

}

bool Engine::Saver::LoadProject(const std::string filePath)
{
    juce::File projectFile(filePath);
    if (!projectFile.existsAsFile()) {
        LOG_ERROR("Project file does not exist: " << filePath);
        return false;
    }

    juce::FileInputStream inputStream(projectFile);
    if (!inputStream.openedOk()) {
        LOG_ERROR("Failed to open file for loading: " << filePath);
        return false;
    }

    juce::var json = juce::JSON::parse(inputStream);
    if (json.isUndefined() || !json.isObject()) {
        LOG_ERROR("Failed to parse project JSON from file: " << filePath);
        return false;
    }

    m_core.stop();
    m_core.tracks.clear();

    if (json.hasProperty("bpm")) {
        m_core.setBPM(json["bpm"]);
        LOG("Set bpm:" << m_core.bpm);
    }
    if (json.hasProperty("position")) {
        m_core.setPosition(json["position"]);
        LOG("Set position:" << m_core.position);
    }


    if (json.hasProperty("tracks")) {
        const juce::var& tracksArray = json["tracks"];
        for (const auto& trackVar : *tracksArray.getArray()) {

            Track track;
            track.isMidiTrack = trackVar["isMidiTrack"];
            track.isSamplerTrack = trackVar["isSamplerTrack"];
            track.gain = trackVar["gain"];
            track.muted = trackVar["muted"];

            // Загружаем клипы
            if (trackVar.hasProperty("clips")) {
                for (const auto& clipVar : *trackVar["clips"].getArray()) {
                    std::string clipType = clipVar["type"].toString().toStdString();
                    double startBeats = clipVar["startBeats"];
                    double durationBeats = clipVar["durationBeats"];
                    float gain = clipVar["gain"];
                    bool muted = clipVar["muted"];

                    if (clipType == "audio") {
                        auto audioClip = std::make_unique<AudioClip>();
                        audioClip->startBeats = startBeats;
                        audioClip->durationBeats = durationBeats;
                        audioClip->gain = gain;
                        audioClip->muted = muted;
                        audioClip->file = juce::File(clipVar["filePath"].toString());
                        audioClip->useRAM = clipVar["useRAM"];
                        audioClip->startTime = m_core.beatsToSeconds(startBeats);
                        audioClip->duration = m_core.beatsToSeconds(durationBeats);

                        if (audioClip->useRAM) {
                            m_core.loadClipToRAM(*audioClip);
                        }

                        track.clips.push_back(std::move(audioClip));
                    }
                    else if (clipType == "midi") {
                        auto midiClip = std::make_unique<MidiClip>();
                        midiClip->startBeats = startBeats;
                        midiClip->durationBeats = durationBeats;
                        midiClip->gain = gain;
                        midiClip->muted = muted;
                        midiClip->startTime = m_core.beatsToSeconds(startBeats);
                        midiClip->duration = m_core.beatsToSeconds(durationBeats);

                       
                        juce::String midiBase64 = clipVar["midiData"].toString();
                        juce::MemoryOutputStream midiOutputStream;
                        if (!juce::Base64::convertFromBase64(midiOutputStream, midiBase64)) {
                            LOG_ERROR("Failed to decode Base64 MIDI data: " << midiBase64.toStdString());
                            continue;
                        }

                        juce::MemoryInputStream midiStream(midiOutputStream.getData(), midiOutputStream.getDataSize(), false);

                        juce::MidiFile midiFile;
                        if (!midiFile.readFrom(midiStream)) {
                            LOG_ERROR("Failed to read MIDI sequence from stream");
                            continue;
                        }

                        if (midiFile.getNumTracks() > 0) {
                            midiClip->midiSequence = *(midiFile.getTrack(0));
                        }
                        else {
                            LOG_ERROR("No MIDI tracks found in loaded data");
                        }

                        track.clips.emplace_back(std::move(midiClip));
                        
                        
                    }
                }
            }

            m_core.tracks.emplace_back(std::move(track));
            int currentTrackIndex = m_core.tracks.size() - 1;

            // Загружаем плагины
            if (trackVar.hasProperty("plugins")) {
                for (const auto& pluginVar : *trackVar["plugins"].getArray()) {
                    auto pluginInstance = std::make_unique<PluginInstance>();
                    std::string pluginPath = pluginVar["pluginPath"].toString().toStdString();
                    LOG("Plugin path after load: " << pluginPath);
                    if (!pluginPath.empty()) {
                        m_core.addPluginToTrack(currentTrackIndex, pluginPath);

                        auto& plugins = m_core.tracks[currentTrackIndex].plugins;
                        if (plugins.empty() || !plugins.back()->plugin) {
                            LOG_ERROR("Failed to load plugin at path: " << pluginPath);
                            continue;
                        }

                        auto* pluginInstance = plugins.back().get();

                        if (pluginVar.hasProperty("state")) {
                            juce::String stateBase64 = pluginVar["state"].toString();
                            juce::MemoryOutputStream stateStream;
                            if (juce::Base64::convertFromBase64(stateStream, stateBase64)) {
                                juce::MemoryBlock state(stateStream.getData(), stateStream.getDataSize());
                                pluginInstance->plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
                                pluginInstance->state = state;
                                LOG("Restored plugin state for " << pluginPath << ", size: " << state.getSize() << " bytes");
                            }
                            else {
                                LOG_ERROR("Failed to decode plugin state for " << pluginPath);
                            }
                        }
                    }
                    m_core.tracks[m_core.tracks.size() - 1].plugins.emplace_back(std::move(pluginInstance));
                }
            }


           // m_core.tracks.push_back(std::move(track));
            
                
            LOG("Loaded: " << m_core.tracks.size() << "tracks");
        }
    }

    m_core.updateActiveClips();
    LOG_SUCCESS("Project loaded successfully");
    return true;
}

#pragma endregion
