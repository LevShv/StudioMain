
#include "engine.h"

// Core implementation

#pragma region Core

Engine::Core::Core() {
    formatManager.registerBasicFormats();

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
    juce::File file("C:\\Users\\llvvv\\source\\repos\\Studio\\StudioMain\\Step5");
    track.isMidiTrack = false;
    loadAudioClip(2, file, 2, 1);

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

    for (auto& track : tracks) {
        if (track.isMidiTrack) continue;

        for (auto& clip : track.clips) {
            if (auto* audioClip = dynamic_cast<AudioClip*>(clip.get())) {
                if (audioClip->useRAM) {
                    loadClipToRAM(*audioClip);
                }
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

    // Время в ударах
    const double startBeats = positionInBeats;
    const double blockDurationBeats = secondsToBeats(blockDuration);
    const double endBeats = startBeats + blockDurationBeats;

    // Очистка буфера ПЕРЕД заполнением
    info.clearActiveBufferRegion();

    /*const double startTime = position;
    const double blockDuration = info.numSamples / sampleRate;
    const double endTime = startTime + blockDuration;*/

    // Обработка аудио клипов
    for (auto& active : activeClips) {
        if (auto* audioClip = dynamic_cast<const AudioClip*>(active.clip)) {
            if (audioClip->useRAM) {
                const int startSample = static_cast<int>((startTime - audioClip->startTime) * sampleRate);
                const int numSamples = juce::jmin(
                    info.numSamples,
                    audioClip->buffer.getNumSamples() - startSample
                );

                if (startSample >= 0 && numSamples > 0) {
                    for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel) {
                        info.buffer->addFrom(
                            channel,
                            info.startSample,
                            audioClip->buffer,
                            channel % audioClip->buffer.getNumChannels(),
                            startSample,
                            numSamples,
                            active.track->gain * audioClip->gain
                        );
                    }
                }
            }
            else if (active.source != nullptr) {
                active.source->getNextAudioBlock(info);
            }
        }
    }

    // Обработка MIDI
    processMidiBlocks(info, startTime, endTime);

    // Корректное обновление позиции
    /*position = blockDuration;*/



    updateActiveClips();

    position += blockDuration; // Обновляем ПОСЛЕ
    positionInBeats += blockDurationBeats;
 
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
                }
            }
        }
    }

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

void Engine::Core::moveClip(int trackIndex, int clipIndex, double startBeats) {
    if (trackIndex >= 0 && trackIndex < tracks.size() &&
        clipIndex >= 0 && clipIndex < tracks.at(trackIndex).clips.size()) {
        const juce::ScopedLock sl(lock);
        tracks.at(trackIndex).clips[clipIndex]->startTime = beatsToSeconds(startBeats);
        tracks.at(trackIndex).clips[clipIndex]->startBeats = startBeats;
        updateActiveClips();
    }
}

void Engine::Core::updateActiveClips() {
    activeClips.clear();

	//LOG(position);

    for (auto& track : tracks) {
        if (track.muted) continue;

        for (auto& clip : track.clips) {
            if (clip->isActive(position)) {
                ActiveClip active;
                active.clip = clip.get();
                active.track = &track;

                if (auto* audioClip = dynamic_cast<AudioClip*>(clip.get())) {
                    if (!audioClip->useRAM) { // Только для клипов не в RAM
                        if (auto reader = formatManager.createReaderFor(audioClip->file)) {
                            auto readerPtr = std::unique_ptr<juce::AudioFormatReader>(reader);
                            active.source = std::make_unique<juce::AudioFormatReaderSource>(
                                readerPtr.release(), true);
                            active.source->prepareToPlay(512, sampleRate);
                            active.source->setNextReadPosition(
                                static_cast<juce::int64>((position - audioClip->startTime) * sampleRate));
                        }
                    }
                }
                activeClips.add(std::move(active));
            }
        }
    }
    LOG("Active clips updated at position: " << position << " seconds (" << positionInBeats << " beats)");
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

    // 7. Настройка MIDI
    configureMidiDevices();
    
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

#pragma endregion