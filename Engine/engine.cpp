//#include "engine.h"
//#include <cmath>
//
//// Core Implementation
//Engine::Core::Core() : isPlaying(false), playheadPosition(0.0) {
//    PaError err = Pa_Initialize();
//    if (err != paNoError) {
//        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
//    }
//    else {
//        std::cout << "PortAudio initialized successfully!" << std::endl;
//    }
//}
//
//
//Engine::Core::~Core() {
//    StopPlayback();
//    Pa_Terminate();
//}
//
//int Engine::Core::AudioCallback(const void* inputBuffer, void* outputBuffer,
//    unsigned long framesPerBuffer,
//    const PaStreamCallbackTimeInfo* timeInfo,
//    PaStreamCallbackFlags statusFlags,
//    void* userData) {
//    Core* core = static_cast<Core*>(userData);
//    float* out = static_cast<float*>(outputBuffer);
//    std::memset(out, 0, framesPerBuffer * sizeof(float)); // Очистка буфера
//
//    if (!core->isPlaying) return paContinue;
//
//    // Блокировка для безопасного доступа к стримерам
//    std::lock_guard<std::mutex> lock(core->streamersMutex);
//
//    // Микширование активных стримеров
//    for (auto& [id, streamer] : core->activeStreamers) {
//        if (!streamer.isActive) continue;
//
//        std::vector<float> buffer(framesPerBuffer);
//        sf_count_t readCount = 0;
//
//        if (streamer.ramSamples) {
//            // Используем данные из RAM
//            readCount = std::min<sf_count_t>(
//                framesPerBuffer,
//                streamer.ramSamples->size() - streamer.position
//            );
//            std::memcpy(buffer.data(), streamer.ramSamples->data() + streamer.position,
//                readCount * sizeof(float));
//        }
//        else {
//            // Используем потоковое чтение с диска
//            readCount = streamer.file.read(buffer.data(), framesPerBuffer);
//        }
//
//        for (unsigned long i = 0; i < framesPerBuffer; ++i) {
//            if (i < static_cast<unsigned long>(readCount)) {
//                out[i] += buffer[i] * streamer.volume;
//            }
//        }
//
//        streamer.position += readCount;
//        if (readCount < framesPerBuffer) {
//            streamer.isActive = false; // Клип закончился
//        }
//    }
//
//    // Обновление позиции воспроизведения
//    core->playheadPosition.store(
//        core->playheadPosition.load() +
//        static_cast<double>(framesPerBuffer) / Core::SAMPLE_RATE
//    );
//
//    return paContinue;
//}
//void Engine::Core::StartPlayback() {
//    std::lock_guard<std::mutex> lock(streamersMutex);
//
//    if (!audioStream) {
//        PaError err = Pa_OpenDefaultStream(&audioStream, 0, 1, paFloat32,
//            SAMPLE_RATE, FRAMES_PER_BUFFER,
//            AudioCallback, this);
//        if (err != paNoError) {
//            std::cerr << "PortAudio stream opening failed: " << Pa_GetErrorText(err) << std::endl;
//            return;
//        }
//        else {
//            std::cout << "PortAudio stream opened successfully!" << std::endl;
//        }
//    }
//
//    if (!Pa_IsStreamActive(audioStream)) {
//        PaError err = Pa_StartStream(audioStream);
//        if (err != paNoError) {
//            std::cerr << "PortAudio stream start failed: " << Pa_GetErrorText(err) << std::endl;
//            return;
//        }
//        else {
//            std::cout << "PortAudio stream started successfully!" << std::endl;
//        }
//        isPlaying = true;
//    }
//}
//
//void Engine::Core::StopPlayback() {
//    if (audioStream) {
//        Pa_StopStream(audioStream);
//        Pa_CloseStream(audioStream);
//        audioStream = nullptr;
//    }
//    isPlaying = false;
//    playheadPosition.store(0.0);
//}
//
//void Engine::Core::TogglePause() {
//    isPlaying = !isPlaying;
//}
//
//void Engine::Core::TogglePlayback() {
//    if (isPlaying) {
//        // Если воспроизведение активно, ставим на паузу
//        Pa_StopStream(audioStream);
//        isPlaying = false;
//    }
//    else {
//        // Если воспроизведение на паузе, возобновляем
//        if (!audioStream) {
//            // Если поток не создан, создаем его
//            Pa_OpenDefaultStream(&audioStream, 0, 1, paFloat32,
//                SAMPLE_RATE, FRAMES_PER_BUFFER,
//                AudioCallback, this);
//        }
//        Pa_StartStream(audioStream);
//        isPlaying = true;
//    }
//}
//
//void Engine::Core::UpdateStreamers(const std::vector<Track>& tracks) {
//
//    std::lock_guard<std::mutex> lock(streamersMutex);
//    const double currentTime = playheadPosition.load();
//
//    bool allClipsFinished = true;
//
//    // Очистка неактивных стримеров
//    for (auto it = activeStreamers.begin(); it != activeStreamers.end();) {
//        if (!it->second.isActive) {
//            it = activeStreamers.erase(it);
//        }
//        else {
//            ++it;
//        }
//    }
//
//    // Добавление новых стримеров для активных клипов
//    for (size_t trackIdx = 0; trackIdx < tracks.size(); ++trackIdx) {
//        const auto& track = tracks[trackIdx];
//        if (track.isMuted) continue;
//
//        for (const auto* clip : track.GetActiveClips(currentTime)) {
//            size_t clipId = reinterpret_cast<size_t>(clip); // Уникальный ID клипа
//
//            if (!activeStreamers.count(clipId)) {
//                ClipStreamer newStreamer;
//                newStreamer.volume = clip->volume * track.volume;
//                newStreamer.globalStartTime = clip->startTime;
//
//                if (clip->loadToRAM && !clip->samples.empty()) {
//                    // Используем данные из RAM
//                    newStreamer.ramSamples = &clip->samples;
//                    newStreamer.position = static_cast<sf_count_t>(
//                        (currentTime - clip->startTime + clip->offset) * SAMPLE_RATE
//                        );
//                }
//                else {
//                    // Используем потоковое чтение с диска
//                    newStreamer.file = SndfileHandle(clip->path);
//                    newStreamer.position = static_cast<sf_count_t>(
//                        (currentTime - clip->startTime + clip->offset) * SAMPLE_RATE
//                        );
//                    newStreamer.file.seek(newStreamer.position, SEEK_SET);
//                }
//
//                newStreamer.isActive = true;
//                activeStreamers.emplace(clipId, std::move(newStreamer));
//            }
//            allClipsFinished = false; // Есть активные клипы
//        }
//    }
//
//    // Если все клипы завершены, останавливаем воспроизведение
//    if (allClipsFinished && activeStreamers.empty()) {
//        StopPlayback();
//    }
//
//    // Если все клипы завершены, останавливаем воспроизведение
//    if (allClipsFinished && activeStreamers.empty()) {
//        StopPlayback();
//    }
//}
//
//// FileManager Implementation
//bool Engine::FileManager::ValidateAudioFile(const std::string& path) {
//    SndfileHandle file(path);
//    return file.error() == SF_ERR_NO_ERROR;
//}
//
//// Загружает аудиоданные в RAM, если это указано в клипе
//bool Engine::FileManager::LoadAudioData(AudioClip& clip) {
//    if (!clip.loadToRAM) return true; // Пропускаем, если загрузка в RAM не требуется
//
//    SndfileHandle file(clip.path);
//    if (file.error()) return false; // Ошибка загрузки файла
//
//    clip.samples.resize(file.frames() * file.channels());
//    file.read(clip.samples.data(), clip.samples.size());
//
//    return true;
//}
//
//// Test Implementation
//void Engine::TestPlay() {
//
//
//
//    // Создаем дорожку с двумя аудиоклипами
//    Track Track1;
//    Track1.clips.push_back({
//        "Misc/Step5.wav", // Путь
//        0.0,              // Начало на дорожке
//        0.0,              // Смещение в файле
//        5.0,              // Длительность
//        0.8f              // Громкость
//        });
//    Track1.clips.push_back({
//        "Misc/choose.wav",
//        2.0,              // Начинается через 5 секунд
//        0.0,              // Смещение в файле
//        5.0,              // Длительность
//        0.8f              // Громкость
//        });
//
//    tracks.push_back(Track1);
//
//    Track Track2;
//    AudioClip drumClip1;
//    drumClip1.path = "Misc/Happy.wav";
//    drumClip1.startTime = 0.0;
//    drumClip1.duration = 5.0;
//    drumClip1.loadToRAM = true; // Загружаем в RAM
//    fileManager.LoadAudioData(drumClip1); // Загружаем данные
//
//    AudioClip drumClip2;
//    drumClip2.path = "Misc/Village party.wav";
//    drumClip2.startTime = 15.0;
//    drumClip2.duration = 5.0;
//    drumClip2.loadToRAM = false; // Читаем с диска
//
//    Track2.clips.push_back(drumClip1);
//    Track2.clips.push_back(drumClip2);
//
//    tracks.push_back(Track2);
//
//
//    // Запуск воспроизведения
//    core.TogglePlayback();
//
//    // Основной цикл обновления
//    while (true) {
//        core.UpdateStreamers(tracks);
//
//        if (!core.isPlaying) {
//            break; // Воспроизведение завершено
//        }
//
//        std::this_thread::sleep_for(std::chrono::milliseconds(10));
//    }
//
//    std::cout << "Playback finished!" << std::endl;
//}
//
//void Engine::LoadToTrack(std::string path, double StartTime, int mode)
//{
//    Track Track1;
//    AudioClip Clip;
//
//    switch (mode)
//    {
//    case 1:
//
//        Clip.path = path, // Путь
//            Clip.startTime = StartTime; // Начало на дорожке
//        Clip.duration = 5.0; // Длительность
//        Clip.loadToRAM = false; // Читаем с диска
//        fileManager.LoadAudioData(Clip); // Загружаем данные
//        break;
//
//    case 2:
//
//        Track1.clips.push_back({
//        path, // Путь
//        StartTime,              // Начало на дорожке
//        0.0,              // Смещение в файле
//        5.0,              // Длительность
//        0.8f              // Громкость
//            });
//        break;
//    }
//
//    tracks.push_back(Track1);
//}
//
////void Engine::StartStopAlltracks()
////{
////    core.TogglePlayback();
////
////    isPlaybackThreadRunning = true;
////
////    // Запуск воспроизведения в отдельном потоке
////    std::thread playbackThread([this]() {
////        core.TogglePlayback();
////
////        while (isPlaybackThreadRunning) {
////            core.UpdateStreamers(tracks);
////
////            if (!core.isPlaying) {
////                break; // Воспроизведение завершено
////            }
////
////            std::this_thread::sleep_for(std::chrono::milliseconds(10));
////        }
////
////        isPlaybackThreadRunning = false;
////        std::cout << "Playback finished!" << std::endl;
////        });
////
////    playbackThread.detach();
////}
//void Engine::StartStopAlltracks() {
//    if (core.isPlaying) {
//        core.StopPlayback();
//    }
//    else {
//        core.StartPlayback();
//    }
//}
//
//bool Engine::isPlaying() const {
//    std::lock_guard<std::mutex> lock(m_mutex); // Защищаем доступ к состоянию
//    return m_isPlaying.load();
//}
//
//void Engine::StartPlayback() {
//    std::lock_guard<std::mutex> lock(m_mutex);
//    core.StartPlayback();
//    m_isPlaying = true;
//}
//
//void Engine::StopPlayback() {
//    std::lock_guard<std::mutex> lock(m_mutex);
//    core.StopPlayback();
//    m_isPlaying = false;
//}
//
//
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

    // Блокировка для безопасного доступа к стримерам
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

        for (unsigned long i = 0; i < framesPerBuffer; ++i) {
            if (i < static_cast<unsigned long>(readCount)) {
                out[i] += buffer[i] * streamer.volume;
            }
        }

        streamer.position += readCount;
        if (readCount < framesPerBuffer) {
            streamer.isActive = false; // Клип закончился
        }
    }

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

// Обновление стримеров
void Engine::Core::UpdateStreamers(const std::vector<Track>& tracks) {

    std::lock_guard<std::mutex> lock(streamersMutex);
    const double currentTime = playheadPosition.load();

    bool allClipsFinished = true;

    // Очистка неактивных стримеров
    for (auto it = activeStreamers.begin(); it != activeStreamers.end();) {
        if (!it->second.isActive) {
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

            if (!activeStreamers.count(clipId)) {
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
    }

    // Если все клипы завершены, останавливаем воспроизведение
    if (allClipsFinished && activeStreamers.empty()) {
        StopPlayback();
        std::cout << "Playback finished!" << std::endl;
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