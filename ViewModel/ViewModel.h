// ViewModel.h
#pragma once
#include <log.h>
#include <QObject>
#include "engine.h"
#include "TrackModel.h"
#include <QTimer>
#include <QWindow>
#include "MidiMessageModel.h"
#include "PluginModel.h"

class ViewModel : public QObject {
    Q_OBJECT
        Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
        Q_PROPERTY(int volume READ getUserVolume WRITE setUserVolume NOTIFY volumeChanged)
        Q_PROPERTY(int masterGain READ getMasterGain WRITE setMasterGain NOTIFY gainChanged)
        Q_PROPERTY(double playheadPosition READ playheadPosition NOTIFY playheadPositionChanged)
        Q_PROPERTY(TrackModel* trackModel READ trackModel CONSTANT)
        Q_PROPERTY(MidiMessageModel* midiModel READ midiModel CONSTANT)
        Q_PROPERTY(PluginModel* pluginModel READ pluginModel CONSTANT)
        Q_PROPERTY(QString currentProjectPath READ currentProjectPath WRITE setCurrentProjectPath NOTIFY currentProjectPathChanged)
        Q_PROPERTY(float renderProgress READ renderProgress NOTIFY renderProgressChanged)

public:
    explicit ViewModel(QObject* parent = nullptr);

    double playheadPosition() const { return m_playheadPosition; }

    TrackModel* trackModel() const { return m_trackModel; }
    MidiMessageModel* midiModel() const { return m_midiModel; }
    Engine* getEngine() { return &engine; }
    PluginModel* pluginModel() const { return m_pluginModel; }
    float renderProgress() const { return m_renderProgress; }

    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void setPlayheadPosition(double position);
    Q_INVOKABLE void setBpm(double bpm);
    Q_INVOKABLE void setVolume(int volume);

    Q_INVOKABLE void deleteTrack(int trackIndex);
    Q_INVOKABLE void addAudioTrack();
    Q_INVOKABLE void addMidiTrack();
    Q_INVOKABLE void addSamplerTrack();

    Q_INVOKABLE void setTrackGain(int trackIndex, float gain);
    Q_INVOKABLE void setTrackMute(int index, bool muted);
    Q_INVOKABLE void toggleSolo(int trackIndex);
    Q_INVOKABLE void setName(int trackIndex, QString name);

    Q_INVOKABLE void moveClip(size_t trackIdx, size_t clipIdx, double newStartTime);
    Q_INVOKABLE void deleteClip(int trackIndex, int clipindex);
    Q_INVOKABLE void deleteClips(const QVariantList& clips); 
    Q_INVOKABLE void addAudioClip(int trackIndex, const QString& filePath, double startTime);
    Q_INVOKABLE void addMidiClip(int trackIndex, double startTime);
    Q_INVOKABLE void AddCloneClip(int trackIndex, int masterClipIndex, double startBeats);
    Q_INVOKABLE void changeClipDuration(int trackIndex, int clipIndex, double newDuration);
    Q_INVOKABLE void changeColor(int trackIndex, int clipIndex, const QColor& color);
    Q_INVOKABLE void copyMidiClip(int trackIndex, int clipIndex, double startTime);

    Q_INVOKABLE void RenderToWave(QString path);
    Q_INVOKABLE void SaveProject(QString path);
    Q_INVOKABLE void OpenProject(QString);
    Q_INVOKABLE void createNewProject();

    Q_INVOKABLE QString applicationHomeFolder() const;

    Q_INVOKABLE void enableLoopMode(int trackIndex, int clipIndex);
    Q_INVOKABLE void disableLoopMode();

    Q_INVOKABLE void setUserVolume(float volume);
    Q_INVOKABLE float getUserVolume();

    Q_INVOKABLE void setMasterGain(float volume);
    Q_INVOKABLE float getMasterGain();
   
    bool isPlaying() const;
    int volume() const;

    QString currentProjectPath() const { return m_currentProjectPath; }
    void setCurrentProjectPath(const QString& path);
    Q_SIGNAL void currentProjectPathChanged();
    Q_INVOKABLE void prepareForExit();



signals:
    void isPlayingChanged();
    void volumeChanged();
    void gainChanged();
    void bpmChanged();
    void playheadPositionChanged(double position);
    void clipAdded(int trackIndex);
    void clipMoved(int trackIndex, int clipIndex, double newStartTime);
    void trackGainChanged(int trackIndex, float gain); 
    void trackAdded(int trackIndex); 
    void clipDurationChanged(int trackIndex,int clipIndex, double newDuration);
    void renderProgressChanged();
    void renderFinished(bool success, QString errorMessage);

    

private slots:
    void updatePlayhead();
    void updateRenderProgress(float progress);

private:

    Engine engine;
    TrackModel* m_trackModel;
    MidiMessageModel* m_midiModel;
    PluginModel* m_pluginModel;


	double m_bpm = 120.0;
    double m_playheadPosition = engine.Position();
    bool m_isPlaying = false;
    int m_volume = 1;
    int m_masterGain = 1;
    int soloTrackInd = -1;
    float m_renderProgress = 0.0;

	const std::string samplerPath = "C:\\Users\\llvvv\\source\\repos\\Studio\\Plugins\\Just a Sample.vst3";

	QTimer* m_playheadTimer;
   

    void buildModel();
    void stopDoplay(void (*func)(...));
    QString m_currentProjectPath;

};