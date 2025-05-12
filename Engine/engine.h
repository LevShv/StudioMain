#pragma once
#include <log.h>
#include <JuceHeader.h>

class Engine {
private:

    struct ClipBase {
        double startTime = 0.0;
        double duration = 0.0;
        float gain = 1.0f;
        bool muted = false;

        virtual ~ClipBase() = default;
        virtual bool isActive(double time) const {
            return time >= startTime && time < startTime + duration;
        }
    };

    struct AudioClip : public ClipBase {
        juce::File file;
        juce::AudioBuffer<float> buffer;
        bool useRAM = false;
    };

    struct MidiClip : public ClipBase {
        juce::MidiMessageSequence midiSequence;
    };

    struct Track {
        std::vector<std::unique_ptr<ClipBase>> clips;
        float gain = 1.0f;
        bool muted = false;
        bool isMidiTrack = false;

        Track() = default;

        // Явно удаляем копирование
        Track(const Track&) = delete;
        Track& operator=(const Track&) = delete;

        // Конструктор/оператор перемещения
        Track(Track&&) noexcept = default;
        Track& operator=(Track&&) noexcept = default;
    };

    class Core : public juce::AudioSource, private juce::MidiInputCallback {
    public:
        Core();
        ~Core();

        double position = 0.0;

        std::unique_ptr<juce::MidiOutput> midiOutput;
        juce::CriticalSection lock;
        std::vector<Track> tracks;

        void startAudio(juce::AudioDeviceManager& deviceManager);
        void stopAudio(juce::AudioDeviceManager& deviceManager);
        void play();
        void stop();
        void setPosition(double newPosition);
        double getPosition() const { return position; }
        bool isPlaying() const { return transportPlaying; }

        void loadAudioClip(int trackIndex, const juce::File& file, double startTime, bool loadToRAM);
        void loadMidiClip(int trackIndex, const juce::MidiMessageSequence& sequence, double startTime);
        void moveClip(int trackIndex, int clipIndex, double newStartTime);

        void prepareToPlay(int samplesPerBlock, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
        void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

    private:
        struct ActiveClip {
            std::unique_ptr<juce::AudioFormatReaderSource> source;
            const ClipBase* clip = nullptr;
            const Track* track = nullptr;
            juce::int64 position = 0;


        };

        juce::AudioFormatManager formatManager;
        juce::AudioSourcePlayer audioSourcePlayer;

        juce::Array<ActiveClip> activeClips;
        double sampleRate = 44100.0;
        
        bool transportPlaying = false;

        void updateActiveClips();
        void loadClipToRAM(AudioClip& clip);
        void processMidiBlocks(const juce::AudioSourceChannelInfo&, double startTime, double endTime);
    };

    Core core;
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;

    void configureMidiDevices();
    
public:

    Engine();
    ~Engine();

    void AddAudioClip(int trackInd, const std::string& path, double startTime, bool loadToRAM);
    void AddMidiClip(int trackInd, const juce::MidiMessageSequence& sequence, double startTime);
    void StopMix();
    void PlayMix();
    void MoveClip(int trackIndex, int clipIndex, double newStartTime);
    void SetPlayheadPosition(double position);
    bool IsPlaying();
    void SendMidiMessage(const juce::MidiMessage& message);

    //AddTrack();
    const std::vector<Engine::Track>& GetdataBase() const;

    double& Position();
};