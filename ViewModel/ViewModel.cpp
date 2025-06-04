#include "ViewModel.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QDesktopServices>
#include <juce_gui_basics/juce_gui_basics.h>
#include <windows.h>


ViewModel::ViewModel(QObject* parent) : QObject(parent) {

    m_trackModel = new TrackModel(engine, this);
    m_midiModel = new MidiMessageModel(engine, this);
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
    qDebug() << "ViewModel: Adding plugin to track" << trackIndex << "path:" << pluginPath;
    engine.AddPluginToTrack(trackIndex, pluginPath.toStdString());
    qDebug() << "ViewModel: Plugin added to Engine for track" << trackIndex;
    emit pluginAdded(trackIndex);
    qDebug() << "ViewModel: Emitted pluginAdded for track" << trackIndex;
}

void ViewModel::togglePluginBypass(int trackIndex, int pluginIndex) {
    engine.TogglePluginBypass(trackIndex, pluginIndex);
    emit pluginBypassed(trackIndex, pluginIndex);
}

void ViewModel::deletePlugin(int trackIndex, int pluginIndex) {
    if (trackIndex >= 0 && trackIndex < engine.GetdataBase().size()) {
        // Проверяем, открыт ли редактор плагина
        QPair<int, int> key = { trackIndex, pluginIndex };
        if (m_openPluginEditors.contains(key)) {
            juce::Component* component = m_openPluginEditors[key];
            if (component) {
                // Удаляем компонент с рабочего стола
                if (component->isOnDesktop()) {
                    component->removeFromDesktop();
                    qDebug() << "Plugin editor removed from desktop: track=" << trackIndex << ", plugin=" << pluginIndex;
                }
                // Очищаем WindowProc и данные
                HWND hwnd = (HWND)component->getWindowHandle();
                if (hwnd) {
                    WindowData* data = reinterpret_cast<WindowData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                    if (data) {
                        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)data->originalProc);
                        delete data;
                        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
                    }
                }
                // Удаляем из отслеживания
                m_openPluginEditors.remove(key);
                qDebug() << "Plugin editor removed from tracking: track=" << trackIndex << ", plugin=" << pluginIndex;
            }
        }

        // Удаляем плагин из движка
        engine.RemovePluginFromTrack(trackIndex, pluginIndex);
        m_pluginModel->refresh();
        emit pluginRemoved(trackIndex, pluginIndex);
        qDebug() << "Plugin deleted: trackIndex=" << trackIndex << ", pluginIndex=" << pluginIndex;
    }
    else {
        qWarning() << "Invalid track index for plugin deletion:" << trackIndex;
    }
    emit pluginAdded(trackIndex); // Это может быть ошибкой, возможно, стоит убрать или заменить на pluginRemoved
}
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ViewModel::WindowData* data = reinterpret_cast<ViewModel::WindowData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (data && data->editors->contains(data->key)) {
        if (msg == WM_CLOSE) {
            data->component->setVisible(false);
            qDebug() << "Plugin editor hidden via WM_CLOSE: track=" << data->key.first << ", plugin=" << data->key.second;
            return 0; // Предотвращаем закрытие
        }
        if (msg == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_CLOSE) {
            data->component->setVisible(false);
            qDebug() << "Plugin editor hidden via WM_SYSCOMMAND (SC_CLOSE): track=" << data->key.first << ", plugin=" << data->key.second;
            return 0; // Блокируем команду закрытия
        }
    }
    return CallWindowProc(data ? data->originalProc : DefWindowProc, hwnd, msg, wParam, lParam);
}

void ViewModel::openPluginEditor(int trackIndex, int pluginIndex) {
    qDebug() << "Attempting to open plugin editor: track=" << trackIndex << ", plugin=" << pluginIndex;

    QPair<int, int> key = { trackIndex, pluginIndex };
    // Проверяем, существует ли редактор
    if (m_openPluginEditors.contains(key)) {
        auto* existingComponent = m_openPluginEditors[key];
        if (existingComponent->isVisible()) {
            qDebug() << "Plugin editor is already visible, bringing to front: track=" << trackIndex << ", plugin=" << pluginIndex;
            existingComponent->toFront(true);
            emit pluginEditorOpened(trackIndex, pluginIndex, nullptr);
            return;
        }
        else {
            qDebug() << "Plugin editor exists but is hidden, showing: track=" << trackIndex << ", plugin=" << pluginIndex;
            existingComponent->setVisible(true);
            existingComponent->toFront(true);
            existingComponent->repaint();
            if (!existingComponent->isOnDesktop()) {
                existingComponent->addToDesktop(juce::ComponentPeer::windowHasTitleBar);
                qDebug() << "Re-added component to desktop: track=" << trackIndex << ", plugin=" << pluginIndex;

                // Повторно настраиваем окно
                HWND hwnd = (HWND)existingComponent->getWindowHandle();
                if (hwnd) {
                    LONG style = GetWindowLong(hwnd, GWL_STYLE);
                    style |= WS_SYSMENU | WS_MINIMIZEBOX; // Включаем системное меню и кнопку минимизации
                    SetWindowLong(hwnd, GWL_STYLE, style);
                    // Отключаем команду закрытия в системном меню
                    HMENU hMenu = GetSystemMenu(hwnd, FALSE);
                    if (hMenu) {
                        EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
                    }
                    qDebug() << "Enabled minimize button and disabled close button: track=" << trackIndex << ", plugin=" << pluginIndex;
                }
            }
            emit pluginEditorOpened(trackIndex, pluginIndex, nullptr);
            return;
        }
    }

    if (auto* editor = engine.GetPluginEditor(trackIndex, pluginIndex)) {
        qDebug() << "Editor retrieved: address=" << (void*)editor << ", type=" << typeid(*editor).name();
        auto* component = dynamic_cast<juce::Component*>(editor);
        if (component) {
            // Проверяем размеры компонента
            int width = component->getWidth();
            int height = component->getHeight();
            if (width == 0 || height == 0) {
                qWarning() << "Component has invalid size, setting default: width=400, height=300";
                width = 400;
                height = 300;
            }

            // Добавляем JUCE Component на рабочий стол
            component->addToDesktop(juce::ComponentPeer::windowHasTitleBar);
            qDebug() << "Component added to desktop: isOnDesktop=" << component->isOnDesktop();

            // Устанавливаем видимость и границы
            component->setVisible(true);
            component->setBounds(100, 100, width, height);
            component->toFront(true);
            component->repaint();
            qDebug() << "Component set visible: isVisible=" << component->isVisible();

            // Отслеживаем компонент
            m_openPluginEditors[key] = component;

            // Настройка окна для Windows
            HWND hwnd = (HWND)component->getWindowHandle();
            if (hwnd) {
                // Настраиваем стиль окна
                LONG style = GetWindowLong(hwnd, GWL_STYLE);
                style |= WS_SYSMENU | WS_MINIMIZEBOX; // Включаем системное меню и кнопку минимизации
                SetWindowLong(hwnd, GWL_STYLE, style);
                // Отключаем команду закрытия в системном меню
                HMENU hMenu = GetSystemMenu(hwnd, FALSE);
                if (hMenu) {
                    EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
                }
                // Обновляем рамку окна
                SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                qDebug() << "Enabled minimize button and disabled close button: track=" << trackIndex << ", plugin=" << pluginIndex;

                // Сохраняем данные
                auto* windowData = new WindowData{ component, key, &m_openPluginEditors, nullptr };
                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)windowData);

                // Сохраняем оригинальную WindowProc
                windowData->originalProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);

                // Устанавливаем WindowProc
                SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)windowProc);
                qDebug() << "Custom WindowProc set for Windows: track=" << trackIndex << ", plugin=" << pluginIndex;
            }
            else {
                qWarning() << "Failed to get window handle: track=" << trackIndex << ", plugin=" << pluginIndex;
            }

            // Таймер для проверки состояния
            QTimer* visibilityTimer = new QTimer(this);
            visibilityTimer->setInterval(500);
            QObject::connect(visibilityTimer, &QTimer::timeout, [=]() {
                if (component && m_openPluginEditors.contains(key)) {
                    if (!component->isVisible() || !component->isOnDesktop()) {
                        component->setVisible(false);
                        qDebug() << "Plugin editor hidden via timer check: track=" << trackIndex << ", plugin=" << pluginIndex;
                        visibilityTimer->stop();
                    }
                }
                });
            visibilityTimer->start();

            // Очистка при выходе
            QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [=]() {
                if (component && component->isOnDesktop()) {
                    component->removeFromDesktop();
                    qDebug() << "JUCE Component removed from desktop on app quit: track=" << trackIndex << ", plugin=" << pluginIndex;
                }
                if (m_openPluginEditors.contains(key)) {
                    m_openPluginEditors.remove(key);
                    qDebug() << "Plugin editor removed from tracking on app quit: track=" << trackIndex << ", plugin=" << pluginIndex;
                }
                visibilityTimer->stop();
                if (hwnd) {
                    WindowData* data = reinterpret_cast<WindowData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                    if (data) {
                        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)data->originalProc);
                        delete data;
                        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
                    }
                }
                });

            qDebug() << "Plugin editor opened: track=" << trackIndex << ", plugin=" << pluginIndex
                << ", editor=" << (void*)editor;
            emit pluginEditorOpened(trackIndex, pluginIndex, nullptr);
        }
        else {
            qWarning() << "Failed to cast editor to JUCE Component, type=" << typeid(*editor).name();
        }
    }
    else {
        qWarning() << "Failed to open plugin editor for track" << trackIndex << ", plugin=" << pluginIndex;
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

