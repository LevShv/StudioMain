
#include "engine.h"
#include "JuceHeader.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_processors/format_types/juce_VST3PluginFormat.h>
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

    // Создаем треки с корректной семантикой перемещения
    tracks.reserve(100);
    for (int i = 0; i < 4; i++) {
        Track track;
        track.isMidiTrack = false;
        ClipBase clip;
        tracks.emplace_back(std::move(track));
        juce::File file("C:\\Users\\llvvv\\source\\repos\\Studio\\StudioMain\\Misc\\Step5.wav");
        loadAudioClip(i, file, i, 1);
    }

    Track track;
    ClipBase clip;
    tracks.emplace_back(std::move(track));
    track.isMidiTrack = true;

 //   addPluginToTrack(4, "C:\\Users\\llvvv\\source\\repos\\Studio\\Plugins\\TAL-Sampler.vst3");
    
    juce::MidiMessageSequence sequence;

    // Добавляем ноту C4 (нота включения + нота выключения)
    sequence.addEvent(juce::MidiMessage::noteOn(1, 61, 0.8f), 0.0);  // Нота включена на канале 1, нота 60 (C4), velocity 0.8
    sequence.addEvent(juce::MidiMessage::noteOff(1, 61), 1.0);        // Нота выключена через 1 такт

    // Добавляем ноту E4
    sequence.addEvent(juce::MidiMessage::noteOn(1, 64, 0.7f), 1.0);
    sequence.addEvent(juce::MidiMessage::noteOff(1, 64), 2.0);

    // Добавляем ноту G4
    sequence.addEvent(juce::MidiMessage::noteOn(1, 67, 0.9f), 2.0);
    sequence.addEvent(juce::MidiMessage::noteOff(1, 67), 3.0);
	loadMidiClip(4, sequence, 5);
    

    Track track2;
    ClipBase clip2;
    tracks.emplace_back(std::move(track2));
    track2.isMidiTrack = true;

    juce::MidiMessageSequence sequence2;

    // Добавляем ноту C4 (нота включения + нота выключения)
    sequence2.addEvent(juce::MidiMessage::noteOn(1, 63, 0.8f), 0.0);  // Нота включена на канале 1, нота 60 (C4), velocity 0.8
    sequence2.addEvent(juce::MidiMessage::noteOff(1, 63), 1.0);        // Нота выключена через 1 такт

    // Добавляем ноту E4
    sequence2.addEvent(juce::MidiMessage::noteOn(1, 64, 0.7f), 1.0);
    sequence2.addEvent(juce::MidiMessage::noteOff(1, 64), 2.0);

    // Добавляем ноту G4
    sequence2.addEvent(juce::MidiMessage::noteOn(1, 62, 0.9f), 2.0);
    sequence2.addEvent(juce::MidiMessage::noteOff(1, 62), 3.0);
    loadMidiClip(5, sequence2, 0.0);
    addPluginToTrack(5, "C:\\Users\\llvvv\\source\\repos\\Studio\\Plugins\\Just a Sample.vst3");
    addPluginToTrack(4, "C:\\Users\\llvvv\\source\\repos\\Studio\\Plugins\\Just a Sample.vst3");
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

//void Engine::Core::getNextAudioBlock(const juce::AudioSourceChannelInfo& info) {
//    const juce::ScopedLock sl(lock);
//
//    if (!transportPlaying) {
//        info.clearActiveBufferRegion();
//        return;
//    }
//
//    const double blockDuration = info.numSamples / sampleRate;
//    const double startTime = position;
//    const double endTime = startTime + blockDuration;
//    const double startBeats = positionInBeats;
//    const double blockDurationBeats = secondsToBeats(blockDuration);
//    const double endBeats = startBeats + blockDurationBeats;
//
//    // Очистка буфера
//    info.clearActiveBufferRegion();
//
//    // Буфер для MIDI-сообщений
//    juce::MidiBuffer midiBuffer;
//
//    // Обработка аудио и MIDI клипов
//    for (auto& active : activeClips) {
//        if (auto* audioClip = dynamic_cast<const AudioClip*>(active.clip)) {
//            if (!audioClip->muted && !active.track->muted) {
//                if (audioClip->useRAM) {
//                    const int startSample = static_cast<int>((startTime - audioClip->startTime) * sampleRate);
//                    const int numSamples = juce::jmin(
//                        info.numSamples,
//                        audioClip->buffer.getNumSamples() - startSample
//                    );
//
//                    if (startSample >= 0 && numSamples > 0 && startSample < audioClip->buffer.getNumSamples()) {
//                        for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
//                            info.buffer->addFrom(
//                                channel, info.startSample, audioClip->buffer,
//                                channel % audioClip->buffer.getNumChannels(),
//                                startSample, numSamples,
//                                active.track->gain * audioClip->gain
//                            );
//                        }
//                    }
//                }
//                else if (active.source != nullptr) {
//                    juce::AudioSourceChannelInfo tempInfo(info.buffer, info.startSample, info.numSamples);
//                    active.source->getNextAudioBlock(tempInfo);
//                    for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
//                        info.buffer->applyGain(channel, info.startSample, info.numSamples, active.track->gain * audioClip->gain);
//                    }
//                }
//            }
//        }
//        else if (auto* midiClip = dynamic_cast<const MidiClip*>(active.clip)) {
//            for (const auto& event : midiClip->midiSequence) {
//                double eventTime = midiClip->startTime + event->message.getTimeStamp();
//                const double epsilon = 0.001; // 1 мс
//                LOG("Checking MIDI event: Note " << event->message.getNoteNumber() << ", eventTime " << eventTime << ", startTime " << startTime << ", endTime " << endTime);
//                if (eventTime >= startTime - epsilon && eventTime < endTime) {
//                    int sampleOffset = static_cast<int>((eventTime - startTime) * sampleRate);
//                    if (sampleOffset < 0) {
//                      //  LOG_WARN("Negative sampleOffset: " << sampleOffset << ", adjusting to 0");
//                        sampleOffset = 0;
//                    }
//                    midiBuffer.addEvent(event->message, sampleOffset);
//                   // LOG("MIDI event added: Note " << event->message.getNoteNumber() << " at time " << eventTime << ", sampleOffset " << sampleOffset);
//                }
//                else {
//                    //LOG("MIDI event skipped: Note " << event->message.getNoteNumber() << ", eventTime " << eventTime << " outside range [" << startTime - epsilon << ", " << endTime << ")");
//                }
//            }
//        }
//    }
//
//    // Обработка плагинов на дорожках
//    for (auto& track : tracks) {
//        if (track.muted || track.plugins.empty()) continue;
//
//        // Подготавливаем временный буфер для плагина
//        pluginBuffer.setSize(info.buffer->getNumChannels(), info.numSamples);
//        pluginBuffer.clear();
//
//        for (auto& pluginInstance : track.plugins) {
//            if (!pluginInstance->bypass && pluginInstance->plugin) {
//                // Передаем MIDI в плагин
//                pluginInstance->plugin->processBlock(pluginBuffer, midiBuffer);
//
//                // Микшируем выход плагина в основной буфер
//                for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
//                    info.buffer->addFrom(
//                        channel, info.startSample, pluginBuffer,
//                        channel % pluginBuffer.getNumChannels(),
//                        0, info.numSamples, track.gain
//                    );
//                }
//            }
//        }
//    }
//
//    // Отправка MIDI на внешние устройства
//    if (midiOutput && !midiBuffer.isEmpty()) {
//        for (const auto& metadata : midiBuffer) {
//            midiOutput->sendMessageNow(metadata.getMessage());
//        }
//    }
//
//    // Обновление позиции
//    position += blockDuration;
//    positionInBeats = secondsToBeats(position);
//    updateActiveClips();
//}

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

    // Очистка буфера
    info.clearActiveBufferRegion();

    for (auto& track : tracks) {
        // Обрабатываем аудиоклипы
        for (auto& active : activeClips) {
            if (active.track != &track) continue; // Пропускаем клипы, не принадлежащие текущему треку
            if (auto* audioClip = dynamic_cast<const AudioClip*>(active.clip)) {
                if (!audioClip->muted && !active.track->muted) {
                    if (audioClip->useRAM) {
                        const int startSample = static_cast<int>((startTime - audioClip->startTime) * sampleRate);
                        const int numSamples = juce::jmin(
                            info.numSamples,
                            audioClip->buffer.getNumSamples() - startSample
                        );

                        if (startSample >= 0 && numSamples > 0 && startSample < audioClip->buffer.getNumSamples()) {
                            for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
                                info.buffer->addFrom(
                                    channel, info.startSample, audioClip->buffer,
                                    channel % audioClip->buffer.getNumChannels(),
                                    startSample, numSamples,
                                    active.track->gain * audioClip->gain
                                );
                            }
                        }
                    }
                    else if (active.source != nullptr) {
                        juce::AudioSourceChannelInfo tempInfo(info.buffer, info.startSample, info.numSamples);
                        active.source->getNextAudioBlock(tempInfo);
                        for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
                            info.buffer->applyGain(channel, info.startSample, info.numSamples, active.track->gain * audioClip->gain);
                        }
                    }
                }
            }
        }

        // Собираем MIDI-сообщения только для текущего трека
        juce::MidiBuffer midiBuffer;
        for (auto& active : activeClips) {
            if (active.track != &track) continue; // Пропускаем клипы, не принадлежащие текущему треку
            if (auto* midiClip = dynamic_cast<const MidiClip*>(active.clip)) {
                for (const auto& event : midiClip->midiSequence) {
                    double eventTime = midiClip->startTime + event->message.getTimeStamp();
                    const double epsilon = 0.001;
                  //  LOG("Checking MIDI event: Note " << event->message.getNoteNumber() << ", eventTime " << eventTime << ", startTime " << startTime << ", endTime " << endTime);
                    if (eventTime >= startTime - epsilon && eventTime < endTime) {
                        int sampleOffset = static_cast<int>((eventTime - startTime) * sampleRate);
                        if (sampleOffset < 0) {
                          //  LOG_WARN("Negative sampleOffset: " << sampleOffset << ", adjusting to 0");
                            sampleOffset = 0;
                        }
                        midiBuffer.addEvent(event->message, sampleOffset);
                        //LOG("MIDI event added: Note " << event->message.getNoteNumber() << " at time " << eventTime << ", sampleOffset " << sampleOffset);
                    }
                    else {
                        //LOG("MIDI event skipped: Note " << event->message.getNoteNumber() << ", eventTime " << eventTime << " outside range [" << startTime - epsilon << ", " << endTime << ")");
                    }
                }
            }
        }

        // Обработка плагинов на дорожке
        if (track.muted || track.plugins.empty()) continue;

        pluginBuffer.setSize(info.buffer->getNumChannels(), info.numSamples);
        pluginBuffer.clear();

        for (auto& pluginInstance : track.plugins) {
            if (!pluginInstance->bypass && pluginInstance->plugin) {
                pluginInstance->plugin->processBlock(pluginBuffer, midiBuffer);
                for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
                    info.buffer->addFrom(
                        channel, info.startSample, pluginBuffer,
                        channel % pluginBuffer.getNumChannels(),
                        0, info.numSamples, track.gain
                    );
                }
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

void Engine::Core::loadAudioClip(int trackIndex, const juce::File& file,
    double startBeats, bool loadToRAM) {
    if (trackIndex < 0 || trackIndex >= tracks.size() || tracks[trackIndex].isMidiTrack) {
        LOG_ERROR("Invalid track index or MIDI track");
        return;
    }

    auto newClip = std::make_unique<AudioClip>();
    newClip->file = file;
    newClip->startBeats = startBeats;
    newClip->startTime = beatsToSeconds(startBeats); // Переводим биты в секунды //// начало бит
    newClip->useRAM = loadToRAM;

    if (auto reader = formatManager.createReaderFor(file)) {
        newClip->duration = reader->lengthInSamples / reader->sampleRate;
        newClip->durationBeats = secondsToBeats(newClip->duration); /// задали длину в битах

        if (loadToRAM) {
            newClip->buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&newClip->buffer, 0, (int)reader->lengthInSamples, 0, true, true);
        }
    }

    tracks.at(trackIndex).clips.push_back(std::move(newClip));
}

void Engine::Core::loadMidiClip(int trackIndex, const juce::MidiMessageSequence& sequence,
    double startBeats) {

    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Invalid track index");
        return;
    }
    if (!tracks[trackIndex].isMidiTrack) {
        if (tracks[trackIndex].clips.size() == 0) {
            tracks[trackIndex].isMidiTrack = true;
        } 
        else {
            LOG_ERROR("This track is not a MIDI track!");
            return;
        }
    }

    auto newClip = std::make_unique<MidiClip>();
    newClip->midiSequence = sequence;
    newClip->startTime = beatsToSeconds(startBeats); // Переводим биты в секунды
    newClip->startBeats = startBeats;
    LOG("StartBeat for new clip set: " << startBeats);
    LOG("StartTime for new clip set: " << newClip->startTime);

 // Устанавливаем опорный BPM ///

    double endTime = 0;
    for (int i = 0; i < sequence.getNumEvents(); i++) {
        auto event = sequence.getEventPointer(i);
        endTime = juce::jmax(endTime, event->message.getTimeStamp());
    }
    newClip->duration = endTime + 0.1;
    newClip->durationBeats = secondsToBeats(newClip->duration); ///

    tracks.at(trackIndex).clips.push_back(std::move(newClip));
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

                if (auto* audioClip = dynamic_cast<AudioClip*>(clip.get())) {
                    if (!audioClip->useRAM) {
                        if (auto reader = formatManager.createReaderFor(audioClip->file)) {
                            auto readerPtr = std::unique_ptr<juce::AudioFormatReader>(reader);
                            active.source = std::make_unique<juce::AudioFormatReaderSource>(
                                readerPtr.release(), true);
                            active.source->prepareToPlay(512, sampleRate);
                            juce::int64 readPosition = static_cast<juce::int64>((position - audioClip->startTime) * sampleRate);
                            active.source->setNextReadPosition(readPosition);
                            LOG("Added non-RAM audio clip at startTime: " << audioClip->startTime << ", duration: " << audioClip->duration);
                        }
                        else {
                            LOG_ERROR("Failed to create reader for file: " << audioClip->file.getFullPathName().toStdString());
                        }
                    }
                    else {
                        LOG("Added RAM audio clip at startTime: " << audioClip->startTime << ", duration: " << audioClip->duration);
                    }
                }
                else if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
                    LOG("Added MIDI clip at startTime: " << midiClip->startTime << ", duration: " << midiClip->duration);
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

void Engine::AddMidiClip(int trackInd, const juce::MidiMessageSequence& sequence, double startBeats) {
    core.loadMidiClip(trackInd, sequence, startBeats);
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
    return core.position;  
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
#pragma endregion