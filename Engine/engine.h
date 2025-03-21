#pragma once

#include <portaudio.h>
#include <sndfile.hh>
#include <iostream>
#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <unordered_map>

class Engine {
public:
    void TestPlay();

    struct AudioClip {

        std::string path;          // Путь к аудиофайлу
        double startTime = 0.0;    // Время начала клипа на дорожке (в секундах)
        double offset = 0.0;       // Смещение внутри аудиофайла (в секундах)
        double duration = 0.0;     // Длительность клипа (в секундах)
        float volume = 1.0f;       // Громкость клипа
        bool isMuted = false;      // Флаг отключения клипа
        std::vector<float> samples; // Аудиоданные (если загружены в RAM)
        bool loadToRAM = false;

        // Метод для проверки, активен ли клип в данный момент
        bool IsActive(double globalTime) const {
            return globalTime >= startTime && globalTime < startTime + duration;
        }
    };

    class Track {
    public:
        std::vector<AudioClip> clips; // Аудиоклипы на дорожке
        bool isMuted = false;         // Флаг отключения всей дорожки
        float volume = 1.0f;         // Громкость дорожки

        // Метод для получения активных клипов в данный момент
        std::vector<const AudioClip*> GetActiveClips(double globalTime) const {
            std::vector<const AudioClip*> activeClips;
            for (const auto& clip : clips) {
                if (clip.IsActive(globalTime)) {
                    activeClips.push_back(&clip);
                }
            }
            return activeClips;
        }
    };

    std::vector<Track> tracks;

    class Core {
    public:
        static const int SAMPLE_RATE = 44100;
        const int FRAMES_PER_BUFFER = 512;  // Уменьшили для уменьшения задержки

        std::atomic<bool> isPlaying{ false };
        std::atomic<double> playheadPosition{ 0.0 }; // Текущая позиция воспроизведения в секундах

        Core();
        ~Core();

        static int AudioCallback(const void* inputBuffer, void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

        void UpdateStreamers(const std::vector<Track>& tracks);

        void StartPlayback();
        void StopPlayback();
        void TogglePause();

        void TogglePlayback();

        

    private:

        struct ClipStreamer {
            SndfileHandle file;          // Аудиофайл (для потокового чтения)
            sf_count_t position = 0;     // Текущая позиция в файле (в сэмплах)
            bool isActive = false;        // Флаг активности
            float volume = 1.0f;          // Громкость
            double globalStartTime = 0.0; // Время начала в проекте (в секундах)
            const std::vector<float>* ramSamples = nullptr; // Указатель на данные в RAM
        };

        std::mutex streamersMutex;
        std::unordered_map<size_t, ClipStreamer> activeStreamers; // Активные стримеры
        PaStream* audioStream = nullptr;

    };

    class FileManager {
    public:
        static bool ValidateAudioFile(const std::string& path);
        static bool LoadAudioData(AudioClip& clip);
    };
};


