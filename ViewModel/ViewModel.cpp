// ViewModel.cpp
#include "ViewModel.h"
//
ViewModel::ViewModel(QObject* parent) : QObject(parent) {
    m_trackModel = new TrackModel(engine, this);
    
}

void ViewModel::togglePlayback() {
    if (isPlaying()) {
        engine.StopMix();
    }
    else {
        engine.PlayMix();
    }
    m_isPlaying = engine.IsPlaying();
    emit isPlayingChanged();
}

void ViewModel::setPlayheadPosition(double position) {
    engine.SetPlayheadPosition(position);
    m_playheadPosition = position;
    emit playheadPositionChanged(position);
}

void ViewModel::moveClip(size_t trackIdx, size_t clipIdx, double newStartTime) {
    // Вызываем метод Engine
    engine.MoveClip(static_cast<int>(trackIdx), static_cast<int>(clipIdx), newStartTime);
    // Уведомляем о перемещении
    emit clipMoved(static_cast<int>(trackIdx), static_cast<int>(clipIdx), newStartTime);
}

void ViewModel::addAudioClip(int trackIndex, const QString& filePath, double startTime) {
    // Вызываем метод Engine
    engine.AddAudioClip(trackIndex, filePath.toStdString(), startTime, false);
    // Уведомляем о добавлении
    emit clipAdded(trackIndex);
}

bool ViewModel::isPlaying() const {
    return m_isPlaying;
}

int ViewModel::volume() const {
    return m_volume;
}

void ViewModel::setVolume(int volume) {
    if (m_volume != volume) {
        m_volume = volume;
        emit volumeChanged();
    }
}