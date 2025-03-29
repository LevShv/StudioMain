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

    for (auto& track : tracks)
        for (auto& clip : track.clips)
            if (clip.useRAM)
                loadClipToRAM(clip);
}

void Engine::Core::releaseResources() {
    activeClips.clear();
}

void Engine::Core::getNextAudioBlock(const juce::AudioSourceChannelInfo& info) {
    if (!transportPlaying) {
        info.clearActiveBufferRegion();
        return;
    }

    const juce::ScopedLock sl(lock);
    info.clearActiveBufferRegion();

    // Обработка активных клипов...
    // (остальная часть метода остается без изменений)

    position += info.numSamples / sampleRate;
    updateActiveClips();
}

void Engine::Core::play() {
    const juce::ScopedLock sl(lock);
    playing = true;
    updateActiveClips();
}

void Engine::Core::stop() {
    const juce::ScopedLock sl(lock);
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
    audioSourcePlayer.setSource(&core);
    deviceManager.initialiseWithDefaultDevices(0, 2);
    deviceManager.addAudioCallback(&audioSourcePlayer);
}

Engine::~Engine()
{
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
}

void Engine::AddClip(int trackInd, std::string path, int startTime, bool loadToRAM)
{
    juce::File audioFile(path);
    core.loadClip(trackInd, audioFile, startTime, loadToRAM);
}

void Engine::StopMix() { core.stop(); }

void Engine::PlayMix() { core.play(); }

void Engine::MoveClip(int trackIndex, int clipIndex, double newStartTime) { core.moveClip(trackIndex, clipIndex, newStartTime); }

void Engine::SetPlayheadPosition(double position) { core.setPosition(position); }

bool Engine::IsPlaying() { return core.isPlaying(); };