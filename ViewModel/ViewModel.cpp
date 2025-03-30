// ViewModel.cpp

#include "ViewModel.h"
#include <QtCore/QDebug>
#include <string>


ViewModel::ViewModel(QObject* parent) : QObject(parent)
{
    engine.AddAudioClip(0, "Misc/Happy.wav", 0.0, false);
    engine.AddAudioClip(1, "Misc/Step5.wav", 1, true);
}

Q_INVOKABLE void ViewModel::togglePlayback()
{
    if (engine.IsPlaying()) {
        engine.StopMix();
        m_isPlaying = false;
    }
    else {
        engine.PlayMix();
        m_isPlaying = true;
    }
    emit isPlayingChanged();

    qDebug() << (m_isPlaying ? "Track is playing" : "Track is stopped");
}

Q_INVOKABLE void ViewModel::setPlayheadPosition(double position)
{
    engine.SetPlayheadPosition(position);
}

Q_INVOKABLE void ViewModel::moveClip(size_t trackIdx, size_t clipIdx, double newStartTime)
{
    engine.MoveClip(trackIdx, clipIdx, newStartTime);
}

bool ViewModel::isPlaying() const
{
    return m_isPlaying;
}

int ViewModel::volume() const
{
    return m_volume;
}

void ViewModel::setVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        // Здесь можно добавить установку громкости в engine
        emit volumeChanged();
        qDebug() << "Volume changed to:" << m_volume;
    }
}