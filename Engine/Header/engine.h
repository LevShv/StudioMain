#pragma once
#include <JuceHeader.h>

class Engine {
public:

#pragma region Structures

    struct ClipBase {

        std::string clipID;
        std::string color = "#FFFFFF";
        std::string name = "";

        double startTime = 0.0;
        double duration = 0.0;
        float gain = 1.0f;
        bool muted = false;

        double startBeats = 0.0; 
        double durationBeats = 0.0; 

        virtual ~ClipBase() = default;
        virtual bool isActive(double time) const;
        virtual bool isActiveInRange(double startTime, double endTime) const;
        std::string generateClipID();
        std::string generateUniqueColor(const std::string& clipID);
    };

    using ClipPtr = std::unique_ptr<ClipBase>;

    struct AudioClip : public ClipBase {
        juce::File file;
        juce::AudioBuffer<float> buffer;
        bool useRAM = false;
        std::vector<float> waveformData;
    };

    struct MidiClip : public ClipBase {
        juce::MidiMessageSequence midiSequence;
        double minDurationBeats = 1;
    };

    struct CloneClip : public ClipBase {
        ClipBase* masterClip = nullptr; 
        std::string masterClipID;

        CloneClip(ClipBase* master, double startBeats);
        bool isActive(double time) const override;
        bool isActiveInRange(double startTime, double endTime) const override;
    };

    struct PluginInstance {
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        juce::AudioProcessorEditor* editor = nullptr;
        bool bypass = false;
        std::string Path;
        juce::MemoryBlock state;

        PluginInstance() = default;
        ~PluginInstance();
    };
   
    struct Track {
        std::vector<std::unique_ptr<ClipBase>> clips;
        std::vector<std::unique_ptr<PluginInstance>> plugins;
        std::string name = "";
        float gain = 1.0f;
        bool muted = false;
        bool isMidiTrack = false;
        bool isSamplerTrack = false;
        bool solo = false;

        Track() = default;
        Track(const Track&) = delete;
        Track& operator=(const Track&) = delete;
        Track(Track&&) noexcept = default;
        Track& operator=(Track&&) noexcept = default;
    };

#pragma endregion

#pragma region Public Engine

    Engine();
    ~Engine();

    //Controls

    void StopMix();
    void PlayMix();
    void SetPlayheadPosition(double position);
    double GetPlayheadPosition() const;
    double GetBPM() const;
    void SetBPM(double newBPM);
    bool IsPlaying();
    double& Position();


    //Tracks  

    int AddAudioTrack();
    int AddMidiTrack();
    int AddSamplerTrack();
    void DeleteTrack(int trackIndex);
    void ChangeDuration(int trackIndex, int clipIndex, double newDuration);
    void SetTrackGain(int trackIndex, float gain);
    void SetTrackMute(int index, bool muted);
    void ToggleSolo(int trackIndex);
    void setName(int trackIndex, std::string name);


    //Clpis

    void AddAudioClip(int trackInd, const std::string& path, double startBeats, bool loadToRAM);
    bool AddMidiClip(int trackInd, double startBeats);
    void AddCloneClip(int trackIndex, int masterClipIndex, double startBeats);

    void MoveClip(int trackIndex, int clipIndex, double startBeats);
    void DeleteClip(int trackIndex, int clipIndex);
    void ChangeColor(int trackIndex, int clipIndex, std::string color);


    // Midi

    void SendMidiMessage(const juce::MidiMessage& message);
    void AddMidiNote(int trackIndex, int clipIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);
    void DeleteMidiNote(int trackIndex, int clipIndex, int noteIndex);
    void UpdateMidiNote(int trackIndex, int clipIndex, int noteIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);
    void cleanMidiSequence(MidiClip* midiClip);


    //Tools

    double SecondsToBeats(double seconds) const;
    double BeatsToSeconds(double beats) const;
    const std::vector<Engine::Track>& GetdataBase() const;


    //Clip loop mode

    void EnableLoopMode(int trackIndex, int clipIndex);
    void DisableLoopMode();


    //Plugins

    void AddPluginToTrack(int trackIndex, const std::string& pluginPath);
    void RemovePluginFromTrack(int trackIndex, int pluginIndex);
    void TogglePluginBypass(int trackIndex, int pluginIndex);
    juce::AudioProcessorEditor* GetPluginEditor(int trackIndex, int pluginIndex);


    //Files

    void RenderToFile(std::string& Path);
    void SaveProject(const std::string& Path);
    bool LoadProject(const std::string& Path);
    void CreateNewProject();

    //Prepaly

    void PlayNote(int trackIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);


#pragma endregion

private:

#pragma region Core

    class Core : public juce::AudioSource, private juce::MidiInputCallback {
    public:

        Core();
        ~Core();

        //Main objects

        juce::CriticalSection noteLock;
        juce::CriticalSection lock;
        std::vector<Track> tracks;

       
        //Controls 

        double position = 0.0;
        double positionInBeats = 0.0; 
        double bpm = 120.0;
        int timeSignatureNumerator = 4; 
        int timeSignatureDenominator = 4; 

        void startAudio(juce::AudioDeviceManager& deviceManager);
        void stopAudio(juce::AudioDeviceManager& deviceManager);
        void play();
        void stop();
        void setPosition(double newPosition);
        double getPosition() const { return position; }
        bool isPlaying() const { return transportPlaying; }

        void setTimeSignature(int numerator, int denominator);
        std::pair<int, int> getTimeSignature() const { return { timeSignatureNumerator, timeSignatureDenominator }; }

        void setBPM(double newBPM);
        double getBPM() const { return bpm; }



        
        //Converts

        double secondsToBeats(double seconds, double bpm) const;
        double secondsToBeats(double seconds) const;
        double beatsToSeconds(double beats, double bpm) const;
        double beatsToSeconds(double beats) const;
        double secondsToMeasures(double seconds) const;
        double measuresToSeconds(double measures) const;


        //Sound methods

        void updateActiveClips();
        void prepareToPlay(int samplesPerBlock, double sampleRate) override;
        void releaseResources() override;
        void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;


        //Tracks
         
        void setName(int trackIndex, std::string name);
        void toggleSolo(int trackIndex);


        //Clip methods

        void addCloneClip(int trackIndex, int masterClipIndex, double startBeats);
        void changeMidiclipDuration(int trackIndex, int clipIndex, double newDuration);
        void changeAudioclipDuration(int trackIndex, int clipIndex, double newDuration);
        void moveClip(int trackIndex, int clipIndex, double newStartTime);

        void loadAudioClip(int trackIndex, const juce::File& file, double startBeats, bool loadToRAM);
        void loadMidiClip(int trackIndex, const juce::MidiMessageSequence& sequence, double startBeats);
        void loadClipToRAM(AudioClip& clip);


        // Midi

        std::map<std::pair<int, int>, double> activeNotes; // ����: (�����, ����� ����), ��������: ����� noteOn
        std::unique_ptr<juce::MidiOutput> midiOutput;

        void addMidiNote(int trackIndex, int clipIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);
        void deleteMidiNote(int trackIndex, int clipIndex, int noteIndex);
        void updateMidiNote(int trackIndex, int clipIndex, int noteIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);
        void cleanMidiSequence(MidiClip* clip);

        void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

        //Plugins

        juce::AudioPluginFormatManager pluginFormatManager; // ��� �������� VST/VST3

        void addPluginToTrack(int trackIndex, const juce::String& pluginPath);
        void removePluginFromTrack(int trackIndex, int pluginIndex);
        void togglePluginBypass(int trackIndex, int pluginIndex);
        juce::AudioProcessorEditor* getPluginEditor(int trackIndex, int pluginIndex);


        //Clip loop mode
        
        bool loopModeEnabled = false;// Флаг режима циклического воспроизведения
        int loopTrackIndex = -1;     // Индекс трека для циклического воспроизведения
        int loopClipIndex = -1;      // Индекс клипа для циклического воспроизведения
        double loopStartTime = 0.0;  // Начальная позиция воспроизведения в секундах относительно клипа
        double loopDuration = 0.0;   // Длительность циклического воспроизведения (длительность клипа)
        
        void enableLoopMode(int trackIndex, int clipIndex);
        void disableLoopMode();


        //Render

        void RenderToFile(std::string& Path);


        //Preplay

        void playNote(int trackIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);

        


    private:

        struct ActiveClip {
            std::unique_ptr<juce::AudioFormatReaderSource> source;
            const ClipBase* clip = nullptr;
            const Track* track = nullptr;
            juce::int64 position = 0;

        };
        juce::KnownPluginList pluginList; // ��������� KnownPluginList

        juce::AudioBuffer<float> pluginBuffer; // ����� ��� ��������� ��������
        juce::AudioFormatManager formatManager;
        juce::AudioSourcePlayer audioSourcePlayer;

        juce::Array<ActiveClip> activeClips;
        double sampleRate = 44100.0;
        
        bool transportPlaying = false;
        bool audioProcessingEnabled = true;

        void processMidiBlocks(const juce::AudioSourceChannelInfo&, double startTime, double endTime);
};

#pragma endregion

#pragma region Saver

    class Saver {
    public:
        Saver(Core& core) : m_core(core) {}
        void SaveProject(const std::string filePath);
        bool LoadProject(const std::string filePath);
    private:
        Core& m_core;
    };

#pragma endregion
    
#pragma region Private Engine
    Core core;
    Saver saver{ core };
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;

    void configureMidiDevices();

#pragma endregion
};