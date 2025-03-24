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
#include <RtMidi.h>

class Engine {
public:

    std::atomic<bool> isPlaybackThreadRunning{ false }; // Флаг для управления потоком

    void LoadToTrack(std::string path, double StartTime, int mode, int TrackNumber);
    void StartStopAlltracks();
    bool isPlaying() const;
    void StartPlayback();
    void StopPlayback();
	void SetPlayheadPosition(double position);
	void MoveClip(size_t trackIdx, size_t clipIdx, double newStartTime);

    struct AudioClip {

        std::string path;          // Путь к аудиофайлу
        double startTime = 0.0;    // Время начала клипа на дорожке (в секундах)
        double offset = 0.0;       // Смещение внутри аудиофайла (в секундах)
        double duration = 0.0;     // Длительность клипа (в секундах)
        float volume = 1.0f;       // Громкость клипа
        bool isMuted = false;      // Флаг отключения клипа
        std::vector<float> samples; // Аудиоданные (если загружены в RAM)
		bool loadToRAM = false;	// Флаг загрузки в RAM
        bool isFinished = false;   // Флаг завершения клипа

        bool CalculateDuration() {
            SndfileHandle file(path);
            if (file.error()) return false; // Ошибка загрузки файла

            // Длительность = (количество сэмплов) / (частота дискретизации)
            duration = static_cast<double>(file.frames()) / file.samplerate();
            return true;
        }
        // Метод для проверки, активен ли клип в данный момент
        bool IsActive(double globalTime) const {
            return globalTime >= startTime && globalTime < startTime + duration;
        }
    };

    class Track {
    public:

		bool isEmpty = true;          // Флаг пустоты дорожки
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

    

    class Core {
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
    public:

        std::vector<Track> tracks = { Track(), Track(), Track(), Track(), Track(),
                              Track(), Track(), Track(), Track(), Track() };

        static const int SAMPLE_RATE = 44100;
        const int FRAMES_PER_BUFFER = 512;  // Уменьшили для уменьшения задержки

        std::atomic<bool> isPlaying{ false };
        std::atomic<double> playheadPosition{ 0.0 }; // Текущая позиция воспроизведения в секундах

        Core();
        ~Core();

		void AddClip(size_t trackIdx, AudioClip clip);

        static int AudioCallback(const void* inputBuffer, void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

        void UpdateStreamers(const std::vector<Track>& tracks);

        bool IsValidClipIndex(size_t trackIdx, size_t clipIdx) const;
        void ChangeTrack();
        void StartPlayback();
        void StopPlayback();
       // Engine::Core::ClipStreamer CreateClipStreamer(const AudioClip* clip, const Track& track, double currentTime);
       // bool IsClipActive(const AudioClip& clip, double currentTime);
       // void CleanupInactiveStreamers();
       // void ResetFinishedFlagForClips(double newPosition);
       // void AddNewStreamer(const AudioClip* clip, const Track& track, double currentTime);
        void SetPlayheadPosition(double newPosition);

        void MoveClip(size_t trackIdx, size_t clipIdx, double newStartTime);
   

    };

    class FileManager {
    public:
        static bool ValidateAudioFile(const std::string& path);
        static bool LoadAudioData(AudioClip& clip);
    };

private:
     Core core;
     FileManager fileManager;
     std::atomic<bool> m_isPlaying{ false };
     mutable std::mutex m_mutex;
};


