#include "engine.h"
#include <cmath>

// Core Implementation
Engine::Core::Core() {
    Pa_Initialize();
}

Engine::Core::~Core() {
    StopPlayback();
    Pa_Terminate();
}

int Engine::Core::AudioCallback(const void* inputBuffer, void* outputBuffer,
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
        sf_count_t readCount = streamer.file.read(buffer.data(), framesPerBuffer);

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

    // Обновление позиции воспроизведения
    core->playheadPosition.store(
        core->playheadPosition.load() +
        static_cast<double>(framesPerBuffer) / Core::SAMPLE_RATE
    );

    return paContinue;
}
void Engine::Core::StartPlayback() {
    if (!audioStream) {
        Pa_OpenDefaultStream(&audioStream, 0, 1, paFloat32,
            SAMPLE_RATE, FRAMES_PER_BUFFER,
            AudioCallback, this);
    }

    if (!Pa_IsStreamActive(audioStream)) {
        Pa_StartStream(audioStream);
        isPlaying = true;
    }
}

void Engine::Core::StopPlayback() {
    if (audioStream) {
        Pa_StopStream(audioStream);
        Pa_CloseStream(audioStream);
        audioStream = nullptr;
    }
    isPlaying = false;
    playheadPosition.store(0.0);
}

void Engine::Core::TogglePause() {
    isPlaying = !isPlaying;
}

void Engine::Core::TogglePlayback() {
    if (isPlaying) {
        StopPlayback();
    }
    else {
        StartPlayback();
    }
}

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
        if (track.isMuted) continue;

        for (const auto* clip : track.GetActiveClips(currentTime)) {
            size_t clipId = reinterpret_cast<size_t>(clip); // Уникальный ID клипа

            if (!activeStreamers.count(clipId)) {
                // Инициализация нового стримера
                ClipStreamer newStreamer;
                newStreamer.file = SndfileHandle(clip->path);
                newStreamer.position = static_cast<sf_count_t>(
                    (currentTime - clip->startTime + clip->offset) * SAMPLE_RATE
                    );
                newStreamer.file.seek(newStreamer.position, SEEK_SET);
                newStreamer.isActive = true;
                newStreamer.volume = clip->volume * track.volume;
                newStreamer.globalStartTime = clip->startTime;

                activeStreamers.emplace(clipId, std::move(newStreamer));
            }
            allClipsFinished = false; // Есть активные клипы
        }
    }

    // Если все клипы завершены, останавливаем воспроизведение
    if (allClipsFinished && activeStreamers.empty()) {
        StopPlayback();
    }
}

// FileManager Implementation
bool Engine::FileManager::ValidateAudioFile(const std::string& path) {
    SndfileHandle file(path);
    return file.error() == SF_ERR_NO_ERROR;
}

// Test Implementation
void Engine::TestPlay() {

    Core core;

    // Создаем дорожку с двумя аудиоклипами
    Track drumTrack;
    drumTrack.clips.push_back({
        "Misc/Step5.wav", // Путь
        0.0,              // Начало на дорожке
        0.0,              // Смещение в файле
        5.0,              // Длительность
        0.8f              // Громкость
        });
    drumTrack.clips.push_back({
        "Misc/choose.wav",
        2.0,              // Начинается через 5 секунд
        0.0,              // Смещение в файле
        5.0,              // Длительность
        0.8f              // Громкость
        });

    tracks.push_back(drumTrack);

    // Запуск воспроизведения
    core.TogglePlayback();

    // Основной цикл обновления
    while (true) {
        core.UpdateStreamers(tracks);

        if (!core.isPlaying) {
            break; // Воспроизведение завершено
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Playback finished!" << std::endl;
}