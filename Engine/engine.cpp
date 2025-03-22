
#include "engine.h"
#include <iostream>
#include <thread>
#include <chrono>

// Конструктор Core
Engine::Core::Core() : isPlaying(false), playheadPosition(0.0) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
    }
    else {
        std::cout << "PortAudio initialized successfully!" << std::endl;
    }
}

// Деструктор Core
Engine::Core::~Core() {
    StopPlayback();
    Pa_Terminate();
}

void Engine::Core::AddTrack(Track track)
{
	tracks.push_back(track);
}

void Engine::Core::AddClip(size_t trackIdx, AudioClip clip)
{
	tracks[trackIdx].clips.push_back(clip);
	tracks[trackIdx].isEmpty = false;
}

// Аудиоколлбэк для PortAudio
int Engine::Core::AudioCallback(
    const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {

    Core* core = static_cast<Core*>(userData);
    float* out = static_cast<float*>(outputBuffer);
    std::memset(out, 0, framesPerBuffer * sizeof(float)); // Очистка буфера

    if (!core->isPlaying) return paContinue;

    std::lock_guard<std::mutex> lock(core->streamersMutex);

    // Микширование активных стримеров
    for (auto& [id, streamer] : core->activeStreamers) {
        if (!streamer.isActive) continue;

        std::vector<float> buffer(framesPerBuffer);
        sf_count_t readCount = 0;

        if (streamer.ramSamples) {
            // Используем данные из RAM
            readCount = std::min<sf_count_t>(
                framesPerBuffer,
                streamer.ramSamples->size() - streamer.position
            );
            std::memcpy(buffer.data(), streamer.ramSamples->data() + streamer.position,
                readCount * sizeof(float));
        }
        else {
            // Используем потоковое чтение с диска
            readCount = streamer.file.read(buffer.data(), framesPerBuffer);
        }

        for (unsigned long i = 0; i < readCount; ++i) {
            out[i] += buffer[i] * streamer.volume;
        }

        streamer.position += readCount;

        // Если клип завершен, помечаем его как неактивный
        if (readCount < framesPerBuffer) {
            streamer.isActive = false; // Клип закончился
            std::cout << "Clip finished: " << id << std::endl;

            // Помечаем клип как завершенный
            auto* clip = reinterpret_cast<AudioClip*>(id);
            clip->isFinished = true;
        }
    }

    // Обновление позиции воспроизведения
    core->playheadPosition.store(
        core->playheadPosition.load() +
        static_cast<double>(framesPerBuffer) / SAMPLE_RATE
    );

    return paContinue;
}
// Запуск воспроизведения
void Engine::Core::StartPlayback() {
    std::lock_guard<std::mutex> lock(streamersMutex);

    if (!audioStream) {
        PaError err = Pa_OpenDefaultStream(&audioStream, 0, 1, paFloat32,
            SAMPLE_RATE, FRAMES_PER_BUFFER,
            AudioCallback, this);
        if (err != paNoError) {
            std::cerr << "PortAudio stream opening failed: " << Pa_GetErrorText(err) << std::endl;
            return;
        }
    }

    PaError err = Pa_StartStream(audioStream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream start failed: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    isPlaying = true;

    // Запуск потока для обновления стримеров
    std::thread([this]() {
        while (isPlaying) {
            UpdateStreamers(tracks);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        }).detach();
}

// Остановка воспроизведения
void Engine::Core::StopPlayback() {
    std::lock_guard<std::mutex> lock(streamersMutex);

    // Останавливаем поток воспроизведения, если он активен
    if (audioStream && Pa_IsStreamActive(audioStream)) {
        PaError err = Pa_StopStream(audioStream);
        if (err != paNoError) {
            std::cerr << "PortAudio stream stop failed: " << Pa_GetErrorText(err) << std::endl;
        }
        else {
            std::cout << "PortAudio stream stopped successfully!" << std::endl;
        }
    }

    // Закрываем поток, если он был открыт
    if (audioStream) {
        PaError err = Pa_CloseStream(audioStream);
        if (err != paNoError) {
            std::cerr << "PortAudio stream closing failed: " << Pa_GetErrorText(err) << std::endl;
        }
        else {
            std::cout << "PortAudio stream closed successfully!" << std::endl;
        }
        audioStream = nullptr; // Сбрасываем указатель на поток
    }

    // Останавливаем флаг воспроизведения
    isPlaying = false;
}

void Engine::Core::SetPlayheadPosition(double newPosition) {
    // Захватываем мьютекс для защиты общих ресурсов
    std::lock_guard<std::mutex> lock(streamersMutex);

    // Останавливаем воспроизведение, если оно активно
    bool wasPlaying = isPlaying.load();
    if (wasPlaying) {
        PaError err = Pa_StopStream(audioStream);
        if (err != paNoError) {
            std::cerr << "Failed to stop audio stream: " << Pa_GetErrorText(err) << std::endl;
            return;
        }
    }

    // Устанавливаем новую позицию плейхеда
    playheadPosition.store(newPosition);

    // Сбрасываем флаг isFinished для клипов, которые теперь активны
    for (auto& track : tracks) {
        if (track.isMuted || track.isEmpty) continue;

        for (auto& clip : track.clips) {
            if ((newPosition >= clip.startTime && newPosition < clip.startTime + clip.duration) || newPosition <= clip.startTime) {
                clip.isFinished = false; // Сбрасываем флаг завершения
            }
        }
    }

    // Очищаем активные стримеры
    activeStreamers.clear();

    // Создаем новые стримеры для активных клипов
    for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
        const auto& track = tracks[trackIdx];
        if (track.isMuted || track.isEmpty) continue;

        for (const auto* clip : track.GetActiveClips(newPosition)) {
            size_t clipId = reinterpret_cast<size_t>(clip);

            // Создаем новый стример
            ClipStreamer newStreamer;
            newStreamer.volume = clip->volume * track.volume;
            newStreamer.globalStartTime = clip->startTime;

            if (clip->loadToRAM && !clip->samples.empty()) {
                newStreamer.ramSamples = &clip->samples;
                newStreamer.position = static_cast<sf_count_t>(
                    (newPosition - clip->startTime) * SAMPLE_RATE
                    );
            }
            else {
                newStreamer.file = SndfileHandle(clip->path);
                newStreamer.position = static_cast<sf_count_t>(
                    (newPosition - clip->startTime) * SAMPLE_RATE
                    );
                newStreamer.file.seek(newStreamer.position, SEEK_SET);
            }

            newStreamer.isActive = true;
            activeStreamers.emplace(clipId, std::move(newStreamer));
            std::cout << "New streamer added for clip: " << clip->path << std::endl;
        }
    }

    // Возобновляем воспроизведение, если оно было активно
    if (wasPlaying) {
        PaError err = Pa_StartStream(audioStream);
        if (err != paNoError) {
            std::cerr << "Failed to start audio stream: " << Pa_GetErrorText(err) << std::endl;
            return;
        }
    }

    std::cout << "Playhead moved to: " << newPosition << std::endl;
}

void Engine::Core::UpdateStreamers(const std::vector<Track>& tracks) {
    std::lock_guard<std::mutex> lock(streamersMutex);
    const double currentTime = playheadPosition.load();
    std::cout << "Current playhead position: " << currentTime << std::endl;

    bool allClipsFinished = true;

    // Очистка неактивных стримеров
    for (auto it = activeStreamers.begin(); it != activeStreamers.end();) {
        if (!it->second.isActive) {
            std::cout << "Removing inactive streamer: " << it->first << std::endl;
            it = activeStreamers.erase(it);
        }
        else {
            ++it;
        }
    }

    // Добавление новых стримеров для активных клипов
    for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
        const auto& track = tracks[trackIdx];
        if (track.isMuted || track.isEmpty) continue;

        for (const auto* clip : track.GetActiveClips(currentTime)) {
            size_t clipId = reinterpret_cast<size_t>(clip);

            // Проверяем, существует ли уже стример для этого клипа
            if (activeStreamers.count(clipId) || clip->isFinished) {
                continue; // Стример уже существует или клип завершен
            }

            // Создаем новый стример только если его еще нет
            ClipStreamer newStreamer;
            newStreamer.volume = clip->volume * track.volume;
            newStreamer.globalStartTime = clip->startTime;

            if (clip->loadToRAM && !clip->samples.empty()) {
                newStreamer.ramSamples = &clip->samples;
                newStreamer.position = static_cast<sf_count_t>(
                    (currentTime - clip->startTime + clip->offset) * SAMPLE_RATE
                    );
            }
            else {
                newStreamer.file = SndfileHandle(clip->path);
                newStreamer.position = static_cast<sf_count_t>(
                    (currentTime - clip->startTime + clip->offset) * SAMPLE_RATE
                    );
                newStreamer.file.seek(newStreamer.position, SEEK_SET);
            }

            newStreamer.isActive = true;
            activeStreamers.emplace(clipId, std::move(newStreamer));
            std::cout << "New streamer added for clip: " << clip->path << std::endl;
        }
        allClipsFinished = false; // Есть активные клипы
    }

    std::cout << "Streamer count " << activeStreamers.size() << std::endl;

    // Если все клипы завершены, останавливаем воспроизведение
    if (allClipsFinished && activeStreamers.empty()) {
        std::cout << "All clips finished, stopping playback..." << std::endl;
        StopPlayback();
    }
}
// Загрузка аудиоданных
bool Engine::FileManager::LoadAudioData(AudioClip& clip) {
    if (!clip.loadToRAM) return true;

    SndfileHandle file(clip.path);
    if (file.error()) return false;

    clip.samples.resize(file.frames() * file.channels());
    file.read(clip.samples.data(), clip.samples.size());

    return true;
}

// Валидация аудиофайла
bool Engine::FileManager::ValidateAudioFile(const std::string& path) {
    SndfileHandle file(path);
    return file.error() == SF_ERR_NO_ERROR;
}

// Загрузка трека
void Engine::LoadToTrack(std::string path, double StartTime, int mode, int TrackNumber) {

    if (!FileManager::ValidateAudioFile(path)) {
        std::cerr << "Failed to load audio file: " << path << std::endl;
        return;
    }

	if (core.tracks.size() < TrackNumber)
	{
		std::cerr << "Track not found: " << TrackNumber << std::endl;
		return;
	}

    AudioClip Clip;

    switch (mode) {
    case 1:
        Clip.path = path;
        Clip.startTime = StartTime;
        Clip.duration = 5.0;
        Clip.loadToRAM = false;

        if (!FileManager::LoadAudioData(Clip)) {
            std::cerr << "Failed to load audio data: " << path << std::endl;
            return;
        }

        std::cout << "Audio file loaded: " << path << std::endl;
        break;

    case 2:
        Clip = AudioClip({
            path,
            StartTime,
            0.0,
            5.0,
            0.8f
            });
        std::cout << "Audio clip added to track: " << path << std::endl;
        break;
    }

	core.AddClip(TrackNumber, Clip);
}

// Запуск/остановка воспроизведения
void Engine::StartStopAlltracks() {
    if (core.isPlaying) {
        core.StopPlayback();
    }
    else {
        core.StartPlayback();
    }
}

bool Engine::isPlaying() const {
    std::lock_guard<std::mutex> lock(m_mutex); // Защищаем доступ к состоянию
    return m_isPlaying.load();
}

void Engine::StartPlayback() {
    std::lock_guard<std::mutex> lock(m_mutex); // Защищаем доступ к состоянию
    core.StartPlayback();
    m_isPlaying = true;
}

void Engine::StopPlayback() {
    std::lock_guard<std::mutex> lock(m_mutex); // Защищаем доступ к состоянию
    core.StopPlayback();
    m_isPlaying = false;
}

void Engine::SetPlayheadPosition(double position)
{
	core.SetPlayheadPosition(position);
}
