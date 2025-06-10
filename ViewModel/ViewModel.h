// ViewModel.h
#pragma once
#include <log.h>
#include <QObject>
#include "engine.h" // Предполагается, что у вас есть этот файл
#include "TrackModel.h"
#include <QTimer>
#include <QWindow>
#include "MidiMessageModel.h" // Добавляем для MidiMessageModel
#include "PluginModel.h"

class ViewModel : public QObject {
    Q_OBJECT
        Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
        Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
        Q_PROPERTY(double playheadPosition READ playheadPosition NOTIFY playheadPositionChanged)
        Q_PROPERTY(TrackModel* trackModel READ trackModel CONSTANT)
        Q_PROPERTY(MidiMessageModel* midiModel READ midiModel CONSTANT) // Свойство для midiModel
        Q_PROPERTY(PluginModel* pluginModel READ pluginModel CONSTANT)

public:
    explicit ViewModel(QObject* parent = nullptr);

    double playheadPosition() const { return m_playheadPosition; }

    TrackModel* trackModel() const { return m_trackModel; }
    MidiMessageModel* midiModel() const { return m_midiModel; } // Геттер для midiModel
    Engine* getEngine() { return &engine; } // Оставляем для других случаев
    PluginModel* pluginModel() const { return m_pluginModel; }

    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void setPlayheadPosition(double position);
    Q_INVOKABLE void moveClip(size_t trackIdx, size_t clipIdx, double newStartTime);
 
    Q_INVOKABLE void setVolume(int volume);

    Q_INVOKABLE void deleteTrack(int trackIndex);
    Q_INVOKABLE void deleteClip(int trackIndex, int clipindex);
    Q_INVOKABLE void deleteClips(const QVariantList& clips); // Новый метод
    Q_INVOKABLE void addAudioTrack();
	Q_INVOKABLE void addMidiTrack();
    Q_INVOKABLE void addSamplerTrack();
    Q_INVOKABLE void addAudioClip(int trackIndex, const QString& filePath, double startTime);
    Q_INVOKABLE void addMidiClip(int trackIndex, double startTime);
    Q_INVOKABLE void AddCloneClip(int trackIndex, int masterClipIndex, double startBeats);

    Q_INVOKABLE void RenderToWave(QString path);
    Q_INVOKABLE void SaveProject(QString path);
    Q_INVOKABLE void OpenProject(QString);

    Q_INVOKABLE void changeClipDuration(int trackIndex, int clipIndex, double newDuration); // Новый метод
    Q_INVOKABLE QString applicationHomeFolder() const;

    Q_INVOKABLE void enableLoopMode(int trackIndex, int clipIndex);
    Q_INVOKABLE void disableLoopMode();
   
    bool isPlaying() const;
    int volume() const;

signals:
    void isPlayingChanged();
    void volumeChanged();
    void bpmChanged();
    void playheadPositionChanged(double position);
    void clipAdded(int trackIndex);
    void clipMoved(int trackIndex, int clipIndex, double newStartTime);
    //void pluginBypassed(int trackIndex, int pluginIndex);
    //void pluginAdded(int trackIndex);
    // void pluginRemoved(int trackIndex, int pluginIndex);
   // void pluginEditorOpened(int trackIndex, int pluginIndex, QWindow* window);
    void trackAdded(int trackIndex); 
    void clipDurationChanged(int trackIndex,int clipIndex, double newDuration);

    

private slots:
    void updatePlayhead();

private:

    Engine engine;
    TrackModel* m_trackModel;
    MidiMessageModel* m_midiModel;
    PluginModel* m_pluginModel;


	double m_bpm = 120.0; // Инициализация BPM
    double m_playheadPosition = engine.Position();
    bool m_isPlaying = false;
    int m_volume = 50;

	const std::string samplerPath = "C:\\Users\\llvvv\\source\\repos\\Studio\\Plugins\\Just a Sample.vst3"; // Укажите реальный путь к сэмплеру

	QTimer* m_playheadTimer;
   

    void buildModel();
};