// ViewModel.cpp
#include "ViewModel.h"
//
ViewModel::ViewModel(QObject* parent) : QObject(parent) {
    m_trackModel = new TrackModel(engine, this);
    m_bpm = engine.GetBPM(); // Инициализация BPM из Engine
    m_playheadPosition = 0.0;
    m_isPlaying = false;
    m_volume = 50;
    
    m_playheadTimer = new QTimer(this);
    connect(m_playheadTimer, &QTimer::timeout, this, &ViewModel::updatePlayhead);
}

void ViewModel::togglePlayback() {
    if (isPlaying()) {
        engine.StopMix();
        m_playheadTimer->stop();
        //setPlayheadPosition(0.0); // Сбрасываем позицию при остановке
    }
    else {
        engine.PlayMix();
        m_playheadTimer->start(16); // ~60 FPS
    }
    m_isPlaying = engine.IsPlaying();
    emit isPlayingChanged();
}

void ViewModel::updatePlayhead() {
    double position = engine.GetPlayheadPosition(); // Получаем позицию в битах
    if (std::abs(position - m_playheadPosition) > 0.001) { // Избегаем лишних сигналов
        m_playheadPosition = position;
        emit playheadPositionChanged(position);
    }
}

void ViewModel::setPlayheadPosition(double position) {
    //double seconds = engine.beatsToSeconds(position);
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
    engine.AddAudioClip(trackIndex, filePath.toStdString(), startTime, true);
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