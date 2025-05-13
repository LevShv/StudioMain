// ViewModel.h
#pragma once
#include <log.h>
#include <QObject>
#include "engine.h" // Предполагается, что у вас есть этот файл
#include "TrackModel.h"

class ViewModel : public QObject {
    Q_OBJECT
        Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
        Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
        Q_PROPERTY(TrackModel* trackModel READ trackModel CONSTANT)

public:
    explicit ViewModel(QObject* parent = nullptr);

    TrackModel* trackModel() const { return m_trackModel; }

    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void setPlayheadPosition(double position);
    Q_INVOKABLE void moveClip(size_t trackIdx, size_t clipIdx, double newStartTime);
    Q_INVOKABLE void addAudioClip(int trackIndex, const QString& filePath, double startTime);
    Q_INVOKABLE void setVolume(int volume);

    bool isPlaying() const;
    int volume() const;

signals:
    void isPlayingChanged();
    void volumeChanged();
    void playheadPositionChanged(double position);
    void clipAdded(int trackIndex); // Сигнал о добавлении клипа
    void clipMoved(int trackIndex, int clipIndex, double newStartTime); // Сигнал о перемещении клипа

private:
    Engine engine;
    TrackModel* m_trackModel;
    double m_playheadPosition = engine.Position();
    bool m_isPlaying = false;
    int m_volume = 50;
};