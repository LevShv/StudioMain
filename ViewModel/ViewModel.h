// ViewModel.h
#pragma once
#include <log.h>
#include <QObject>
#include "engine.h" // Предполагается, что у вас есть этот файл

class ViewModel : public QObject {
    Q_OBJECT
        Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
        Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)

public:
    explicit ViewModel(QObject* parent = nullptr);

    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void setPlayheadPosition(double position);
    Q_INVOKABLE void moveClip(size_t trackIdx, size_t clipIdx, double newStartTime);

    bool isPlaying() const;
    int volume() const;
    void setVolume(int volume);

signals:
    void isPlayingChanged();
    void volumeChanged();

private:
    Engine engine;
    bool m_isPlaying = false;
    int m_volume = 50;
};