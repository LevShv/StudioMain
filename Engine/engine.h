#pragma once
#include <log.h>
#include <JuceHeader.h>

class Engine {
public:

    struct ClipBase {
        double startTime = 0.0;
        double duration = 0.0;
        float gain = 1.0f;
        bool muted = false;


        double startBeats = 0.0; // Время в ударах
        double durationBeats = 0.0; // Длительность в ударах

        virtual ~ClipBase() = default;
        virtual bool isActive(double time) const {
            return time >= startTime && time < startTime + duration;
        }

        virtual bool isActiveInRange(double startTime, double endTime) const {
            const double epsilon = 0.0001;
            return (this->startTime <= endTime + epsilon) &&
                (this->startTime + this->duration >= startTime - epsilon);
        }
    };

    using ClipPtr = std::unique_ptr<ClipBase>;

    struct AudioClip : public ClipBase {
        juce::File file;
        juce::AudioBuffer<float> buffer;
        bool useRAM = false;
    };

    struct MidiClip : public ClipBase {
        juce::MidiMessageSequence midiSequence;
    };

    struct PluginInstance {
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        juce::AudioProcessorEditor* editor = nullptr; // Для GUI плагина
        bool bypass = false;
        std::string Path;

        PluginInstance() = default;
        ~PluginInstance() { if (editor) delete editor; }
    };
   
    struct Track {
        std::vector<std::unique_ptr<ClipBase>> clips;
        std::vector<std::unique_ptr<PluginInstance>> plugins;
        float gain = 1.0f;
        bool muted = false;
        bool isMidiTrack = false;
        bool isSamplerTrack = false;

        Track() = default;

        // Явно удаляем копирование
        Track(const Track&) = delete;
        Track& operator=(const Track&) = delete;

        // Конструктор/оператор перемещения
        Track(Track&&) noexcept = default;
        Track& operator=(Track&&) noexcept = default;
    };

    Engine();
    ~Engine();

	void AddPluginToTrack(int trackIndex, const std::string& pluginPath);
	void RemovePluginFromTrack(int trackIndex, int pluginIndex);
	juce::AudioProcessorEditor* GetPluginEditor(int trackIndex, int pluginIndex);

	void TogglePluginBypass(int trackIndex, int pluginIndex);

    void AddAudioClip(int trackInd, const std::string& path, double startBeats, bool loadToRAM);
    void AddMidiClip(int trackInd, const juce::MidiMessageSequence& sequence, double startBeats);
    void StopMix();
    void PlayMix();
    void MoveClip(int trackIndex, int clipIndex, double startBeats);
    void SetPlayheadPosition(double position);
    double GetPlayheadPosition() const;
    bool IsPlaying();
    void SendMidiMessage(const juce::MidiMessage& message);
    double GetBPM() const;
    void SetBPM(double newBPM);

    // Добавляем новые методы для добавления дорожек
    int AddAudioTrack();
    int AddMidiTrack();
    int AddSamplerTrack();

    void DeleteTrack(int trackIndex);
    void DeleteClip(int trackIndex, int clipIndex);

    //AddTrack();
    const std::vector<Engine::Track>& GetdataBase() const;

    double& Position();

    void RenderToFile(std::string& Path);
    void SaveProject(const std::string& Path);
    bool LoadProject(const std::string& Path);

private:

    class Core : public juce::AudioSource, private juce::MidiInputCallback {
    public:
        Core();
        ~Core();
       

        double position = 0.0;
        double positionInBeats = 0.0; // Позиция в ударах
        double bpm = 120.0; // Значение по умолчанию: 120 BPM
        int timeSignatureNumerator = 4; // Числитель метра (4 в 4/4)
        int timeSignatureDenominator = 4; // Знаменатель метра (4 в 4/4)

        std::map<std::pair<int, int>, double> activeNotes; // Ключ: (канал, номер ноты), значение: время noteOn
        juce::CriticalSection noteLock;

        std::unique_ptr<juce::MidiOutput> midiOutput;
        juce::CriticalSection lock;
        std::vector<Track> tracks;

        juce::AudioPluginFormatManager pluginFormatManager; // Для загрузки VST/VST3

        void addPluginToTrack(int trackIndex, const juce::String& pluginPath);
        void removePluginFromTrack(int trackIndex, int pluginIndex);
        void togglePluginBypass(int trackIndex, int pluginIndex);
        juce::AudioProcessorEditor* getPluginEditor(int trackIndex, int pluginIndex);

        void setBPM(double newBPM);
        double getBPM() const { return bpm; }
        void setTimeSignature(int numerator, int denominator);
        std::pair<int, int> getTimeSignature() const { return { timeSignatureNumerator, timeSignatureDenominator }; }
        double secondsToBeats(double seconds) const;
        double beatsToSeconds(double beats) const;
        double secondsToMeasures(double seconds) const;
        double measuresToSeconds(double measures) const;

        void startAudio(juce::AudioDeviceManager& deviceManager);
        void stopAudio(juce::AudioDeviceManager& deviceManager);
        void play();
        void stop();
        void setPosition(double newPosition);
        double getPosition() const { return position; }
        bool isPlaying() const { return transportPlaying; }

        void updateActiveClips();

        void loadAudioClip(int trackIndex, const juce::File& file, double startBeats, bool loadToRAM);
        void loadMidiClip(int trackIndex, const juce::MidiMessageSequence& sequence, double startBeats);
        void loadClipToRAM(AudioClip& clip);

        void moveClip(int trackIndex, int clipIndex, double newStartTime);

        //void updateActiveClips(double blockStartTime, double blockEndTime);

        void prepareToPlay(int samplesPerBlock, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
        void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;


		void RenderToFile(std::string& Path);

    private:

        struct ActiveClip {
            std::unique_ptr<juce::AudioFormatReaderSource> source;
            const ClipBase* clip = nullptr;
            const Track* track = nullptr;
            juce::int64 position = 0;

        };
        juce::KnownPluginList pluginList; // Добавляем KnownPluginList

        juce::AudioBuffer<float> pluginBuffer; // Буфер для обработки плагинов
        juce::AudioFormatManager formatManager;
        juce::AudioSourcePlayer audioSourcePlayer;

        juce::Array<ActiveClip> activeClips;
        double sampleRate = 44100.0;
        
        bool transportPlaying = false;

        void processMidiBlocks(const juce::AudioSourceChannelInfo&, double startTime, double endTime);
    };

    class Saver {
    public:
        Saver(Core& core) : m_core(core) {}
        void SaveProject(const std::string filePath);
        bool LoadProject(const std::string filePath);
    private:
        Core& m_core;
    };

    Core core;
    Saver saver{ core };
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;

    void configureMidiDevices();
};