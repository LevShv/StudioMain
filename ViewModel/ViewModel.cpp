#include "ViewModel.h"
#include <QDebug>

ViewModel::ViewModel(QObject* parent) : QObject(parent) {
    m_trackModel = new TrackModel(engine, this);
    m_bpm = engine.GetBPM();
    m_playheadPosition = engine.Position();
    m_isPlaying = false;
    m_volume = 50;

    m_playheadTimer = new QTimer(this);
    connect(m_playheadTimer, &QTimer::timeout, this, &ViewModel::updatePlayhead);
}

void ViewModel::togglePlayback() {
    if (isPlaying()) {
        engine.StopMix();
        m_playheadTimer->stop();
    }
    else {
        engine.PlayMix();
        m_playheadTimer->start(16); // ~60 FPS
    }
    m_isPlaying = engine.IsPlaying();
    emit isPlayingChanged();
}

void ViewModel::updatePlayhead() {
    double position = engine.GetPlayheadPosition();
    if (std::abs(position - m_playheadPosition) > 0.001) {
        m_playheadPosition = position;
        emit playheadPositionChanged(position);
    }
}

void ViewModel::setPlayheadPosition(double position) {
    engine.SetPlayheadPosition(position);
    m_playheadPosition = position;
    emit playheadPositionChanged(position);
}

void ViewModel::moveClip(size_t trackIdx, size_t clipIdx, double newStartTime) {
    engine.MoveClip(static_cast<int>(trackIdx), static_cast<int>(clipIdx), newStartTime);
    ClipModel* clipModel = m_trackModel->getClipModel(static_cast<int>(trackIdx));
    if (clipModel) {
        clipModel->updateClip(static_cast<int>(clipIdx)); // Уведомляем ClipModel об изменении
    }
    emit clipMoved(static_cast<int>(trackIdx), static_cast<int>(clipIdx), newStartTime);
}

void ViewModel::addAudioClip(int trackIndex, const QString& filePath, double startTime) {
    engine.AddAudioClip(trackIndex, filePath.toStdString(), startTime, true);
    ClipModel* clipModel = m_trackModel->getClipModel(trackIndex);
    if (clipModel) {
        clipModel->addClip(engine.GetdataBase()[trackIndex].clips.back()); // Уведомляем о новом клипе
    }
    emit clipAdded(trackIndex);
}

void ViewModel::addPlugin(int trackIndex, const QString& pluginPath) {
    engine.AddPluginToTrack(trackIndex, pluginPath.toStdString());
    emit pluginAdded(trackIndex);
}

void ViewModel::togglePluginBypass(int trackIndex, int pluginIndex) {
    engine.TogglePluginBypass(trackIndex, pluginIndex);
    emit pluginBypassed(trackIndex, pluginIndex);
}

void ViewModel::openPluginEditor(int trackIndex, int pluginIndex) {
    if (auto* editor = engine.GetPluginEditor(trackIndex, pluginIndex)) {
        QWindow* pluginWindow = new QWindow();
        pluginWindow->setTitle(QString("Plugin Editor - Track %1, Plugin %2").arg(trackIndex + 1).arg(pluginIndex + 1));

        auto* component = dynamic_cast<juce::Component*>(editor);
        if (component) {
            component->addToDesktop(0);
            auto nativeHandle = component->getWindowHandle();

            pluginWindow->create();
            pluginWindow->setGeometry(100, 100, component->getWidth(), component->getHeight());
            pluginWindow->setProperty("nativeHandle", reinterpret_cast<qlonglong>(nativeHandle));
            QWindow::fromWinId(reinterpret_cast<WId>(nativeHandle))->setParent(pluginWindow);

            pluginWindow->resize(component->getWidth(), component->getHeight());
            pluginWindow->show();

            emit pluginEditorOpened(trackIndex, pluginIndex, pluginWindow);
        }
        else {
            qWarning() << "Failed to cast editor to JUCE Component";
            delete pluginWindow;
        }
    }
    else {
        qWarning() << "Failed to open plugin editor for track" << trackIndex << ", plugin" << pluginIndex;
    }
}

void ViewModel::addWavTrack() {
    int newTrackIndex = engine.AddAudioTrack();
    if (newTrackIndex >= 0) {
        m_trackModel->ensureClipModel(newTrackIndex); // Создаём ClipModel
        emit trackAdded(newTrackIndex);
        qDebug() << "Added WAV track at index:" << newTrackIndex;
    }
}

void ViewModel::addSamplerTrack() {
    int newTrackIndex = engine.AddMidiTrack();
    if (newTrackIndex >= 0) {
        m_trackModel->ensureClipModel(newTrackIndex); // Создаём ClipModel
        engine.AddPluginToTrack(newTrackIndex, "path/to/TAL-Sampler.vst3"); // Укажите реальный путь
        emit trackAdded(newTrackIndex);
        emit pluginAdded(newTrackIndex);
        qDebug() << "Added Sampler track at index:" << newTrackIndex;
    }
}

void ViewModel::addMidiTrack() {
    int newTrackIndex = engine.AddMidiTrack();
    if (newTrackIndex >= 0) {
        m_trackModel->ensureClipModel(newTrackIndex); // Создаём ClipModel
        emit trackAdded(newTrackIndex);
        qDebug() << "Added MIDI track at index:" << newTrackIndex;
    }
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