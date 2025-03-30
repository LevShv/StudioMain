#include "engine.h"

Engine::Core::Core() {
    formatManager.registerBasicFormats();
    tracks.add(Track());
    audioSourcePlayer.setSource(this);  // Устанавливаем себя как источник аудио


}

Engine::Core::~Core() {
    stop();
    audioSourcePlayer.setSource(nullptr); // Важно для корректного освобождения ресурсов
}

void Engine::Core::startAudio(juce::AudioDeviceManager& deviceManager) {
    deviceManager.addAudioCallback(&audioSourcePlayer); // Регистрируем audioSourcePlayer
}

void Engine::Core::stopAudio(juce::AudioDeviceManager& deviceManager) {
    deviceManager.removeAudioCallback(&audioSourcePlayer);
}

void Engine::Core::prepareToPlay(int samplesPerBlock, double newSampleRate) {
    sampleRate = newSampleRate;
    transportPlaying = false; // Инициализируем состояние

    for (auto& track : tracks)
        for (auto& clip : track.clips)
            if (clip.useRAM)
                loadClipToRAM(clip);
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

    // Очищаем буфер
    for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
        info.buffer->clear(channel, info.startSample, info.numSamples);
    }

    // Обрабатываем активные клипы
    for (auto& active : activeClips) {
        if (active.clip->useRAM) {
            // Воспроизведение из RAM
            const int startSample = static_cast<int>(active.position);
            const int numSamples = juce::jmin(info.numSamples,
                active.clip->buffer.getNumSamples() - startSample);

            for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
                info.buffer->addFrom(channel, info.startSample,
                    active.clip->buffer,
                    channel % active.clip->buffer.getNumChannels(),
                    startSample,
                    numSamples);
            }
            active.position += numSamples;
        }
        else if (active.source != nullptr) {
            // Воспроизведение из файла
            active.source->getNextAudioBlock(info);
        }
    }

    position += info.numSamples / sampleRate;
    updateActiveClips();
    
}

void Engine::Core::play() {
    const juce::ScopedLock sl(lock);
    transportPlaying = true;
    playing = true;
    updateActiveClips();
}

void Engine::Core::stop() {
    const juce::ScopedLock sl(lock);
    transportPlaying = false;
    playing = false;
    activeClips.clear();
}

void Engine::Core::setPosition(double newPosition) {
    const juce::ScopedLock sl(lock);
    position = newPosition;
    updateActiveClips();
}

void Engine::Core::loadClip(int trackIndex, const juce::File& file, double startTime, bool loadToRAM) {
    if (trackIndex >= 0 && trackIndex < tracks.size()) {
        Clip newClip;
        newClip.file = file;
        newClip.startTime = startTime;
        newClip.useRAM = loadToRAM;

        if (auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file))) {
            newClip.duration = reader->lengthInSamples / reader->sampleRate;
            if (loadToRAM) {
                newClip.buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
                reader->read(&newClip.buffer, 0, (int)reader->lengthInSamples, 0, true, true);
                LOG_SUCCESS("Loaded to RAM " << file.getFileName());
            }
            else {
                LOG_SUCCESS("File reader set as a clip " << file.getFileName())
            }
        }

        tracks.getReference(trackIndex).clips.add(newClip);
    }
}

void Engine::Core::moveClip(int trackIndex, int clipIndex, double newStartTime) {
    if (trackIndex >= 0 && trackIndex < tracks.size() &&
        clipIndex >= 0 && clipIndex < tracks.getReference(trackIndex).clips.size()) {
        const juce::ScopedLock sl(lock);
        tracks.getReference(trackIndex).clips.getReference(clipIndex).startTime = newStartTime;
        updateActiveClips();
    }
}

void Engine::Core::updateActiveClips() {
    activeClips.clear();

    for (int i = 0; i < tracks.size(); ++i) {
        auto& track = tracks.getReference(i);
        if (track.muted) continue;

        for (auto& clip : track.clips) {
            if (clip.isActive(position)) {
                ActiveClip active;
                active.clip = &clip;
                active.track = &track;  // Устанавливаем ссылку на трек

                if (!clip.useRAM) {
                    if (auto reader = std::unique_ptr<juce::AudioFormatReader>(
                        formatManager.createReaderFor(clip.file))) {

                        active.source = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
                        active.source->prepareToPlay(512, sampleRate);
                        active.source->setNextReadPosition(static_cast<juce::int64>((position - clip.startTime) * sampleRate));
                    }
                }
                else {
                    active.position = static_cast<juce::int64>((position - clip.startTime) * sampleRate);
                }

                activeClips.add(std::move(active));
            }
        }
    }
}

void Engine::Core::loadClipToRAM(Clip& clip) {
    if (auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(clip.file))) {
        clip.buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&clip.buffer, 0, (int)reader->lengthInSamples, 0, true, true);
    }
}

Engine::Engine()
{
    //// Логируем список доступных устройств
    //auto midiInputs = juce::MidiInput::getAvailableDevices();
    //LOG_INFO("Available MIDI Inputs:");
    //for (const auto& input : midiInputs) {
    //    LOG_INFO(" - " << input.name << " (ID: " << input.identifier << ")");
    //}

    //// Безопасно отключаем устройства
    //for (const auto& input : midiInputs) {
    //    if (deviceManager.isMidiInputDeviceEnabled(input.identifier)) {
    //        deviceManager.setMidiInputDeviceEnabled(input.identifier, false);
    //    }
    //}

    //// Отключаем MIDI-выход
    //deviceManager.setDefaultMidiOutputDevice(juce::String());

    // Настраиваем аудио
    audioSourcePlayer.setSource(&core);
    deviceManager.addAudioCallback(&audioSourcePlayer);

    configureMidiDevices();
    
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.initialise(2, 2, nullptr, true); // 2 in/out channels
    deviceManager.addAudioCallback(&audioSourcePlayer);

    // Получаем текущие настройки и устанавливаем sample rate
    deviceManager.getAudioDeviceSetup(setup);
    setup.sampleRate = 44100.0; // или ваш предпочтительный sample rate
    deviceManager.setAudioDeviceSetup(setup, true);

}

Engine::~Engine()
{
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
   // deviceManager.removeMidiInputDeviceCallback(); // Добавьте эту строку

}

void Engine::AddClip(int trackInd, std::string path, int startTime, bool loadToRAM)
{
    juce::File audioFile;

    try {



        // 1. Создаем нормализованный путь
        juce::String normalizedPath = juce::String(path)
            .replace("\\", "/")          // Унифицируем разделители
            .replace("//", "/");         // Убираем дублирующие слеши

        // 2. Создаем файловый объект с проверкой
        juce::File audioFile2(normalizedPath);

        // 3. Дополнительные проверки
        if (normalizedPath.isEmpty()) {
            LOG_ERROR("Error: Empty path provided");
            return;
        }

        LOG_INFO("File path: " << audioFile.getFullPathName());

        audioFile = audioFile2;

    }
    catch (const std::exception& e) {
        LOG_ERROR("Critical error: " << e.what());
    }

    // Проверка существования файла
    if (!audioFile.exists()) {
        // Файл не существует
        LOG_ERROR("File Does not exist " << audioFile.getFullPathName());
        return;
    }

    // Проверка, что это именно файл (а не директория)
    if (!audioFile.existsAsFile()) {
        LOG_ERROR("This is directory, not a file " << audioFile.getFullPathName());
        return;
    }

    if (!audioFile.hasReadAccess()) {
        LOG_ERROR("No access to read " << path);
        return;
    }
    core.loadClip(trackInd, audioFile, startTime, loadToRAM);
}

void Engine::StopMix() { core.stop(); }

void Engine::PlayMix() { core.play(); }

void Engine::MoveClip(int trackIndex, int clipIndex, double newStartTime) { core.moveClip(trackIndex, clipIndex, newStartTime); }

void Engine::SetPlayheadPosition(double position) { core.setPosition(position); }

bool Engine::IsPlaying() { juce::ScopedLock sl(core.lock); return core.isPlaying(); }

void Engine::configureMidiDevices()
{
    auto midiInputs = juce::MidiInput::getAvailableDevices();

    if (!midiInputs.isEmpty()) {
        for (const auto& input : midiInputs) {
            deviceManager.setMidiInputDeviceEnabled(input.identifier, false);
        }
        deviceManager.setDefaultMidiOutputDevice({});
    }
    else {
        LOG_WARN("No MIDI devices to configure.");
    }
}
