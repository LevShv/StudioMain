
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

// Добавление клипа на дорожку
void Engine::Core::AddClip(size_t trackIdx, AudioClip clip)
{
	tracks[trackIdx].clips.push_back(clip);
	tracks[trackIdx].isEmpty = false;
}

// Аудиоколлбэк для PortAudio
int Engine::Core::AudioCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    Core* core = static_cast<Core*>(userData);
    float* out = static_cast<float*>(outputBuffer);
    std::memset(out, 0, framesPerBuffer * sizeof(float)); // Очистка буфера

    if (!core->isPlaying) return paContinue;

    std::lock_guard<std::mutex> lock(core->streamersMutex);

    for (auto& [id, streamer] : core->activeStreamers) {
        if (!streamer.isActive) continue;

        std::vector<float> buffer(framesPerBuffer);
        sf_count_t readCount = 0;

        if (streamer.ramSamples) {
            readCount = std::min<sf_count_t>(
                framesPerBuffer,
                streamer.ramSamples->size() - streamer.position
            );
            std::memcpy(buffer.data(), streamer.ramSamples->data() + streamer.position,
                readCount * sizeof(float));
        }
        else {
            readCount = streamer.file.read(buffer.data(), framesPerBuffer);
        }

        for (unsigned long i = 0; i < framesPerBuffer; ++i) {
            if (i < static_cast<unsigned long>(readCount)) {
                out[i] += buffer[i] * streamer.volume;
            }
        }

        streamer.position += readCount;
        if (readCount < framesPerBuffer) {
            streamer.isActive = false; // Клип закончился
            std::cout << "Clip finished: " << id << std::endl;
        }
    }

    core->playheadPosition.store(
        core->playheadPosition.load() +
        static_cast<double>(framesPerBuffer) / Core::SAMPLE_RATE
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

// Установка позиции воспроизведения
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

// Перемещение клипа по дорожке
void Engine::Core::MoveClip(size_t trackIdx, size_t clipIdx, double newStartTime) {
    std::lock_guard<std::mutex> lock(streamersMutex); // Защищаем доступ к данным

    // Проверяем, существует ли трек и клип
    if (trackIdx >= tracks.size() || clipIdx >= tracks[trackIdx].clips.size()) {
        std::cerr << "Invalid track or clip index." << std::endl;
        return;
    }
        
       

    auto& clip = tracks[trackIdx].clips[clipIdx];

    // Сбрасываем флаг isFinished, если плейхед находится до нового времени начала или в пределах длительности
    if ((playheadPosition.load() >= newStartTime && playheadPosition.load() < newStartTime + clip.duration) || playheadPosition.load() <= newStartTime) {
        clip.isFinished = false; // Сбрасываем флаг завершения
        std::cout << "Clip " << clip.path << " isFinished reset to false." << std::endl;
    }

    // Обновляем время начала клипа
    clip.startTime = newStartTime;

    // Удаляем старый стример, если он существует
    size_t clipId = reinterpret_cast<size_t>(&clip);
    if (activeStreamers.count(clipId)) {
        activeStreamers.erase(clipId);
        std::cout << "Old streamer removed for clip: " << clip.path << std::endl;
    }

    // Если клип теперь активен, создаем новый стример
    if (playheadPosition.load() >= newStartTime && playheadPosition.load() < newStartTime + clip.duration) {
        ClipStreamer newStreamer;
        newStreamer.volume = clip.volume * tracks[trackIdx].volume;
        newStreamer.globalStartTime = newStartTime;

        if (clip.loadToRAM && !clip.samples.empty()) {
            newStreamer.ramSamples = &clip.samples;
            newStreamer.position = static_cast<sf_count_t>(
                (playheadPosition.load() - newStartTime) * SAMPLE_RATE
                );
        }
        else {
            newStreamer.file = SndfileHandle(clip.path);
            newStreamer.position = static_cast<sf_count_t>(
                (playheadPosition.load() - newStartTime) * SAMPLE_RATE
                );
            newStreamer.file.seek(newStreamer.position, SEEK_SET);
        }

        newStreamer.isActive = true;
        activeStreamers.emplace(clipId, std::move(newStreamer));
        std::cout << "New streamer added for moved clip: " << clip.path << std::endl;
    }

    std::cout << "Clip moved to new start time: " << newStartTime << std::endl;
}

// Обновление стримеров
void Engine::Core::UpdateStreamers(const std::vector<Track>& tracks) {
    std::lock_guard<std::mutex> lock(streamersMutex);
    const double currentTime = playheadPosition.load();
    std::cout << "Current playhead position: " << currentTime << std::endl;
    // Удаляем стримеры для клипов, которые больше не активны
    for (auto it = activeStreamers.begin(); it != activeStreamers.end();) {
        bool isClipActive = false;

        // Проверяем, активен ли клип в текущий момент времени
        for (const auto& track : tracks) {
            for (const auto& clip : track.clips) {
                if (clip.IsActive(currentTime) && reinterpret_cast<size_t>(&clip) == it->first) {
                    isClipActive = true;
                    break;
                }
            }
            if (isClipActive) break;
        }

        if (!isClipActive) {
            std::cout << "Removing inactive streamer: " << it->first << std::endl;
            it = activeStreamers.erase(it);
        } else {
            ++it;
        }
    }

    // Добавляем новые стримеры для активных клипов
    for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
        const auto& track = tracks[trackIdx];
        if (track.isMuted) continue;

        for (const auto& clip : track.clips) {
            if (clip.IsActive(currentTime)) {
                size_t clipId = reinterpret_cast<size_t>(&clip);

                if (!activeStreamers.count(clipId)) {
                    ClipStreamer newStreamer;
                    newStreamer.volume = clip.volume * track.volume;
                    newStreamer.globalStartTime = clip.startTime;

                    if (clip.loadToRAM && !clip.samples.empty()) {
                        // Используем данные из RAM
                        newStreamer.ramSamples = &clip.samples;
                        newStreamer.position = static_cast<sf_count_t>(
                            (currentTime - clip.startTime + clip.offset) * SAMPLE_RATE
                        );
                    } else {
                        // Используем потоковое чтение с диска
                        newStreamer.file = SndfileHandle(clip.path);
                        newStreamer.position = static_cast<sf_count_t>(
                            (currentTime - clip.startTime + clip.offset) * SAMPLE_RATE
                        );
                        newStreamer.file.seek(newStreamer.position, SEEK_SET);
                    }

                    newStreamer.isActive = true;
                    activeStreamers.emplace(clipId, std::move(newStreamer));
                    std::cout << "New streamer added for clip: " << clip.path << std::endl;
                }
            }
        }
    }
}

// Проверка валидности индекса клипа
bool Engine::Core::IsValidClipIndex(size_t trackIdx, size_t clipIdx) const 
{
    if(trackIdx < tracks.size() && clipIdx < tracks[trackIdx].clips.size()) return true;
    else {
        std::cerr << "Invalid track or clip index." << std::endl;
        return false;
    }
}

// *************************** FileManager ***************************

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


// **************************** обертка *****************************

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
            5,
            0.8f
            });
		Clip.CalculateDuration();
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

// Проверка состояния воспроизведения
bool Engine::isPlaying() const {
    std::lock_guard<std::mutex> lock(m_mutex); // Защищаем доступ к состоянию
    return m_isPlaying.load();
}

// Запуск воспроизведения
void Engine::StartPlayback() {
    std::lock_guard<std::mutex> lock(m_mutex); // Защищаем доступ к состоянию
    core.StartPlayback();
    m_isPlaying = true;
}

// Остановка воспроизведения
void Engine::StopPlayback() {
    std::lock_guard<std::mutex> lock(m_mutex); // Защищаем доступ к состоянию
    core.StopPlayback();
    m_isPlaying = false;
}

// Установка позиции воспроизведения
void Engine::SetPlayheadPosition(double position)
{
	core.SetPlayheadPosition(position);
}

// Перемещение клипа по дорожке
void Engine::MoveClip(size_t trackIdx, size_t clipIdx, double newStartTime)
{
	core.MoveClip(trackIdx, clipIdx, newStartTime);
}
