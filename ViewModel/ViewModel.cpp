#include "ViewModel.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QDesktopServices>

ViewModel::ViewModel(QObject* parent) : QObject(parent) {
    m_currentProjectPath = ""; // Изначально путь пустой
    m_trackModel = new TrackModel(engine, this);
    m_midiModel  = new MidiMessageModel(engine, this);
    m_pluginModel = new PluginModel(engine, this);

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

void ViewModel::setBpm(double newBpm)
{
    engine.SetBPM(newBpm);
}

void ViewModel::moveClip(size_t trackIdx, size_t clipIdx, double newStartTime) {
    engine.MoveClip(static_cast<int>(trackIdx), static_cast<int>(clipIdx), newStartTime);
    ClipModel* clipModel = m_trackModel->getClipModel(static_cast<int>(trackIdx));
    if (clipModel) {
        clipModel->updateClip(static_cast<int>(clipIdx)); // Уведомляем ClipModel об изменении
    }
    // Синхронизируем позицию курсора в UI
    double newPlayheadPosition = engine.GetPlayheadPosition();
    if (std::abs(newPlayheadPosition - m_playheadPosition) > 0.001) {
        m_playheadPosition = newPlayheadPosition;
        emit playheadPositionChanged(m_playheadPosition);
        qDebug() << "Updated playhead position after moving clip: " << m_playheadPosition << " beats";
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

void ViewModel::setTrackGain(int trackIndex, float gain)
{
    if (trackIndex < 0 || trackIndex >= engine.GetdataBase().size()) {
        qWarning() << "Invalid track index for gain change:" << trackIndex;
        return;
    }

    engine.SetTrackGain(trackIndex, gain);
   // m_trackModel->update(); // Обновляем модель для синхронизации UI
    emit trackGainChanged(trackIndex, gain);
    qDebug() << "Track" << trackIndex << "gain set to" << gain;
}

void ViewModel::setTrackMute(int index, bool muted)
{
    engine.SetTrackMute(index, muted);
}

void ViewModel::toggleSolo(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= engine.GetdataBase().size()) {
        qWarning() << "Invalid track index for gain change:" << trackIndex;
        return;
    }
    
    engine.ToggleSolo(trackIndex);
}

void ViewModel::setName(int trackIndex, QString name)
{
    if (trackIndex < 0 || trackIndex >= engine.GetdataBase().size()) {
        qWarning() << "Invalid track index for name change:" << trackIndex;
        return;
    }

    engine.setName(trackIndex, name.toStdString());
}

void ViewModel::RenderToWave(QString path)
{
	std::string pathStr = path.toStdString();
	engine.RenderToFile(pathStr);
	qDebug() << "Render to file:" << path;
}

void ViewModel::SaveProject(QString path)
{
    const std::string pathStr = path.toStdString();
    engine.SaveProject(pathStr);
    setCurrentProjectPath(path); // Обновляем путь
    qDebug() << "Save Proj to file:" << path;
}

void ViewModel::OpenProject(QString path)
{
    const std::string pathStr = path.toStdString();
    qDebug() << "Trying to open Proj in file:" << path;

    if (engine.LoadProject(pathStr)) {
        buildModel();
        m_trackModel->update();
        setCurrentProjectPath(path); // Обновляем путь
        qDebug() << "File finnaly opened" << path;
    }
    

}

void ViewModel::createNewProject()
{
    if (isPlaying()) {
        engine.StopMix();
        m_playheadTimer->stop();
        m_isPlaying = false;
        emit isPlayingChanged();
        qDebug() << "Stopped playback for new project creation";
    }

    // Создаём новый проект
    engine.CreateNewProject();

    // Обновляем модели
    buildModel();
    m_trackModel->update(); // Обновляем модель треков
    m_midiModel->refresh(); // Используем refresh вместо update
    m_pluginModel->refresh(); // Используем refresh вместо update

    // Сбрасываем позицию курсора и BPM в UI
    m_playheadPosition = 0.0;
    m_bpm = engine.GetBPM();
    emit playheadPositionChanged(m_playheadPosition);
    emit bpmChanged();
    setCurrentProjectPath(""); // Сбрасываем путь
    // Сбрасываем текущие индексы в MidiMessageModel для предотвращения ошибок
    m_midiModel->setTrackIndex(-1);
    m_midiModel->setClipIndex(-1);

    // Убедимся, что UI переключен в SONG mode
    qDebug() << "New project created, UI updated: bpm=" << m_bpm << ", playheadPosition=" << m_playheadPosition;
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

    ClipModel* clipModel = m_trackModel->getClipModel(trackIndex);
    if (clipModel) {
        clipModel->updateClip(clipIndex); // Уведомляем ClipModel об изменении
    }

    emit clipDurationChanged(trackIndex, clipIndex, newDuration);
    qDebug() << "Clip duration changed: trackIndex=" << trackIndex << ", clipIndex=" << clipIndex << ", newDuration=" << newDuration << "beats";
}

void ViewModel::changeColor(int trackIndex, int clipIndex, const QColor& color)
{
    std::string colorStr = color.name(QColor::HexRgb).toStdString();

    // Вызываем метод engine.changeColor
    engine.ChangeColor(trackIndex, clipIndex, colorStr);
    ClipModel* clipModel = m_trackModel->getClipModel(trackIndex);
    if (clipModel) {
        clipModel->updateClip(clipIndex); // Уведомляем ClipModel об изменении
    }
    
}

QString ViewModel::applicationHomeFolder() const
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

void ViewModel::enableLoopMode(int trackIndex, int clipIndex)
{
    bool wasPlaying = isPlaying();
    if (wasPlaying) {
        engine.StopMix();
        m_playheadTimer->stop();
        m_isPlaying = false;
        emit isPlayingChanged();
        qDebug() << "Stopped playback before deleting track";
    }

    engine.EnableLoopMode(trackIndex, clipIndex);
    setPlayheadPosition(engine.GetPlayheadPosition());

    if (wasPlaying && !engine.GetdataBase().empty()) {
        engine.PlayMix();
        m_playheadTimer->start(16);
        m_isPlaying = true;
        emit isPlayingChanged();
        qDebug() << "Resumed playback after deleting track";
    }

}

void ViewModel::disableLoopMode()
{
    engine.DisableLoopMode();
}

void ViewModel::setUserVolume(float volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        engine.SetUserVolume(volume);
        emit volumeChanged();
    }
}

float ViewModel::getUserVolume()
{
    return engine.GetUserVolume();
}

void ViewModel::setMasterGain(float volume)
{
    if (m_masterGain != volume) {
        m_masterGain = volume;
        engine.SetUserVolume(volume);
        emit volumeChanged();
    }
}

float ViewModel::getMasterGain()
{
    return engine.GetMasterGain();
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

void ViewModel::stopDoplay(void(*func)(...))
{
    bool wasPlaying = isPlaying();
    if (wasPlaying) {
        engine.StopMix();
        m_playheadTimer->stop();
        m_isPlaying = false;
        emit isPlayingChanged();
        qDebug() << "Stopped playback before deleting track";
    }

    func();

    if (wasPlaying && !engine.GetdataBase().empty()) {
        engine.PlayMix();
        m_playheadTimer->start(16);
        m_isPlaying = true;
        emit isPlayingChanged();
        qDebug() << "Resumed playback after deleting track";
    }
}

void ViewModel::setCurrentProjectPath(const QString& path) {
    if (m_currentProjectPath != path) {
        m_currentProjectPath = path;
        emit currentProjectPathChanged();
        qDebug() << "Current project path updated:" << m_currentProjectPath;
    }
}

void ViewModel::prepareForExit() {
    if (isPlaying()) {
        engine.StopMix();
        m_playheadTimer->stop();
        m_isPlaying = false;
        emit isPlayingChanged();
        qDebug() << "Stopped playback before application exit";
    }
    // Дополнительная очистка, если требуется
}