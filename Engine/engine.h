#pragma once
#include <log.h>
#include <JuceHeader.h>
class Engine {
public:

    struct Clip {
        juce::File file;
        double startTime = 0.0;
        double duration = 0.0;
        float gain = 1.0f;
        bool muted = false;
        juce::AudioBuffer<float> buffer;
        bool useRAM = false;

        bool isActive(double time) const {
            return time >= startTime && time < startTime + duration;
        }
    };

    struct Track {
        juce::Array<Clip> clips;
        float gain = 1.0f;
        bool muted = false;
    };

    Engine();
    ~Engine();

    void AddClip(int trackInd, std::string path, int startTime, bool loadToRAM);
    void StopMix();
    void PlayMix();
    void MoveClip(int trackIndex, int clipIndex, double newStartTime);
    void SetPlayheadPosition(double position);
    bool IsPlaying();


private:

    void configureMidiDevices();

    class Core : public juce::AudioSource {
    public:

        Core();
        ~Core();

        juce::CriticalSection lock;
        // Управление аудиоустройством
        void startAudio(juce::AudioDeviceManager& deviceManager);
        void stopAudio(juce::AudioDeviceManager& deviceManager);

        // Управление воспроизведением
        void play();
        void stop();
        void setPosition(double newPosition);
        double getPosition() const { return position; }
        bool isPlaying() const { return transportPlaying; }

        // Работа с клипами
        void loadClip(int trackIndex, const juce::File& file, double startTime, bool loadToRAM = false);
        void moveClip(int trackIndex, int clipIndex, double newStartTime);

        // AudioSource interface
        void prepareToPlay(int samplesPerBlock, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;

    private:
        juce::AudioFormatManager formatManager;
        juce::Array<Track> tracks;
        juce::AudioSourcePlayer audioSourcePlayer; // Добавлен AudioSourcePlayer

        bool playing = false;  // Добавляем объявление переменной
        double sampleRate = 44100.0;
        double position = 0.0;
        bool transportPlaying = false;
        

        struct ActiveClip {
            std::unique_ptr<juce::AudioFormatReaderSource> source;
            const Clip* clip = nullptr;
            const Track* track = nullptr;
            juce::int64 position = 0;
        };
        juce::Array<ActiveClip> activeClips;

        void updateActiveClips();
        void loadClipToRAM(Clip& clip);
    };

    Core core;
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;



};
