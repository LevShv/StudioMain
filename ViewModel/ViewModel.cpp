#include "ViewModel.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QDesktopServices>


ViewModel::ViewModel(QObject* parent) : QObject(parent) {

    m_trackModel = new TrackModel(engine, this);
    m_midiModel = new MidiMessageModel(engine, this);

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

void ViewModel::addMidiClip(int trackIndex, double startTime)
{
    if (engine.AddMidiClip(trackIndex, startTime)) {
        ClipModel* clipModel = m_trackModel->getClipModel(trackIndex);
        if (clipModel) {
            clipModel->addClip(engine.GetdataBase()[trackIndex].clips.back()); // Уведомляем о новом клипе
        }
        emit clipAdded(trackIndex);
    }
    
}

void ViewModel::AddCloneClip(int trackIndex, int masterClipIndex, double startBeats)
{
    
    engine.AddCloneClip(trackIndex, masterClipIndex, startBeats);
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

        // Устанавливаем флаги для стандартного окна с заголовком и кнопками
        pluginWindow->setFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint);

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

void ViewModel::deleteClip(int trackIndex, int clipindex)
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

void ViewModel::deleteClips(const QVariantList& clips) {
    qDebug() << "deleteClips called with" << clips.size() << "clips";

    // Проверяем, проигрывается ли проект
    bool wasPlaying = isPlaying();
    if (wasPlaying) {
        engine.StopMix();
        m_playheadTimer->stop();
        m_isPlaying = false;
        emit isPlayingChanged();
        qDebug() << "Stopped playback before deleting clips";
    }

    // Группируем клипы по trackIndex для оптимизации
    QMap<int, QList<int>> clipsByTrack;
    for (const QVariant& clipVar : clips) {
        QVariantMap clipMap = clipVar.toMap();
        int trackIndex = clipMap["trackIndex"].toInt();
        int clipIndex = clipMap["clipIndex"].toInt();
        clipsByTrack[trackIndex].append(clipIndex);
    }

    // Обрабатываем каждый трек
    for (auto it = clipsByTrack.constBegin(); it != clipsByTrack.constEnd(); ++it) {
        int trackIndex = it.key();
        QList<int> clipIndices = it.value();

        if (trackIndex < 0 || trackIndex >= engine.GetdataBase().size()) {
            qWarning() << "Invalid track index for clip deletion:" << trackIndex;
            continue;
        }

        // Сортируем индексы клипов в обратном порядке, чтобы избежать проблем со сдвигом
        std::sort(clipIndices.begin(), clipIndices.end(), std::greater<int>());

        ClipModel* clipModel = m_trackModel->getClipModel(trackIndex);
        if (!clipModel) {
            qWarning() << "No ClipModel found for track:" << trackIndex;
            continue;
        }

        // Удаляем клипы
        for (int clipIndex : clipIndices) {
            if (clipIndex < 0 || clipIndex >= engine.GetdataBase()[trackIndex].clips.size()) {
                qWarning() << "Invalid clip index for deletion: trackIndex=" << trackIndex << "clipIndex=" << clipIndex;
                continue;
            }

            clipModel->deleteClip(clipIndex);
            engine.DeleteClip(trackIndex, clipIndex);
            qDebug() << "Deleted clip: trackIndex=" << trackIndex << "clipIndex=" << clipIndex;
        }
    }

    // Возобновляем воспроизведение, если оно было активно
    if (wasPlaying && !engine.GetdataBase().empty()) {
        engine.PlayMix();
        m_playheadTimer->start(16);
        m_isPlaying = true;
        emit isPlayingChanged();
        qDebug() << "Resumed playback after deleting clips";
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

void ViewModel::changeClipDuration(int trackIndex, int clipIndex, double newDuration)
{
    if (trackIndex < 0 || trackIndex >= engine.GetdataBase().size() ||
        clipIndex < 0 || clipIndex >= engine.GetdataBase()[trackIndex].clips.size()) {
        qWarning() << "Invalid track or clip index for duration change: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex;
        return;
    }

    if (newDuration <= 0.0) {
        qWarning() << "Invalid duration: " << newDuration << ", duration must be positive";
        return;
    }

    engine.ChangeDuration(trackIndex, clipIndex, newDuration);

    // Обновляем модель клипа
    ClipModel* clipModel = m_trackModel->getClipModel(trackIndex);
    if (clipModel) {
        clipModel->updateClip(clipIndex); // Уведомляем ClipModel об изменении
    }

    emit clipDurationChanged(trackIndex, clipIndex, newDuration);
    qDebug() << "Clip duration changed: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex << ", newDuration=" << newDuration << "beats";
}

Q_INVOKABLE QString ViewModel::applicationHomeFolder() const
{
    // Получаем директорию, где находится исполняемый файл
    QString appDir = QCoreApplication::applicationDirPath();

    // Формируем путь к HomeLeTo
    QString homePath = QDir::cleanPath(appDir + "/HomeLeTo");

    // Проверяем существование папки
    QDir dir(homePath);
    if (!dir.exists()) {
        qWarning() << "HomeLeTo directory does not exist:" << homePath;
        // Если папки нет, возвращаем рабочую директорию приложения
        return appDir;
    }

    LOG_INFO("Application home folder: " + homePath.toStdString());
    return homePath;

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

void ViewModel::addMidiNote(int trackIndex, int clipIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    // Проверяем валидность параметров
    if (trackIndex < 0 || clipIndex < 0 || noteNumber < 0 || noteNumber > 127 ||
        startBeats < 0 || durationBeats <= 0 || velocity < 0 || velocity > 1.0 || channel < 1 || channel > 16) {
        qWarning() << "ViewModel: Invalid note parameters: noteNumber=" << noteNumber
            << "startBeats=" << startBeats << "durationBeats=" << durationBeats
            << "velocity=" << velocity << "channel=" << channel;
        return;
    }

    // Обновляем индексы в midiModel
    m_midiModel->setTrackIndex(trackIndex);
    m_midiModel->setClipIndex(clipIndex);

    // Проверяем валидность индексов
    const auto& database = engine.GetdataBase();
    if (trackIndex >= database.size() || clipIndex >= database[trackIndex].clips.size()) {
        qWarning() << "ViewModel: Invalid trackIndex=" << trackIndex << "or clipIndex=" << clipIndex;
        return;
    }

    // Проверяем, является ли клип MidiClip
    if (!dynamic_cast<Engine::MidiClip*>(database[trackIndex].clips[clipIndex].get())) {
        qWarning() << "ViewModel: Clip at trackIndex=" << trackIndex << "clipIndex=" << clipIndex << "is not a MidiClip";
        return;
    }

    // Добавляем ноту в Engine
    engine.AddMidiNote(trackIndex, clipIndex, noteNumber, startBeats, durationBeats, velocity, channel);

    // Синхронизируем модель с Engine
    m_midiModel->refresh();
    qDebug() << "ViewModel: Added MIDI note: trackIndex=" << trackIndex << "clipIndex=" << clipIndex
        << "noteNumber=" << noteNumber << "startBeats=" << startBeats << "velocity=" << velocity;
}

void ViewModel::deleteMidiNote(int trackIndex, int clipIndex, int index) {
    if (trackIndex == m_midiModel->trackIndex() && clipIndex == m_midiModel->clipIndex()) {
        engine.DeleteMidiNote(trackIndex, clipIndex, index);
        m_midiModel->deleteNote(index);
        qDebug() << "ViewModel: Deleted MIDI note at index=" << index << "trackIndex=" << trackIndex << "clipIndex=" << clipIndex;
    }
    else {
        qWarning() << "Cannot delete note: trackIndex=" << trackIndex << "or clipIndex=" << clipIndex << "does not match midiModel";
    }
}

void ViewModel::updateMidiNote(int trackIndex, int clipIndex, int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (trackIndex == m_midiModel->trackIndex() && clipIndex == m_midiModel->clipIndex()) {
        engine.UpdateMidiNote(trackIndex, clipIndex, index, noteNumber, startBeats, durationBeats, velocity, channel);
        m_midiModel->updateNote(index, noteNumber, startBeats, durationBeats, velocity, channel);
        qDebug() << "ViewModel: Updated MIDI note at index=" << index << "trackIndex=" << trackIndex << "clipIndex=" << clipIndex << "noteNumber=" << noteNumber;
    }
    else {
        qWarning() << "Cannot update note: trackIndex=" << trackIndex << "or clipIndex=" << clipIndex << "does not match midiModel";
    }
}