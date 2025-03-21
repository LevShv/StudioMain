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
                ClipStreamer newStreamer;
                newStreamer.volume = clip->volume * track.volume;
                newStreamer.globalStartTime = clip->startTime;

                if (clip->loadToRAM && !clip->samples.empty()) {
                    // Используем данные из RAM
                    newStreamer.ramSamples = &clip->samples;
                    newStreamer.position = static_cast<sf_count_t>(
                        (currentTime - clip->startTime + clip->offset) * SAMPLE_RATE
                        );
                }
                else {
                    // Используем потоковое чтение с диска
                    newStreamer.file = SndfileHandle(clip->path);
                    newStreamer.position = static_cast<sf_count_t>(
                        (currentTime - clip->startTime + clip->offset) * SAMPLE_RATE
                        );
                    newStreamer.file.seek(newStreamer.position, SEEK_SET);
                }

                newStreamer.isActive = true;
                activeStreamers.emplace(clipId, std::move(newStreamer));
            }
            allClipsFinished = false; // Есть активные клипы
        }
    }

    // Если все клипы завершены, останавливаем воспроизведение
    if (allClipsFinished && activeStreamers.empty()) {
        StopPlayback();
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

// Загружает аудиоданные в RAM, если это указано в клипе
bool Engine::FileManager::LoadAudioData(AudioClip& clip) {
    if (!clip.loadToRAM) return true; // Пропускаем, если загрузка в RAM не требуется

    SndfileHandle file(clip.path);
    if (file.error()) return false; // Ошибка загрузки файла

    clip.samples.resize(file.frames() * file.channels());
    file.read(clip.samples.data(), clip.samples.size());

    return true;
}

// Test Implementation
void Engine::TestPlay() {

    Core core;
    FileManager fileManager;

    // Создаем дорожку с двумя аудиоклипами
    Track Track1;
    Track1.clips.push_back({
        "Misc/Step5.wav", // Путь
        0.0,              // Начало на дорожке
        0.0,              // Смещение в файле
        5.0,              // Длительность
        0.8f              // Громкость
        });
    Track1.clips.push_back({
        "Misc/choose.wav",
        2.0,              // Начинается через 5 секунд
        0.0,              // Смещение в файле
        5.0,              // Длительность
        0.8f              // Громкость
        });

    tracks.push_back(Track1);

    Track Track2;
    AudioClip drumClip1;
    drumClip1.path = "Misc/Happy.wav";
    drumClip1.startTime = 0.0;
    drumClip1.duration = 5.0;
    drumClip1.loadToRAM = true; // Загружаем в RAM
    fileManager.LoadAudioData(drumClip1); // Загружаем данные

    AudioClip drumClip2;
    drumClip2.path = "Misc/Village party.wav";
    drumClip2.startTime = 15.0;
    drumClip2.duration = 5.0;
    drumClip2.loadToRAM = false; // Читаем с диска

    Track2.clips.push_back(drumClip1);
    Track2.clips.push_back(drumClip2);

    tracks.push_back(Track2);


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