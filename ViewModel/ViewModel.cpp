#include "ViewModel.h"
#include <QDebug>

ViewModel::ViewModel(QObject* parent) : QObject(parent) {

    m_trackModel = new TrackModel(engine, this);

    m_playheadTimer = new QTimer(this);
    connect(m_playheadTimer, &QTimer::timeout, this, &ViewModel::updatePlayhead);

    buildModel();
}

void ViewModel::buildModel() {
    m_bpm = engine.GetBPM();
    m_playheadPosition = engine.Position();
    m_isPlaying = false;
    m_volume = 50;

    // Уведомляем QML об изменениях
    emit bpmChanged();
    emit playheadPositionChanged(m_playheadPosition);
    emit isPlayingChanged();
    emit volumeChanged();
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

void ViewModel::deleteTrack(int trackIndex) {
    if (trackIndex >= 0 && trackIndex < engine.GetdataBase().size()) {
        bool wasPlaying = isPlaying();
        if (wasPlaying) {
            engine.StopMix();
            m_playheadTimer->stop();
            m_isPlaying = false;
            emit isPlayingChanged();
            qDebug() << "Stopped playback before deleting track";
        }

        engine.DeleteTrack(trackIndex);
        m_trackModel->deleteTrack(trackIndex);
        qDebug() << "Deleted track at index:" << trackIndex;

        // Возобновляем воспроизведение, если оно было активно
        if (wasPlaying && !engine.GetdataBase().empty()) {
            engine.PlayMix();
            m_playheadTimer->start(16);
            m_isPlaying = true;
            emit isPlayingChanged();
            qDebug() << "Resumed playback after deleting track";
        }
    }
    else {
        qWarning() << "Invalid track index for deletion:" << trackIndex;
    }
}

Q_INVOKABLE void ViewModel::deleteClip(int trackIndex, int clipindex)
{
    if (trackIndex >= 0 && trackIndex < engine.GetdataBase().size()) {
        if (clipindex >= 0 && clipindex < engine.GetdataBase()[trackIndex].clips.size()) {
            bool wasPlaying = isPlaying();
            if (wasPlaying) {
                engine.StopMix();
                m_playheadTimer->stop();
                m_isPlaying = false;
                emit isPlayingChanged();
                qDebug() << "Stopped playback before deleting track";
            }

			ClipModel* clipmodel = m_trackModel->getClipModel(trackIndex);
            clipmodel->deleteClip(clipindex);
			engine.DeleteClip(trackIndex, clipindex);

            if (wasPlaying && !engine.GetdataBase().empty()) {
                engine.PlayMix();
                m_playheadTimer->start(16);
                m_isPlaying = true;
                emit isPlayingChanged();
                qDebug() << "Resumed playback after deleting track";
            }

        }
        else {
			qWarning() << "Invalid clip index for deletion:" << clipindex;
        }
    }
    else {
        qWarning() << "Invalid track index for clip deletion:" << trackIndex;
    }
}

void ViewModel::addAudioTrack() {
    int newTrackIndex = engine.AddAudioTrack();
    
	if (newTrackIndex >= 0) {
		m_trackModel->addTrack("Audio", newTrackIndex);
		emit trackAdded(newTrackIndex);
	}
}

void ViewModel::addSamplerTrack() {
    int newTrackIndex = engine.AddSamplerTrack();
    if (newTrackIndex >= 0) {
        m_trackModel->addTrack("Sampler", newTrackIndex);
        emit trackAdded(newTrackIndex);
    }
}

Q_INVOKABLE void ViewModel::RenderToWave(QString path)
{
	std::string pathStr = path.toStdString();
	engine.RenderToFile(pathStr);
	qDebug() << "Render to file:" << path;
}

Q_INVOKABLE void ViewModel::SaveProject(QString path)
{
    const std::string pathStr = path.toStdString();
    engine.SaveProject(pathStr);
    qDebug() << "Save Proj to file:" << path;
}

Q_INVOKABLE void ViewModel::OpenProject(QString path)
{
    const std::string pathStr = path.toStdString();
    qDebug() << "Trying to open Proj in file:" << path;

    if (engine.LoadProject(pathStr)) {
        buildModel();
        m_trackModel->update();
        qDebug() << "File finnaly opened" << path;
    }
    

}

void ViewModel::addMidiTrack() {
    int newTrackIndex = engine.AddMidiTrack();
    if (newTrackIndex >= 0) {
        m_trackModel->addTrack("Midi", newTrackIndex);
        emit trackAdded(newTrackIndex);
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