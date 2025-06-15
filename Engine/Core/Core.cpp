#include <log.h>
#include "engine.h"
#include "JuceHeader.h"
#include <thread>
#include <mutex>
#include <algorithm>
#include <juce_audio_formats/juce_audio_formats.h>
#include "lame.h"

#define _CRT_SECURE_NO_WARNINGS

#pragma region Core

Engine::Core::Core() {
    // Инициализация форматов
    formatManager.registerBasicFormats();
    pluginFormatManager.addDefaultFormats();

    LOG("Available plugin formats:");
    for (auto* format : pluginFormatManager.getFormats()) {
        LOG(format->getName().toStdString());
    }

    // Инициализация MIDI-выхода
    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    if (!midiOutputs.isEmpty()) {
        midiOutput = juce::MidiOutput::openDevice(midiOutputs[0].identifier);
        LOG("MIDI output initialized: " << midiOutputs[0].name.toStdString());
    }

    // Сброс параметров проекта
    position = 0.0;
    positionInBeats = 0.0;
    bpm = 120.0;
    masterGain = 1.0f; // Инициализация masterGain
    userVolume = 1.0f; // Инициализация userVolume
    loopModeEnabled = false;
    loopTrackIndex = -1;
    loopClipIndex = -1;
    loopStartTime = 0.0;
    loopDuration = 0.0;

    {
        const juce::ScopedLock noteSl(noteLock);
        activeNotes.clear();
    }
    LOG("Core parameters initialized: position=0.0, bpm=120.0, masterGain=1.0, userVolume=1.0, loopModeEnabled=false");

    // Резервируем место для треков
    tracks.reserve(6);

    // Создаем один аудиотрек по умолчанию
    Track track;
    track.isMidiTrack = false;
    tracks.emplace_back(std::move(track));
    LOG_SUCCESS("Added default audio track at index 0");

    // Подключаем audioSourcePlayer
    audioSourcePlayer.setSource(this);

    updateActiveClips();
    LOG("Core initialized successfully for new project");
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
    if (!audioProcessingEnabled) {
        info.clearActiveBufferRegion();
        return;
    }

    const double blockDuration = info.numSamples / sampleRate;
    const double blockDurationBeats = secondsToBeats(blockDuration);
    double startTime = position;
    double endTime = startTime + blockDuration;
    double startBeats = positionInBeats;
    double endBeats = startBeats + blockDurationBeats;

    info.clearActiveBufferRegion();

    if (!transportPlaying) {
        return;
    }

    if (loopModeEnabled) {
        if (loopTrackIndex < 0 || loopTrackIndex >= tracks.size() ||
            loopClipIndex < 0 || loopClipIndex >= tracks[loopTrackIndex].clips.size()) {
            LOG_ERROR("Invalid loop track or clip index");
            return;
        }

        auto& track = tracks[loopTrackIndex];
        auto& clip = track.clips[loopClipIndex];
        if (!dynamic_cast<MidiClip*>(clip.get())) {
            LOG_ERROR("Loop clip is not a MIDI clip");
            return;
        }

        // Проверяем зацикливание
        if (position >= loopStartTime + loopDuration) {
            position = loopStartTime;
            positionInBeats = secondsToBeats(position);
            startTime = position;
            endTime = startTime + blockDuration;
            startBeats = positionInBeats;
            endBeats = startBeats + blockDurationBeats;
            LOG("Looped back to: " << position << " seconds (" << positionInBeats << " beats)");
        }

        // Настройка буфера
        int pluginChannels = info.buffer->getNumChannels();
        pluginBuffer.setSize(pluginChannels, info.numSamples);
        pluginBuffer.clear();
        LOG("pluginBuffer: channels=" << pluginBuffer.getNumChannels() << ", samples=" << pluginBuffer.getNumSamples());
        juce::MidiBuffer midiBuffer;
        midiBuffer.clear();
        if (!midiBuffer.isEmpty()) {
            LOG_ERROR("midiBuffer not empty before adding events!");
        }

        if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
            constexpr double epsilon = 1.0e-10;
            double cycleTime = fmod(position - loopStartTime, loopDuration); // Время внутри цикла
            double cycleStartTime = cycleTime;
            double cycleEndTime = cycleTime + blockDuration;

            for (const auto& event : midiClip->midiSequence) {
                double eventTimeInClip = event->message.getTimeStamp(); // Время события в клипе (относительно 0)
                if (eventTimeInClip >= cycleStartTime - epsilon && eventTimeInClip < cycleEndTime) {
                    int sampleOffset = static_cast<int>((eventTimeInClip - cycleStartTime) * sampleRate);
                    if (sampleOffset >= 0 && sampleOffset < info.numSamples) {
                        midiBuffer.addEvent(event->message, sampleOffset);
                        LOG("Loop mode: Added MIDI event, note=" << event->message.getNoteNumber()
                            << ", type=" << (event->message.isNoteOn() ? "noteOn" : event->message.isNoteOff() ? "noteOff" : "other")
                            << ", time=" << eventTimeInClip << ", sampleOffset=" << sampleOffset
                            << ", channel=" << event->message.getChannel());
                    }
                }
            }
        }

        if (!track.muted && !track.plugins.empty()) {
            LOG("Processing MIDI buffer with " << midiBuffer.getNumEvents() << " events:");
            for (const auto& metadata : midiBuffer) {
                auto msg = metadata.getMessage();
                LOG("MIDI event: type=" << (msg.isNoteOn() ? "noteOn" : msg.isNoteOff() ? "noteOff" : "other")
                    << ", note=" << msg.getNoteNumber()
                    << ", channel=" << msg.getChannel()
                    << ", sampleOffset=" << metadata.samplePosition);
            }
            for (auto& pluginInstance : track.plugins) {
                if (pluginInstance->plugin && !pluginInstance->bypass) {
                    LOG("Pre-plugin buffer magnitude: " << pluginBuffer.getMagnitude(0, info.numSamples));
                    pluginInstance->plugin->processBlock(pluginBuffer, midiBuffer);
                    LOG("Post-plugin buffer magnitude: " << pluginBuffer.getMagnitude(0, info.numSamples));
                }
            }
            LOG("Output buffer magnitude before addFrom: " << info.buffer->getMagnitude(0, info.numSamples));
            for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
                info.buffer->addFrom(
                    channel, info.startSample, pluginBuffer,
                    channel % pluginBuffer.getNumChannels(),
                    0, info.numSamples, track.gain * clip->gain
                );
            }
        }

        // Применяем masterGain и userVolume к финальному буферу
        for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
            info.buffer->applyGain(channel, info.startSample, info.numSamples, masterGain * userVolume);
        }

        float maxSample = info.buffer->getMagnitude(0, info.numSamples);
        LOG("Loop mode buffer magnitude after gain: " << maxSample);
        LOG("Track gain: " << track.gain << ", Clip gain: " << clip->gain << ", Master gain: " << masterGain << ", User volume: " << userVolume);

        position += blockDuration;
        positionInBeats = secondsToBeats(position);
        updateActiveClips();
    }
    else {
        // Обычный режим воспроизведения
        for (auto& track : tracks) {
            pluginBuffer.setSize(info.buffer->getNumChannels(), info.numSamples);
            pluginBuffer.clear();
            bool hasAudio = false;

            for (auto& active : activeClips) {
                if (active.track != &track) continue;

                const ClipBase* clipToProcess = active.clip;
                if (auto* cloneClip = dynamic_cast<const CloneClip*>(active.clip)) {
                    clipToProcess = cloneClip->masterClip;
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

        // Применяем masterGain и userVolume к финальному буферу
        for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
            info.buffer->applyGain(channel, info.startSample, info.numSamples, masterGain * userVolume);
        }

        float maxSample = info.buffer->getMagnitude(0, info.numSamples);
        LOG("Output buffer magnitude after gain: " << maxSample);
        LOG("Master gain: " << masterGain << ", User volume: " << userVolume);

        position += blockDuration;
        positionInBeats = secondsToBeats(position);
        updateActiveClips();
    }
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

void Engine::Core::enableLoopMode(int trackIndex, int clipIndex) {
    const juce::ScopedLock sl(lock);
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        clipIndex < 0 || clipIndex >= tracks[trackIndex].clips.size()) {
        LOG_ERROR("Invalid track or clip index for loop mode: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
        return;
    }
    if (!dynamic_cast<MidiClip*>(tracks[trackIndex].clips[clipIndex].get())) {
        LOG_ERROR("Clip at trackIndex=" << trackIndex << ", clipIndex=" << clipIndex << " is not a MIDI clip");
        return;
    }

    // Останавливаем без allNotesOff
    transportPlaying = false;
    activeClips.clear();

    loopTrackIndex = trackIndex;
    loopClipIndex = clipIndex;
    loopModeEnabled = true;
    auto& clip = tracks[trackIndex].clips[clipIndex];

    loopStartTime = clip->startTime;
    loopDuration = clip->duration;
    //position = loopStartTime;
    //positionInBeats = secondsToBeats(position);
    setPosition(position);

    auto& track = tracks[trackIndex];
    for (auto& pluginInstance : track.plugins) {
        if (pluginInstance->plugin && !pluginInstance->bypass) {
            // Используем тот же размер блока, что в getNextAudioBlock
            pluginInstance->plugin->prepareToPlay(sampleRate, pluginBuffer.getNumSamples());
            LOG("Prepared plugin: " << pluginInstance->Path << " with samplesPerBlock=" << pluginBuffer.getNumSamples());
        }
    }

    updateActiveClips();
    LOG_SUCCESS("Loop mode enabled: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex
        << ", startTime=" << loopStartTime << " seconds, duration=" << loopDuration
        << ", position=" << position << " seconds");
}

void Engine::Core::disableLoopMode()
{
    const juce::ScopedLock sl(lock);
    if (!loopModeEnabled) {
        LOG("Loop mode already disabled");
        return;
    }

    // Сохраняем начальную позицию клипа перед отключением
    double clipStartTime = loopStartTime;

    loopModeEnabled = false;
    loopTrackIndex = -1;
    loopClipIndex = -1;
    loopStartTime = 0.0;
    loopDuration = 0.0;

    updateActiveClips();
    LOG_SUCCESS("Loop mode disabled, playhead set to: " << position << " seconds (" << positionInBeats << " beats)");
}

void Engine::Core::renderToFile(std::string& outputPath, std::function<void(float)> progressCallback) {
    LOG("Starting render to file: " << outputPath);

    // 1. Проверяем и корректируем путь
    juce::File outputFile(outputPath);
    if (!outputFile.hasFileExtension("wav") && !outputFile.hasFileExtension("mp3") && !outputFile.hasFileExtension("aiff")) {
        outputFile = outputFile.withFileExtension("wav");
        outputPath = outputFile.getFullPathName().toStdString();
        LOG("Added .wav extension to output path: " << outputPath);
    }

    // Проверяем и создаём родительскую директорию
    juce::File parentDir = outputFile.getParentDirectory();
    LOG("Checking parent directory: " << parentDir.getFullPathName().toStdString());
    if (!parentDir.exists()) {
        LOG("Parent directory does not exist, attempting to create...");
        if (!parentDir.createDirectory()) {
            char errorBuf[256];
            strerror_s(errorBuf, sizeof(errorBuf), errno);
            LOG_ERROR("Failed to create parent directory: " << parentDir.getFullPathName().toStdString() << ", errno: " << errno << ", error: " << errorBuf);
            return;
        }
        LOG("Parent directory created successfully");
    }
    else {
        LOG("Parent directory exists");
    }

    // Проверяем и создаём/пересоздаём файл для MP3
    juce::String fileExtension = outputFile.getFileExtension().toLowerCase();
    if (fileExtension == ".mp3") {
        LOG("Attempting to create MP3 output file: " << outputFile.getFullPathName().toStdString());
        if (outputFile.exists()) {
            LOG("MP3 file already exists, attempting to overwrite...");
            if (!outputFile.deleteFile()) {
                char errorBuf[256];
                strerror_s(errorBuf, sizeof(errorBuf), errno);
                LOG_ERROR("Failed to delete existing MP3 file: " << outputFile.getFullPathName().toStdString() << ", errno: " << errno << ", error: " << errorBuf);
                return;
            }
            LOG("Existing MP3 file deleted");
        }
        if (!outputFile.create()) {
            char errorBuf[256];
            strerror_s(errorBuf, sizeof(errorBuf), errno);
            LOG_ERROR("Failed to create MP3 output file: " << outputFile.getFullPathName().toStdString() << ", errno: " << errno << ", error: " << errorBuf);
            return;
        }
        LOG("MP3 output file created successfully");
    }

    // 2. Определяем максимальную длительность проекта
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

    // 3. Настраиваем параметры рендера
    const int samplesPerBlock = 512;
    const double renderSampleRate = sampleRate > 0 ? sampleRate : 44100.0;
    const int numChannels = 2;
    const int totalSamples = static_cast<int>(projectDuration * renderSampleRate);

    LOG("Render parameters: sampleRate=" << renderSampleRate << ", samplesPerBlock=" << samplesPerBlock << ", totalSamples=" << totalSamples);

    // 4. Сохраняем исходные значения позиции
    double originalPosition = position;
    double originalPositionInBeats = positionInBeats;

    // Сбрасываем позицию для рендера
    position = 0.0;
    positionInBeats = 0.0;
    updateActiveClips();

    // 5. Определяем формат и создаём соответствующий writer
    std::unique_ptr<juce::FileOutputStream> fileStream;
    std::unique_ptr<juce::AudioFormatWriter> writer;

    if (fileExtension == ".mp3") {
        // Рендер в MP3 через LAME
        FILE* mp3File = nullptr;
        std::string filePath = outputFile.getFullPathName().toStdString();
        LOG("Opening MP3 file for writing: " << filePath);
        if (fopen_s(&mp3File, filePath.c_str(), "wb") != 0) {
            char errorBuf[256];
            strerror_s(errorBuf, sizeof(errorBuf), errno);
            LOG_ERROR("Failed to open MP3 file for writing: " << filePath << ", errno: " << errno << ", error: " << errorBuf);
            return;
        }
        LOG("MP3 file opened successfully: " << filePath);

        lame_t lame = lame_init();
        if (!lame) {
            LOG_ERROR("Failed to initialize LAME encoder");
            fclose(mp3File);
            return;
        }
        LOG("LAME initialized successfully");

        lame_set_in_samplerate(lame, static_cast<int>(renderSampleRate));
        lame_set_num_channels(lame, numChannels);
        lame_set_brate(lame, 192);
        lame_set_mode(lame, STEREO);
        if (lame_init_params(lame) < 0) {
            LOG_ERROR("Failed to initialize LAME parameters");
            lame_close(lame);
            fclose(mp3File);
            return;
        }
        LOG("LAME parameters set successfully");

        juce::AudioBuffer<float> renderBuffer(numChannels, samplesPerBlock);
        int samplesRendered = 0;

        while (samplesRendered < totalSamples) {
            int samplesThisBlock = juce::jmin(samplesPerBlock, totalSamples - samplesRendered);
            renderBuffer.setSize(numChannels, samplesThisBlock, true);
            renderBuffer.clear();

            const double blockDuration = samplesThisBlock / renderSampleRate;
            const double startTime = position;
            const double endTime = startTime + blockDuration;

            for (auto& track : tracks) {
                pluginBuffer.setSize(numChannels, samplesThisBlock);
                pluginBuffer.clear();
                bool hasAudio = false;

                for (auto& active : activeClips) {
                    if (active.track != &track) continue;

                    const ClipBase* clipToProcess = active.clip;
                    if (auto* cloneClip = dynamic_cast<const CloneClip*>(active.clip)) clipToProcess = cloneClip->masterClip;
                    if (auto* audioClip = dynamic_cast<const AudioClip*>(clipToProcess)) {
                        if (!audioClip->muted && !active.track->muted) {
                            if (audioClip->useRAM) {
                                const int startSample = static_cast<int>((startTime - active.clip->startTime) * renderSampleRate);
                                const int numSamples = juce::jmin(samplesThisBlock, audioClip->buffer.getNumSamples() - startSample);
                                if (startSample >= 0 && numSamples > 0 && startSample < audioClip->buffer.getNumSamples()) {
                                    for (int channel = 0; channel < pluginBuffer.getNumChannels(); ++channel) {
                                        pluginBuffer.addFrom(channel, 0, audioClip->buffer, channel % audioClip->buffer.getNumChannels(), startSample, numSamples, active.track->gain * audioClip->gain);
                                    }
                                    hasAudio = true;
                                }
                            }
                        }
                    }
                }

                juce::MidiBuffer midiBuffer;
                for (auto& active : activeClips) {
                    if (active.track != &track) continue;

                    const ClipBase* clipToProcess = active.clip;
                    if (auto* cloneClip = dynamic_cast<const CloneClip*>(active.clip)) clipToProcess = cloneClip->masterClip;
                    if (auto* midiClip = dynamic_cast<const MidiClip*>(clipToProcess)) {
                        for (const auto& event : midiClip->midiSequence) {
                            double eventTime = active.clip->startTime + event->message.getTimeStamp();
                            const double epsilon = 0.015; // Синхронизировано с getNextAudioBlock
                            if (eventTime >= startTime - epsilon && eventTime < endTime) {
                                int sampleOffset = static_cast<int>((eventTime - startTime) * renderSampleRate);
                                if (sampleOffset < 0) sampleOffset = 0;
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
                    for (int channel = 0; channel < numChannels; ++channel) {
                        renderBuffer.addFrom(channel, 0, processedBuffer, channel % processedBuffer.getNumChannels(), 0, samplesThisBlock, track.gain);
                    }
                }
            }

            for (int channel = 0; channel < numChannels; ++channel) {
                renderBuffer.applyGain(channel, 0, samplesThisBlock, masterGain);
            }

            std::vector<short> pcmBuffer(samplesThisBlock * numChannels);
            for (int i = 0; i < samplesThisBlock; ++i) {
                for (int ch = 0; ch < numChannels; ++ch) {
                    pcmBuffer[i * numChannels + ch] = static_cast<short>(renderBuffer.getSample(ch, i) * 32767.0f);
                }
            }

            std::vector<unsigned char> mp3Buffer(7200);
            int bytesWritten = lame_encode_buffer_interleaved(lame, pcmBuffer.data(), samplesThisBlock, mp3Buffer.data(), mp3Buffer.size());
            if (bytesWritten > 0) {
                if (fwrite(mp3Buffer.data(), 1, bytesWritten, mp3File) != bytesWritten) {
                    char errorBuf[256];
                    strerror_s(errorBuf, sizeof(errorBuf), errno);
                    LOG_ERROR("Failed to write MP3 data to file: " << filePath << ", errno: " << errno << ", error: " << errorBuf);
                    lame_close(lame);
                    fclose(mp3File);
                    return;
                }
            }
            else if (bytesWritten < 0) {
                LOG_ERROR("LAME encoding error: " << bytesWritten);
                lame_close(lame);
                fclose(mp3File);
                return;
            }

            samplesRendered += samplesThisBlock;
            if (progressCallback) progressCallback(static_cast<float>(samplesRendered) / totalSamples);

            position += blockDuration;
            updateActiveClips();
        }

        int mp3BufferSize = 7200;
        std::vector<unsigned char> mp3Buffer(mp3BufferSize);
        int bytesWritten = lame_encode_flush(lame, mp3Buffer.data(), mp3BufferSize);
        if (bytesWritten > 0) {
            if (fwrite(mp3Buffer.data(), 1, bytesWritten, mp3File) != bytesWritten) {
                char errorBuf[256];
                strerror_s(errorBuf, sizeof(errorBuf), errno);
                LOG_ERROR("Failed to write MP3 flush data to file: " << filePath << ", errno: " << errno << ", error: " << errorBuf);
            }
        }

        lame_close(lame);
        fclose(mp3File);
        LOG_SUCCESS("MP3 rendering completed: " << filePath);
    }
    else {
        // Рендер в WAV или AIFF
        fileStream.reset(new juce::FileOutputStream(outputFile));
        if (!fileStream->openedOk()) {
            char errorBuf[256];
            strerror_s(errorBuf, sizeof(errorBuf), errno);
            LOG_ERROR("Failed to open file stream for writing: " << outputPath << ", errno: " << errno << ", error: " << errorBuf);
            return;
        }

        if (fileExtension == ".aiff") {
            juce::AiffAudioFormat aiffFormat;
            writer.reset(aiffFormat.createWriterFor(fileStream.release(), renderSampleRate, numChannels, 16, {}, 0));
            if (!writer) {
                LOG_ERROR("Failed to create AIFF writer for file: " << outputPath);
                return;
            }
        }
        else {
            juce::WavAudioFormat wavFormat;
            writer.reset(wavFormat.createWriterFor(fileStream.release(), renderSampleRate, numChannels, 16, {}, 0));
            if (!writer) {
                LOG_ERROR("Failed to create WAV writer for file: " << outputPath);
                return;
            }
        }

        juce::AudioBuffer<float> renderBuffer(numChannels, samplesPerBlock);
        juce::AudioSourceChannelInfo bufferInfo(&renderBuffer, 0, samplesPerBlock);
        int samplesRendered = 0;

        while (samplesRendered < totalSamples) {
            int samplesThisBlock = juce::jmin(samplesPerBlock, totalSamples - samplesRendered);
            bufferInfo.numSamples = samplesThisBlock;
            renderBuffer.clear();

            const double blockDuration = samplesThisBlock / renderSampleRate;
            const double startTime = position;
            const double endTime = startTime + blockDuration;

            for (auto& track : tracks) {
                pluginBuffer.setSize(numChannels, samplesThisBlock);
                pluginBuffer.clear();
                bool hasAudio = false;

                for (auto& active : activeClips) {
                    if (active.track != &track) continue;

                    const ClipBase* clipToProcess = active.clip;
                    if (auto* cloneClip = dynamic_cast<const CloneClip*>(active.clip)) clipToProcess = cloneClip->masterClip;
                    if (auto* audioClip = dynamic_cast<const AudioClip*>(clipToProcess)) {
                        if (!audioClip->muted && !active.track->muted) {
                            if (audioClip->useRAM) {
                                const int startSample = static_cast<int>((startTime - active.clip->startTime) * renderSampleRate);
                                const int numSamples = juce::jmin(samplesThisBlock, audioClip->buffer.getNumSamples() - startSample);
                                if (startSample >= 0 && numSamples > 0 && startSample < audioClip->buffer.getNumSamples()) {
                                    for (int channel = 0; channel < pluginBuffer.getNumChannels(); ++channel) {
                                        pluginBuffer.addFrom(channel, 0, audioClip->buffer, channel % audioClip->buffer.getNumChannels(), startSample, numSamples, active.track->gain * audioClip->gain);
                                    }
                                    hasAudio = true;
                                }
                            }
                        }
                    }
                }

                juce::MidiBuffer midiBuffer;
                for (auto& active : activeClips) {
                    if (active.track != &track) continue;

                    const ClipBase* clipToProcess = active.clip;
                    if (auto* cloneClip = dynamic_cast<const CloneClip*>(active.clip)) clipToProcess = cloneClip->masterClip;
                    if (auto* midiClip = dynamic_cast<const MidiClip*>(clipToProcess)) {
                        for (const auto& event : midiClip->midiSequence) {
                            double eventTime = active.clip->startTime + event->message.getTimeStamp();
                            const double epsilon = 0.01; // Синхронизировано с getNextAudioBlock
                            if (eventTime >= startTime - epsilon && eventTime < endTime) {
                                int sampleOffset = static_cast<int>((eventTime - startTime) * renderSampleRate);
                                if (sampleOffset < 0) sampleOffset = 0;
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
                    for (int channel = 0; channel < numChannels; ++channel) {
                        renderBuffer.addFrom(channel, 0, processedBuffer, channel % processedBuffer.getNumChannels(), 0, samplesThisBlock, track.gain);
                    }
                }
            }

            for (int channel = 0; channel < numChannels; ++channel) {
                renderBuffer.applyGain(channel, 0, samplesThisBlock, masterGain);
            }

            if (!writer->writeFromAudioSampleBuffer(renderBuffer, 0, samplesThisBlock)) {
                LOG_ERROR("Failed to write audio block at samplesRendered=" << samplesRendered);
                writer->flush();
                writer.reset();
                return;
            }

            samplesRendered += samplesThisBlock;
            float progress = static_cast<float>(samplesRendered) / totalSamples;
            if (progressCallback) progressCallback(progress);

            position += blockDuration;
            positionInBeats = secondsToBeats(position);
            updateActiveClips();
        }

        if (!writer->flush()) {
            LOG_ERROR("Failed to flush writer for file: " << outputPath);
            writer.reset();
            return;
        }
        writer.reset();
    }

    LOG("File written and closed: " << outputPath);

    // Восстанавливаем позицию
    position = originalPosition;
    positionInBeats = originalPositionInBeats;
    updateActiveClips();

    // Проверяем существование файла
    if (!outputFile.existsAsFile()) {
        LOG_ERROR("Rendered file does not exist: " << outputPath);
        return;
    }

    // Уведомляем о завершении
    if (progressCallback) progressCallback(1.0f);

    LOG_SUCCESS("Render completed successfully to: " << outputPath);
}
void Engine::Core::playNote(int trackIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel)
{
    const juce::ScopedLock sl(lock); // Защищаем доступ

    // Проверка параметров
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Invalid track index: " << trackIndex);
        return;
    }
    if (!tracks[trackIndex].isMidiTrack) {
        LOG_ERROR("Track at index " << trackIndex << " is not a MIDI track");
        return;
    }
    if (noteNumber < 0 || noteNumber > 127 || velocity < 0.0f || velocity > 1.0f || channel < 1 || channel > 16) {
        LOG_ERROR("Invalid note parameters: noteNumber=" << noteNumber
            << ", velocity=" << velocity << ", channel=" << channel);
        return;
    }
    if (durationBeats <= 0.0) {
        LOG_ERROR("Invalid duration: " << durationBeats << " beats");
        return;
    }

    // Создаем MIDI-сообщения
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(channel, noteNumber, velocity);
    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(channel, noteNumber);

    // Подготавливаем буфер
    const int blockSize = 256; // Увеличиваем до 256 для стабильности
    juce::AudioBuffer<float> tempBuffer(2, blockSize); // Стерео
    juce::MidiBuffer midiBuffer;

    // Сохраняем состояние
    bool wasPlaying = transportPlaying;
    transportPlaying = true; // Включаем для обработки

    // Обрабатываем noteOn
    tempBuffer.clear();
    midiBuffer.addEvent(noteOn, 0);
    for (auto& pluginInstance : tracks[trackIndex].plugins) {
        if (pluginInstance->plugin && !pluginInstance->bypass) {
            // Убедимся, что плагин готов
            pluginInstance->plugin->prepareToPlay(sampleRate, blockSize);
            pluginInstance->plugin->processBlock(tempBuffer, midiBuffer);
            LOG("Processed noteOn through plugin: " << pluginInstance->Path);
        }
    }

    // Проверяем буфер
    float maxSample = tempBuffer.getMagnitude(0, blockSize);
    LOG("noteOn buffer magnitude: " << maxSample);



    // Планируем noteOff
    double durationSeconds = beatsToSeconds(durationBeats);
    juce::Timer::callAfterDelay(
        static_cast<int>((durationSeconds + 0.1) * 1000.0), // 100 мс запас для затухания
        [this, trackIndex, channel, noteNumber, wasPlaying]() {
            const juce::ScopedLock sl(lock);
            transportPlaying = true;
            juce::AudioBuffer<float> tempBuffer(2, 256);
            juce::MidiBuffer midiBuffer;
            midiBuffer.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), 0);
            for (auto& pluginInstance : tracks[trackIndex].plugins) {
                if (pluginInstance->plugin && !pluginInstance->bypass) {
                    pluginInstance->plugin->prepareToPlay(sampleRate, 256);
                    pluginInstance->plugin->processBlock(tempBuffer, midiBuffer);
                    LOG("Processed noteOff through plugin: " << pluginInstance->Path);
                }
            }
            float maxSample = tempBuffer.getMagnitude(0, 256);
            LOG("noteOff buffer magnitude: " << maxSample);
            if (midiOutput) {
                midiOutput->sendMessageNow(juce::MidiMessage::noteOff(channel, noteNumber));
                LOG("Sent noteOff to MIDI output: noteNumber=" << noteNumber);
            }
            transportPlaying = wasPlaying;
        }
    );

    transportPlaying = wasPlaying; // Восстанавливаем
    LOG_SUCCESS("Played note: trackIndex=" << trackIndex << ", noteNumber=" << noteNumber
        << ", durationBeats=" << durationBeats);
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
    audioProcessingEnabled = true;
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

    if (midiOutput) {
        for (int channel = 1; channel <= 16; ++channel) {
            midiOutput->sendMessageNow(juce::MidiMessage::allNotesOff(channel));
            LOG("Sent allNotesOff for channel " << channel << " via midiOutput");
        }
    }

    // Отправляем allNotesOff через плагины для всех треков
    for (auto& track : tracks) {
        if (track.isMidiTrack && !track.plugins.empty()) {
            juce::MidiBuffer midiBuffer;
            for (int channel = 1; channel <= 16; ++channel) {
                midiBuffer.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
            }
            juce::AudioBuffer<float> tempBuffer(2, 512); // Временный буфер
            tempBuffer.clear();
            for (auto& pluginInstance : track.plugins) {
                if (pluginInstance->plugin && !pluginInstance->bypass) {
                    pluginInstance->plugin->processBlock(tempBuffer, midiBuffer);
                    LOG("Sent allNotesOff through plugin: " << pluginInstance->Path);
                }
            }
        }
    }

    if (loopModeEnabled) {
        if (loopTrackIndex < 0 || loopTrackIndex >= tracks.size() ||
            loopClipIndex < 0 || loopClipIndex >= tracks[loopTrackIndex].clips.size()) {
            LOG_ERROR("Invalid loop track or clip index in setPosition");
            return;
        }

        // Ограничиваем newPosition в пределах [loopStartTime, loopStartTime + loopDuration]
        double minPosition = loopStartTime;
        double maxPosition = loopStartTime + loopDuration;
        double clampedPosition = juce::jlimit(minPosition, maxPosition, newPosition);

        if (clampedPosition != newPosition) {
            LOG_WARN("Attempted to move playhead to " << newPosition << " seconds, "
                << "clamped to " << clampedPosition << " seconds (loop boundaries: "
                << minPosition << " to " << maxPosition << ")");
        }

        position = clampedPosition;
    }
    else {
        position = newPosition;
    }

    positionInBeats = secondsToBeats(position);
    updateActiveClips();
    {
        const juce::ScopedLock noteSl(noteLock);
        activeNotes.clear();
    }
    LOG("Playhead moved to: " << position << " seconds (" << positionInBeats << " beats)");
}

void Engine::Core::loadAudioClip(int trackIndex, const juce::File& file, double startBeats, bool loadToRAM) {
    if (trackIndex < 0 || trackIndex >= tracks.size()) return;

    auto clip = std::make_unique<AudioClip>();
    clip->file = file;
    clip->startBeats = startBeats;
    clip->startTime = beatsToSeconds(startBeats);
    clip->clipID = clip->generateClipID();
    clip->color = clip->generateUniqueColor(clip->clipID);

    juce::AudioFormatReader* reader = formatManager.createReaderFor(file);
    if (reader) {
        clip->duration = reader->lengthInSamples / reader->sampleRate;
        clip->durationBeats = secondsToBeats(clip->duration);
    }
    else if (file.getFileExtension().toLowerCase() == ".mp3") {
        FILE* mp3File = nullptr;
        fopen_s(&mp3File, file.getFullPathName().toRawUTF8(), "rb");
        if (mp3File) {
            lame_t lame = lame_init();
            lame_set_decode_only(lame, 1);
            hip_t hip = hip_decode_init();
            std::vector<short> pcmBuffer(1152 * 2);
            std::vector<unsigned char> mp3Buffer(7200);
            int read, samples;
            double totalSamples = 0;

            while ((read = fread(mp3Buffer.data(), 1, mp3Buffer.size(), mp3File)) > 0) {
                samples = hip_decode(hip, mp3Buffer.data(), read, pcmBuffer.data(), pcmBuffer.data() + 1152);
                if (samples > 0) totalSamples += samples / 2; // Считаем стерео-сэмплы
            }

            hip_decode_exit(hip);
            lame_close(lame);
            fclose(mp3File);

            clip->duration = totalSamples / 44100.0; // Предполагаем 44.1kHz
            clip->durationBeats = secondsToBeats(clip->duration);
        }
    }

    if (loadToRAM && clip->duration > 0) {
        if (reader) {
            clip->buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&clip->buffer, 0, (int)reader->lengthInSamples, 0, true, true);
        }
        else if (file.getFileExtension().toLowerCase() == ".mp3") {
            FILE* mp3File = nullptr;
            fopen_s(&mp3File, file.getFullPathName().toRawUTF8(), "rb");
            if (mp3File) {
                lame_t lame = lame_init();
                lame_set_decode_only(lame, 1);
                hip_t hip = hip_decode_init();
                std::vector<short> pcmBuffer(1152 * 2);
                std::vector<unsigned char> mp3Buffer(7200);
                int read, samples;
                clip->buffer.setSize(2, static_cast<int>(clip->duration * 44100.0)); // Стерео
                clip->buffer.clear();
                int writePos = 0;

                while ((read = fread(mp3Buffer.data(), 1, mp3Buffer.size(), mp3File)) > 0) {
                    samples = hip_decode(hip, mp3Buffer.data(), read, pcmBuffer.data(), pcmBuffer.data() + 1152);
                    if (samples > 0 && writePos + samples / 2 <= clip->buffer.getNumSamples()) {
                        for (int i = 0; i < samples / 2; ++i) {
                            clip->buffer.setSample(0, writePos + i, pcmBuffer[i * 2] / 32767.0f);
                            clip->buffer.setSample(1, writePos + i, pcmBuffer[i * 2 + 1] / 32767.0f);
                        }
                        writePos += samples / 2;
                    }
                }

                hip_decode_exit(hip);
                lame_close(lame);
                fclose(mp3File);
            }
        }
        clip->useRAM = true;
        // Пересчёт waveformData (как в оригинале)
        int numSamples = clip->buffer.getNumSamples();
        int numChannels = clip->buffer.getNumChannels();
        const int minSamplesPerPoint = 100;
        const int maxSamplesPerPoint = 1000;
        const int maxSampleCount = 10000;
        int sampleCount = numSamples / minSamplesPerPoint;
        sampleCount = std::max(1, std::min(sampleCount, maxSampleCount));
        if (numSamples / sampleCount > maxSamplesPerPoint) sampleCount = numSamples / maxSamplesPerPoint;
        int step = numSamples / sampleCount;
        if (step < 1) step = 1;

        std::vector<float> waveformData(sampleCount);
        for (int i = 0; i < sampleCount && i * step < numSamples; ++i) {
            float maxAmplitude = 0.0f;
            for (int j = 0; j < step; ++j) {
                int sampleIdx = i * step + j;
                float amplitude = 0.0f;
                for (int c = 0; c < numChannels; ++c) {
                    if (sampleIdx < numSamples) amplitude += std::abs(clip->buffer.getSample(c, sampleIdx));
                }
                amplitude /= numChannels;
                maxAmplitude = std::max(maxAmplitude, amplitude);
            }
            waveformData[i] = maxAmplitude;
        }
        clip->waveformData = std::move(waveformData);
    }

    if (reader) delete reader;
    tracks[trackIndex].clips.push_back(std::move(clip));
    LOG_SUCCESS("Loaded audio clip: trackIndex=" << trackIndex << ", file=" << file.getFullPathName().toStdString());
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
    newClip->color = newClip->generateUniqueColor(newClip->clipID);
    LOG("StartBeat for new clip set: " << startBeats);
    LOG("StartTime for new clip set: " << newClip->startTime);

    double endTime = 0.0;
    for (int i = 0; i < sequence.getNumEvents(); i++) {
        auto event = sequence.getEventPointer(i);
        endTime = juce::jmax(endTime, event->message.getTimeStamp());
    }

    if (endTime == 0.0) {
        newClip->durationBeats = 8.0; // Пустой клип: 4 бита
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
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Invalid track index: trackIndex=" << trackIndex << ", tracks.size=" << tracks.size());
        return nullptr;
    }
    if (pluginIndex < 0 || pluginIndex >= tracks[trackIndex].plugins.size()) {
        LOG_ERROR("Invalid plugin index: pluginIndex=" << pluginIndex << ", plugins.size="
            << tracks[trackIndex].plugins.size() << ", trackIndex=" << trackIndex);
        return nullptr;
    }

    auto& pluginInstance = tracks[trackIndex].plugins[pluginIndex];
    if (!pluginInstance->plugin) {
        LOG_ERROR("Plugin instance is null for trackIndex=" << trackIndex << ", pluginIndex=" << pluginIndex);
        return nullptr;
    }

    if (!pluginInstance->editor) {
        pluginInstance->editor = pluginInstance->plugin->createEditorIfNeeded();
        if (!pluginInstance->editor) {
            LOG_ERROR("Failed to create editor for plugin: trackIndex=" << trackIndex
                << ", pluginIndex=" << pluginIndex << ", plugin=" << (void*)pluginInstance->plugin.get());
            return nullptr;
        }
        LOG("Editor created: trackIndex=" << trackIndex << ", pluginIndex=" << pluginIndex
            << ", plugin=" << (void*)pluginInstance->plugin.get() << ", editor=" << (void*)pluginInstance->editor);
    }
    else {
        LOG("Editor already exists: trackIndex=" << trackIndex << ", pluginIndex=" << pluginIndex
            << ", editor=" << (void*)pluginInstance->editor);
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
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        clipIndex < 0 || clipIndex >= tracks[trackIndex].clips.size()) {
        LOG_ERROR("Invalid track or clip index: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
        return;
    }

    const juce::ScopedLock sl(lock);
    auto& clip = tracks[trackIndex].clips[clipIndex];
    double newStartTime = beatsToSeconds(startBeats);

    // Сохраняем относительную позицию курсора в цикле, если в PAT mode
    double relativePosition = 0.0;
    if (loopModeEnabled && loopTrackIndex == trackIndex && loopClipIndex == clipIndex) {
        relativePosition = position - loopStartTime;
        if (relativePosition < 0 || relativePosition >= loopDuration) {
            relativePosition = 0.0; // Если курсор вне цикла, начинаем с начала
        }
        LOG("PAT mode active, preserving relative position: " << relativePosition << " seconds");
    }

    // Обновляем позицию клипа
    clip->startTime = newStartTime;
    clip->startBeats = startBeats;
    LOG("Moved clip at trackIndex=" << trackIndex << ", clipIndex=" << clipIndex
        << " to startBeats=" << startBeats << ", startTime=" << newStartTime << " seconds");

    // Обновляем параметры цикла, если клип активен в PAT mode
    if (loopModeEnabled && loopTrackIndex == trackIndex && loopClipIndex == clipIndex) {
        loopStartTime = newStartTime;
        loopDuration = clip->duration;
        double newPosition = loopStartTime + relativePosition;
        LOG("Updated loop parameters: loopStartTime=" << loopStartTime << ", loopDuration=" << loopDuration
            << ", newPosition=" << newPosition << " seconds");
        setPosition(newPosition); // Безопасное обновление позиции
    }

    // Очищаем MIDI-события только если необходимо
    if (loopModeEnabled && loopTrackIndex == trackIndex && loopClipIndex == clipIndex) {
        juce::MidiBuffer clearBuffer;
        clearBuffer.addEvent(juce::MidiMessage::allNotesOff(1), 0); // Только для канала 1
        for (auto& pluginInstance : tracks[trackIndex].plugins) {
            if (pluginInstance->plugin && !pluginInstance->bypass) {
                juce::AudioBuffer<float> tempBuffer(2, 512);
                tempBuffer.clear();
                pluginInstance->plugin->processBlock(tempBuffer, clearBuffer);
            }
        }
    }

    updateActiveClips();

}

void Engine::Core::copyMidiClip(int trackIndex, int clipIndex, double startTime) {
    const juce::ScopedLock sl(lock); // Защищаем доступ к данным

    // Проверки валидности уже выполнены в Engine::CopyMidiClip, но дублируем для безопасности
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Недопустимый индекс трека в Core: " << trackIndex);
        return;
    }

    auto& track = tracks[trackIndex];
    if (!track.isMidiTrack) {
        LOG_ERROR("Трек не является MIDI-треком в Core: " << trackIndex);
        return;
    }

    if (clipIndex < 0 || clipIndex >= track.clips.size()) {
        LOG_ERROR("Недопустимый индекс клипа в Core: " << clipIndex);
        return;
    }

    if (startTime < 0.0) {
        LOG_ERROR("Недопустимое время начала в Core: " << startTime);
        return;
    }

    // Проверяем тип клипа
    auto* midiClip = dynamic_cast<MidiClip*>(track.clips[clipIndex].get());
    auto* cloneClip = dynamic_cast<CloneClip*>(track.clips[clipIndex].get());

    if (!midiClip && !cloneClip) {
        LOG_ERROR("Клип не является ни MidiClip, ни CloneClip: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
        return;
    }

    std::unique_ptr<ClipBase> newClip;
    double newStartBeats = secondsToBeats(startTime); // Преобразуем startTime в startBeats

    if (midiClip) {
        // Копируем MidiClip
        newClip = std::make_unique<MidiClip>();
        auto* newMidiClip = dynamic_cast<MidiClip*>(newClip.get());
        newMidiClip->startTime = startTime;
        newMidiClip->startBeats = newStartBeats;
        newMidiClip->duration = midiClip->duration;
        newMidiClip->durationBeats = midiClip->durationBeats;
        newMidiClip->midiSequence = midiClip->midiSequence; // Копируем MIDI-события
        newMidiClip->clipID = newMidiClip->generateClipID();
        newMidiClip->color = newMidiClip->generateUniqueColor(newMidiClip->clipID);
        newMidiClip->name = midiClip->name;
        newMidiClip->gain = midiClip->gain;
        newMidiClip->muted = midiClip->muted;
        newMidiClip->minDurationBeats = midiClip->minDurationBeats;
        LOG_SUCCESS("Скопирован MidiClip: trackIndex=" << trackIndex << ", newStartTime=" << startTime << ", clipID=" << newMidiClip->clipID);
    }
    else if (cloneClip) {
        // Копируем CloneClip
        if (!cloneClip->masterClip) {
            LOG_ERROR("Мастер-клип не найден для CloneClip: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex);
            return;
        }

        // Находим индекс мастер-клипа
        int masterClipIndex = -1;
        for (size_t i = 0; i < track.clips.size(); ++i) {
            if (track.clips[i].get() == cloneClip->masterClip) {
                masterClipIndex = static_cast<int>(i);
                break;
            }
        }
        if (masterClipIndex == -1) {
            LOG_ERROR("Мастер-клип не найден в треке: trackIndex=" << trackIndex);
            return;
        }

        // Создаем новый CloneClip
        newClip = std::make_unique<CloneClip>(cloneClip->masterClip, newStartBeats);
        auto* newCloneClip = dynamic_cast<CloneClip*>(newClip.get());
        newCloneClip->startTime = startTime;
        newCloneClip->startBeats = newStartBeats;
        newCloneClip->duration = cloneClip->masterClip->duration;
        newCloneClip->durationBeats = cloneClip->masterClip->durationBeats;
        newCloneClip->masterClip = cloneClip->masterClip;
        newCloneClip->masterClipID = cloneClip->masterClipID; // Обновляем masterClipID
        newCloneClip->clipID = newCloneClip->generateClipID();
        newCloneClip->color = cloneClip->generateUniqueColor(newCloneClip->clipID);
        newCloneClip->name = cloneClip->name;
        newCloneClip->gain = cloneClip->gain;
        newCloneClip->muted = cloneClip->muted;
        LOG_SUCCESS("Скопирован CloneClip: trackIndex=" << trackIndex << ", newStartTime=" << startTime << ", clipID=" << newCloneClip->clipID << ", masterClipIndex=" << masterClipIndex);
    }

    // Добавляем новый клип в трек
    track.clips.push_back(std::move(newClip));
    updateActiveClips();
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
  //  LOG("Updating active clips at position: " << position << " seconds (" << positionInBeats << " beats)");

    if (loopModeEnabled) {
        if (loopTrackIndex < 0 || loopTrackIndex >= tracks.size() ||
            loopClipIndex < 0 || loopClipIndex >= tracks[loopTrackIndex].clips.size()) {
            LOG_ERROR("Invalid loop track or clip index");
            return;
        }

        auto& track = tracks[loopTrackIndex];
        if (track.muted) {
            LOG("Track is muted, no active clips added");
            return;
        }

        auto& clip = track.clips[loopClipIndex];
        if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
            ActiveClip active;
            active.clip = clip.get();
            active.track = &track;
          //  LOG("Added MIDI clip for loop mode at startTime: " << midiClip->startTime);
            activeClips.add(std::move(active));
        }
        else {
            LOG_ERROR("Loop clip is not a MIDI clip");
        }
    }
    else {
        for (auto& track : tracks) {
            if (track.muted) continue;

            for (auto& clip : track.clips) {
                if (clip->isActive(position)) {
                    ActiveClip active;
                    active.clip = clip.get();
                    active.track = &track;

                    if (auto* cloneClip = dynamic_cast<CloneClip*>(clip.get())) {
                        if (auto* audioClip = dynamic_cast<AudioClip*>(cloneClip->masterClip)) {
                            if (!audioClip->useRAM) {
                                if (auto reader = formatManager.createReaderFor(audioClip->file)) {
                                    auto readerPtr = std::unique_ptr<juce::AudioFormatReader>(reader);
                                    active.source = std::make_unique<juce::AudioFormatReaderSource>(
                                        readerPtr.release(), true);
                                    active.source->prepareToPlay(512, sampleRate);
                                    juce::int64 readPosition = static_cast<juce::int64>((position - clip->startTime) * sampleRate);
                                    active.source->setNextReadPosition(readPosition);
                                    //LOG("Added non-RAM clone audio clip at startTime: " << clip->startTime);
                                }
                                else {
                                   // LOG_ERROR("Failed to create reader for file: " << audioClip->file.getFullPathName().toStdString());
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
                               // LOG("Added non-RAM audio clip at startTime: " << audioClip->startTime);
                            }
                            else {
                               // LOG_ERROR("Failed to create reader for file: " << audioClip->file.getFullPathName().toStdString());
                            }
                        }
                    }
                    else if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
                        //LOG("Added MIDI clip at startTime: " << midiClip->startTime);
                    }
                    activeClips.add(std::move(active));
                }
            }
        }
    }
 //   LOG("Total active clips: " << activeClips.size());
}

void Engine::Core::setName(int trackIndex, std::string name)
{
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Invalid track or clip index");
        return;
    }
    tracks[trackIndex].name = name;
}

void Engine::Core::toggleSolo(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        LOG_ERROR("Invalid track index for ToggleSolo: " << trackIndex);
        return;
    }

    auto& selectedTrack = tracks[trackIndex];

    // Переключаем режим соло для выбранной дорожки
    selectedTrack.solo = !selectedTrack.solo;

    if (selectedTrack.solo) {
        // Включаем соло: заглушаем все остальные дорожки
        for (size_t i = 0; i < tracks.size(); ++i) {
            if (i != static_cast<size_t>(trackIndex)) {
                tracks[i].muted = true;
                tracks[i].solo = false; // Отключаем соло у других дорожек
                LOG("Track " << i << " muted due to solo on track " << trackIndex);
            }
            else {
                tracks[i].muted = false; // Убедимся, что соло-дорожка не заглушена
            }
        }
        LOG_SUCCESS("Solo mode enabled for track " << trackIndex);
    }
    else {
        // Отключаем соло: снимаем заглушение со всех дорожек
        for (size_t i = 0; i < tracks.size(); ++i) {
            tracks[i].muted = false;
            LOG("Track " << i << " unmuted");
        }
        LOG_SUCCESS("Solo mode disabled for track " << trackIndex);
    }

    updateActiveClips(); // Обновляем активные клипы
}

void Engine::Core::loadClipToRAM(AudioClip& clip) {
    if (auto reader = formatManager.createReaderFor(clip.file)) {
        clip.buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&clip.buffer, 0, (int)reader->lengthInSamples, 0, true, true);
        clip.useRAM = true;

        // Пересчитываем waveformData
        int numSamples = clip.buffer.getNumSamples();
        int numChannels = clip.buffer.getNumChannels();

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
                        amplitude += std::abs(clip.buffer.getSample(c, sampleIdx));
                    }
                }
                amplitude /= numChannels;
                maxAmplitude = std::max(maxAmplitude, amplitude);
            }
            waveformData[i] = maxAmplitude;
        }

        clip.waveformData = std::move(waveformData);
        LOG("Waveform data calculated for clipID=" << clip.clipID << ", samples=" << sampleCount);
        delete reader;
    }
    else {
        LOG_ERROR("Failed to create reader for file: " << clip.file.getFullPathName().toStdString());
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
        // Собираем все пары noteOn/noteOff
        struct NoteEvent {
            int noteOnIndex;
            int noteOffIndex;
            int noteNumber;
            int channel;
            double startTime;
            double endTime;
            float velocity;
        };
        std::vector<NoteEvent> noteEvents;
        std::map<std::pair<int, int>, int> noteOnIndices;

        for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
            auto* event = midiClip->midiSequence.getEventPointer(i);
            if (event->message.isNoteOn()) {
                noteOnIndices[{event->message.getChannel(), event->message.getNoteNumber()}] = i;
            }
            else if (event->message.isNoteOff()) {
                auto key = std::pair<int, int>{ event->message.getChannel(), event->message.getNoteNumber() };
                if (noteOnIndices.count(key)) {
                    auto* noteOnEvent = midiClip->midiSequence.getEventPointer(noteOnIndices[key]);
                    noteEvents.push_back({
                        noteOnIndices[key],
                        i,
                        event->message.getNoteNumber(),
                        event->message.getChannel(),
                        noteOnEvent->message.getTimeStamp(),
                        event->message.getTimeStamp(),
                        noteOnEvent->message.getVelocity() / 127.0f
                        });
                    noteOnIndices.erase(key);
                }
            }
        }

        // Проверяем валидность noteIndex
        if (noteIndex < 0 || noteIndex >= noteEvents.size()) {
            LOG_ERROR("Invalid noteIndex: " << noteIndex << ", total notes: " << noteEvents.size());
            return;
        }

        // Удаляем старую пару noteOn/noteOff
        auto& noteEvent = noteEvents[noteIndex];
        LOG("Deleting noteOn at index: " << noteEvent.noteOnIndex << ", noteNumber: " << noteEvent.noteNumber);
        LOG("Deleting noteOff at index: " << noteEvent.noteOffIndex << ", noteNumber: " << noteEvent.noteNumber);
        if (noteEvent.noteOffIndex > noteEvent.noteOnIndex) {
            midiClip->midiSequence.deleteEvent(noteEvent.noteOffIndex, false);
            midiClip->midiSequence.deleteEvent(noteEvent.noteOnIndex, false);
        }
        else {
            midiClip->midiSequence.deleteEvent(noteEvent.noteOnIndex, false);
            midiClip->midiSequence.deleteEvent(noteEvent.noteOffIndex, false);
        }

        // Используем исходную velocity, если новая velocity равна 0
        float finalVelocity = (velocity > 0.0f) ? velocity : noteEvent.velocity;
        if (finalVelocity == 0.0f) {
            finalVelocity = 0.787402f; // Дефолтное значение, если velocity не задано
            LOG_WARN("Using default velocity: " << finalVelocity);
        }

        // Добавляем новую ноту
        double startTimeSeconds = beatsToSeconds(startBeats);
        double endTimeSeconds = beatsToSeconds(startBeats + durationBeats);
        LOG("Adding noteOn: noteNumber=" << noteNumber << ", time=" << startTimeSeconds << ", velocity=" << finalVelocity);
        LOG("Adding noteOff: noteNumber=" << noteNumber << ", time=" << endTimeSeconds);
        midiClip->midiSequence.addEvent(juce::MidiMessage::noteOn(channel, noteNumber, finalVelocity), startTimeSeconds);
        midiClip->midiSequence.addEvent(juce::MidiMessage::noteOff(channel, noteNumber), endTimeSeconds);

        // Синхронизируем пары
        midiClip->midiSequence.updateMatchedPairs();

        // Логируем содержимое midiSequence
        LOG("Current midiSequence events after update:");
        for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
            auto* event = midiClip->midiSequence.getEventPointer(i);
            LOG("Event " << i << ": type=" << (event->message.isNoteOn() ? "noteOn" : event->message.isNoteOff() ? "noteOff" : "other")
                << ", noteNumber=" << event->message.getNoteNumber()
                << ", channel=" << event->message.getChannel()
                << ", time=" << event->message.getTimeStamp()
                << ", velocity=" << (event->message.isNoteOn() ? event->message.getVelocity() / 127.0f : 0.0f));
        }

        // Обновляем длительность клипа
        double maxEndTime = 0.0;
        for (const auto& event : midiClip->midiSequence) {
            maxEndTime = juce::jmax(maxEndTime, event->message.getTimeStamp());
        }
        midiClip->duration = juce::jmax(midiClip->duration, maxEndTime + 0.1);
        midiClip->durationBeats = secondsToBeats(midiClip->duration);

        updateActiveClips();
        LOG("Updated MIDI note, new durationBeats: " << midiClip->durationBeats);
    }
    else {
        LOG_ERROR("Clip is not a MidiClip");
    }
}

void Engine::Core::cleanMidiSequence(MidiClip* clip)
{
    if (!clip) {
        LOG_ERROR("Null MidiClip passed to cleanMidiSequence");
        return;
    }

    juce::MidiMessageSequence& sequence = clip->midiSequence;
    juce::MidiMessageSequence cleanedSequence;

    // Фиксируем ticksPerQuarterNote для последовательности
    const int ticksPerQuarterNote = 960;

    for (int i = 0; i < sequence.getNumEvents(); ++i) {
        auto* event = sequence.getEventPointer(i);
        if (event) {
            double timestamp = event->message.getTimeStamp();
            if (timestamp >= 0) {
                if (event->message.isNoteOnOrOff() || event->message.isController() ||
                    event->message.isProgramChange() || event->message.isPitchWheel() ||
                    event->message.isChannelPressure() || event->message.isAftertouch() ||
                    event->message.isMetaEvent() || event->message.isSysEx()) {
                    cleanedSequence.addEvent(event->message, timestamp);
                    LOG("Added MIDI event for clipID=" << clip->clipID << ", timestamp=" << timestamp);
                }
                else {
                    LOG("Skipped unknown MIDI message for clipID=" << clip->clipID << ", timestamp=" << timestamp);
                }
            }
            else {
                LOG("Skipped MIDI event with negative timestamp for clipID=" << clip->clipID << ", timestamp=" << timestamp);
            }
        }
    }

    sequence.swapWith(cleanedSequence);
    sequence.sort();
    LOG("Cleaned MIDI sequence for clipID=" << clip->clipID << ", events: " << sequence.getNumEvents()
        << ", ticksPerQuarterNote=" << ticksPerQuarterNote);
}

void Engine::Core::deleteMidiNote(int trackIndex, int clipIndex, int noteIndex) {
    if (trackIndex < 0 || trackIndex >= tracks.size() ||
        clipIndex < 0 || clipIndex >= tracks[trackIndex].clips.size()) {
        LOG_ERROR("Invalid track or clip index");
        return;
    }

    auto& clip = tracks[trackIndex].clips[clipIndex];
    if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
        // Собираем все пары noteOn/noteOff
        struct NoteEvent {
            int noteOnIndex;
            int noteOffIndex;
            int noteNumber;
            int channel;
            double startTime;
            double endTime;
        };
        std::vector<NoteEvent> noteEvents;
        std::map<std::pair<int, int>, int> noteOnIndices;

        for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
            auto* event = midiClip->midiSequence.getEventPointer(i);
            if (event->message.isNoteOn()) {
                noteOnIndices[{event->message.getChannel(), event->message.getNoteNumber()}] = i;
            }
            else if (event->message.isNoteOff()) {
                auto key = std::pair<int, int>{ event->message.getChannel(), event->message.getNoteNumber() };
                if (noteOnIndices.count(key)) {
                    noteEvents.push_back({
                        noteOnIndices[key],
                        i,
                        event->message.getNoteNumber(),
                        event->message.getChannel(),
                        midiClip->midiSequence.getEventPointer(noteOnIndices[key])->message.getTimeStamp(),
                        event->message.getTimeStamp()
                        });
                    noteOnIndices.erase(key);
                }
            }
        }

        // Проверяем валидность noteIndex
        if (noteIndex < 0 || noteIndex >= noteEvents.size()) {
            LOG_ERROR("Invalid noteIndex: " << noteIndex << ", total notes: " << noteEvents.size());
            return;
        }

        // Удаляем пару noteOn/noteOff
        auto& noteEvent = noteEvents[noteIndex];
        LOG("Deleting noteOn at index: " << noteEvent.noteOnIndex << ", noteNumber: " << noteEvent.noteNumber);
        LOG("Deleting noteOff at index: " << noteEvent.noteOffIndex << ", noteNumber: " << noteEvent.noteNumber);
        if (noteEvent.noteOffIndex > noteEvent.noteOnIndex) {
            midiClip->midiSequence.deleteEvent(noteEvent.noteOffIndex, false);
            midiClip->midiSequence.deleteEvent(noteEvent.noteOnIndex, false);
        }
        else {
            midiClip->midiSequence.deleteEvent(noteEvent.noteOnIndex, false);
            midiClip->midiSequence.deleteEvent(noteEvent.noteOffIndex, false);
        }

        // Синхронизируем пары
        midiClip->midiSequence.updateMatchedPairs();

        // Логируем содержимое midiSequence
        LOG("Current midiSequence events after deletion:");
        for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
            auto* event = midiClip->midiSequence.getEventPointer(i);
            LOG("Event " << i << ": type=" << (event->message.isNoteOn() ? "noteOn" : event->message.isNoteOff() ? "noteOff" : "other")
                << ", noteNumber=" << event->message.getNoteNumber()
                << ", channel=" << event->message.getChannel()
                << ", time=" << event->message.getTimeStamp());
        }

        // Обновляем длительность клипа
        double maxEndTime = 0.0;
        for (const auto& event : midiClip->midiSequence) {
            maxEndTime = juce::jmax(maxEndTime, event->message.getTimeStamp());
        }
        midiClip->duration = juce::jmax(4.0, maxEndTime + 0.1); // Минимальная длительность 4 beats
        midiClip->durationBeats = secondsToBeats(midiClip->duration);

        updateActiveClips();
        LOG("Deleted MIDI note, new durationBeats: " << midiClip->durationBeats);
    }
    else {
        LOG_ERROR("Clip is not a MidiClip");
    }
}

void Engine::Core::setBPM(double newBPM) {
    if (newBPM > 0.0) {
        bpm = newBPM;
        LOG_SUCCESS("BPM updated to: " << bpm);
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

double Engine::Core::secondsToBeats(double seconds, double bpm) const {
    return seconds * (bpm / 60.0);
}

double Engine::Core::beatsToSeconds(double beats, double bpm) const {
    return beats * (60.0 / bpm);
}

double Engine::Core::secondsToBeats(double seconds) const {
    return secondsToBeats(seconds, bpm);
}

double Engine::Core::beatsToSeconds(double beats) const {
    return beatsToSeconds(beats, bpm);
}

double Engine::Core::secondsToMeasures(double seconds) const {
    double beats = secondsToBeats(seconds);
    return beats / timeSignatureNumerator;
}

double Engine::Core::measuresToSeconds(double measures) const {
    double beats = measures * timeSignatureNumerator;
    return beatsToSeconds(beats);
}

void Engine::Core::setMasterGain(float gain) {
    const juce::ScopedLock sl(lock);
    masterGain = juce::jlimit(0.0f, 2.0f, gain); // Ограничиваем диапазон
    LOG_SUCCESS("Master gain set to: " << masterGain);
}

float Engine::Core::getMasterGain() const {
    const juce::ScopedLock sl(lock);
    LOG_INFO("Get User Volume:" << userVolume);
    return masterGain;
}

void Engine::Core::setUserVolume(float volume) {
    const juce::ScopedLock sl(lock);
    LOG_SUCCESS("Set User Volume:" << volume);
    userVolume = juce::jlimit(0.0f, 2.0f, volume); // Ограничиваем диапазон
    LOG("User volume set to: " << userVolume);
}

float Engine::Core::getUserVolume() const {
    const juce::ScopedLock sl(lock);
    LOG_INFO("Get User Volume:" << userVolume);
    return userVolume;
}

#pragma endregion