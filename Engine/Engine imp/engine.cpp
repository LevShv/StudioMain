#include <log.h>
#include "engine.h"
#include "JuceHeader.h"
#include <thread>
#include <mutex>
#include <algorithm>

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

void Engine::EnableLoopMode(int trackIndex, int clipIndex)
{
    core.enableLoopMode(trackIndex, clipIndex);
}

void Engine::DisableLoopMode()
{
    core.disableLoopMode();
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

void Engine::CreateNewProject()
{
    juce::ScopedLock sl(core.lock);

    // 1. Останавливаем воспроизведение
    core.stop();
    LOG("Playback stopped for new project creation");

    // 2. Очищаем все треки
    core.tracks.clear();
    LOG("All tracks cleared");

    // 3. Сбрасываем параметры движка
    core.position = 0.0;
    core.positionInBeats = 0.0;
    core.bpm = 120.0; // Дефолтный BPM
    core.loopModeEnabled = false;
    core.loopTrackIndex = -1;
    core.loopClipIndex = -1;
    core.loopStartTime = 0.0;
    core.loopDuration = 0.0;
   // core.activeClips.clear();
    {
        const juce::ScopedLock noteSl(core.noteLock);
        core.activeNotes.clear();
    }
    LOG("Engine parameters reset: position=0.0, bpm=120.0, loopModeEnabled=false");

    // 4. Очищаем MIDI-выход
    if (core.midiOutput) {
        for (int channel = 1; channel <= 16; ++channel) {
            core.midiOutput->sendMessageNow(juce::MidiMessage::allNotesOff(channel));
        }
        core.midiOutput.reset();
        LOG("MIDI output cleared and reset");
    }

    // 5. Сбрасываем плагины
    for (auto& track : core.tracks) {
        for (auto& pluginInstance : track.plugins) {
            if (pluginInstance->plugin) {
                pluginInstance->plugin->releaseResources();
                pluginInstance->bypass = false;
            }
        }
    }
    // Поскольку треки уже очищены, этот цикл не выполнится, но оставлен для полноты

    // 6. Инициализируем аудиоустройство
    juce::AudioIODevice* audioDevice = deviceManager.getCurrentAudioDevice();
    if (audioDevice) {
        const int bufferSize = audioDevice->getCurrentBufferSizeSamples();
        const double sampleRate = audioDevice->getCurrentSampleRate();
        core.prepareToPlay(bufferSize, sampleRate);
        LOG("Audio device reinitialized: bufferSize=" << bufferSize << ", sampleRate=" << sampleRate);
    }

    // 7. Добавляем один пустой аудиотрек по умолчанию
    int newTrackIndex = AddAudioTrack();
    LOG("Added default audio track at index " << newTrackIndex);

    // 8. Обновляем активные клипы
    core.updateActiveClips();
    LOG("New project created successfully");

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
    if (newBPM < 60.0 || newBPM > 200.0) {
        LOG_ERROR("Invalid BPM value: value " << newBPM << ", keeping current BPM: " << core.bpm); 
        return;
    }

    const juce::ScopedLock sl(core.lock); 
    double oldBPM = core.bpm;
    core.bpm = newBPM;
    LOG("BPM changed tofrom " << oldBPM << " to " << newBPM);

    // Пересчитываем startTime и duration клипов
    for (size_t trackIdx = 0; trackIdx < core.tracks.size(); ++trackIdx) {
        auto& track = core.tracks[trackIdx];
        for (size_t clipIdx = 0; clipIdx < track.clips.size(); ++clipIdx) {
            auto& clip = track.clips[clipIdx];
            // Пересчитываем startTime и duration, сохраняя startBeats и durationBeats
            clip->startTime = BeatsToSeconds(clip->startBeats);
            clip->duration = BeatsToSeconds(clip->durationBeats);
            LOG("Updated clip on track " << trackIdx << ", clip " << clipIdx
                << ": startTime=" << clip->startTime << " seconds, duration=" << clip->duration << " seconds");

            // Для MIDI-клипов обновляем midiSequence
            if (auto* midiClip = dynamic_cast<MidiClip*>(clip.get())) {
                juce::MidiMessageSequence newSequence;
                for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
                    auto* event = midiClip->midiSequence.getEventPointer(i);
                    double oldTimeSeconds = event->message.getTimeStamp();
                    // Пересчитываем время относительно начала клипа
                    double beats = core.secondsToBeats(oldTimeSeconds, oldBPM);
                    double newTimeSeconds = core.beatsToSeconds(beats);
                    juce::MidiMessage newMessage = event->message;
                    newMessage.setTimeStamp(newTimeSeconds);
                    newSequence.addEvent(newMessage);
                }
                midiClip->midiSequence = newSequence;
                LOG("Updated midiSequence for MIDI clip on track " << trackIdx << ", clip " << clipIdx);
            }
        }
    }

    // Обновляем позицию плейхеда
    double beats = core.secondsToBeats(core.position, oldBPM);
    core.position = core.beatsToSeconds(beats);
    core.positionInBeats = beats;
    LOG("Playhead position updated to " << core.position << " seconds (" << core.positionInBeats << " beats)");

    // Обновляем параметры цикла в PAT mode
    if (core.loopModeEnabled && core.loopTrackIndex >= 0 && core.loopClipIndex >= 0) {
        auto& clip = core.tracks[core.loopTrackIndex].clips[core.loopClipIndex];
        core.loopStartTime = clip->startTime;
        core.loopDuration = clip->duration;
        // Корректируем позицию плейхеда, если он вне нового цикла
        if (core.position < core.loopStartTime || core.position >= core.loopStartTime + core.loopDuration) {
            core.position = core.loopStartTime;
            core.positionInBeats = clip->startBeats;
            LOG("Playhead repositioned to loop start: " << core.position << " seconds");
        }
        LOG("Loop parameters updated: loopStartTime=" << core.loopStartTime << ", loopDuration=" << core.loopDuration);
    }

    core.updateActiveClips();
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

void Engine::UpdateMidiNote(int trackIndex, int clipIndex, int noteIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    juce::ScopedLock sl(core.lock);
    core.updateMidiNote(trackIndex, clipIndex, noteIndex, noteNumber, startBeats, durationBeats, velocity, channel);
}

void Engine::DeleteMidiNote(int trackIndex, int clipIndex, int noteIndex) {
    juce::ScopedLock sl(core.lock);
    core.deleteMidiNote(trackIndex, clipIndex, noteIndex);
}

double Engine::SecondsToBeats(double seconds) const
{
    return core.secondsToBeats(seconds);
}

double Engine::BeatsToSeconds(double beats) const
{
    return core.beatsToSeconds(beats);
}

void Engine::cleanMidiSequence(MidiClip* midiClip) {
    std::vector<int> eventsToDelete;
    std::map<std::pair<int, int>, std::pair<double, int>> noteOnTimes; // {channel, noteNumber} -> {time, index}

    // Собираем все noteOn с их индексами
    for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
        auto* event = midiClip->midiSequence.getEventPointer(i);
        if (event->message.isNoteOn()) {
            noteOnTimes[{event->message.getChannel(), event->message.getNoteNumber()}] = { event->message.getTimeStamp(), i };
        }
    }

    // Проверяем noteOff и помечаем те, у которых нет noteOn
    for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
        auto* event = midiClip->midiSequence.getEventPointer(i);
        if (event->message.isNoteOff()) {
            auto key = std::pair<int, int>{ event->message.getChannel(), event->message.getNoteNumber() };
            if (!noteOnTimes.count(key)) {
                LOG_WARN("Removing unmatched noteOff: noteNumber=" << event->message.getNoteNumber()
                    << ", channel=" << event->message.getChannel()
                    << ", time=" << event->message.getTimeStamp());
                eventsToDelete.push_back(i);
            }
        }
    }

    // Удаляем noteOff без noteOn в обратном порядке
    for (auto it = eventsToDelete.rbegin(); it != eventsToDelete.rend(); ++it) {
        midiClip->midiSequence.deleteEvent(*it, false);
    }

    // Проверяем noteOn без noteOff и добавляем noteOff
    for (const auto& [key, timeAndIndex] : noteOnTimes) {
        bool hasNoteOff = false;
        for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
            auto* event = midiClip->midiSequence.getEventPointer(i);
            if (event->message.isNoteOff() &&
                event->message.getChannel() == key.first &&
                event->message.getNoteNumber() == key.second &&
                event->message.getTimeStamp() > timeAndIndex.first) {
                hasNoteOff = true;
                break;
            }
        }
        if (!hasNoteOff) {
            LOG_WARN("Adding missing noteOff for noteOn: noteNumber=" << key.second
                << ", channel=" << key.first << ", time=" << timeAndIndex.first);
            midiClip->midiSequence.addEvent(
                juce::MidiMessage::noteOff(key.first, key.second),
                timeAndIndex.first + BeatsToSeconds(0.25) // Дефолтная длительность 0.25 beats
            );
        }
    }

    midiClip->midiSequence.updateMatchedPairs();
    LOG("After cleanMidiSequence, total events: " << midiClip->midiSequence.getNumEvents());
}

void Engine::PlayNote(int trackIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel)
{
    core.playNote(trackIndex, noteNumber, startBeats, durationBeats, velocity, channel);
}

#pragma endregion


