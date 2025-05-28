#include "ClipModel.h"
#include <QDebug>
#include <QImage>
#include <QPainterPath>
#include <QPainter>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

ClipModel::ClipModel(Engine& engine, int trackIndex, QObject* parent)
    : QAbstractListModel(parent), m_engine(engine), m_trackIndex(trackIndex) {
}

int ClipModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    const auto& tracks = m_engine.GetdataBase();
    if (m_trackIndex < 0 || m_trackIndex >= tracks.size()) return 0;
    int count = static_cast<int>(tracks[m_trackIndex].clips.size());
    return count;
}



QVariant ClipModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    const auto& tracks = m_engine.GetdataBase();
    if (m_trackIndex < 0 || m_trackIndex >= tracks.size()) return QVariant();

    const auto& track = tracks[m_trackIndex];
    int clipIndex = index.row();
    if (clipIndex < 0 || clipIndex >= track.clips.size()) return QVariant();

    const auto& clip = track.clips[clipIndex];

    switch (role) {
    case StartBeatsRole:
        return clip->startBeats;
    case DurationBeatsRole:
        return clip->durationBeats;
    case ClipTypeRole:
        if (dynamic_cast<Engine::AudioClip*>(clip.get())) {
            return "audio";
        }
        return "midi";
    case FilePathRole:
        if (auto audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
            return QString::fromUtf8(
                audioClip->file.getFullPathName().toRawUTF8(),
                audioClip->file.getFullPathName().getNumBytesAsUTF8()
            );
        }
        return QVariant();
    case WaveformDataRole:
        if (auto audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
            QVariantList waveformData;
            for (float amplitude : audioClip->waveformData) {
                waveformData.append(amplitude);
            }
            return waveformData;
        }
        return QVariant();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ClipModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[StartBeatsRole] = "startBeats";
    roles[DurationBeatsRole] = "durationBeats";
    roles[ClipTypeRole] = "type";
    roles[FilePathRole] = "file";
    roles[WaveformDataRole] = "waveformData";
    return roles;
}

void ClipModel::addClip(const Engine::ClipPtr& clip) {
    int newIndex = m_engine.GetdataBase()[m_trackIndex].clips.size() - 1;
    beginInsertRows(QModelIndex(), newIndex, newIndex);
    endInsertRows();
}

void ClipModel::updateClip(int clipIndex) {
    // Очищаем кэш для обновлённого клипа
    m_waveformDataCache.remove(clipIndex);
    QModelIndex idx = createIndex(clipIndex, 0);
    emit dataChanged(idx, idx, { StartBeatsRole, DurationBeatsRole, ClipTypeRole, FilePathRole, WaveformDataRole });
}

void ClipModel::setTrackIndex(int trackIndex) {
    if (m_trackIndex != trackIndex) {
        m_trackIndex = trackIndex;
        m_waveformDataCache.clear(); // Очищаем кэш
        qDebug() << "ClipModel trackIndex changed to:" << m_trackIndex;
        beginResetModel();
        endResetModel();
    }
}

void ClipModel::deleteClip(int clipIndex) {
    m_waveformDataCache.remove(clipIndex); // Удаляем из кэша
    beginRemoveRows(QModelIndex(), clipIndex, clipIndex);
    endRemoveRows();
    qDebug() << "ClipModel deleted clip at index:" << clipIndex;
}

QString ClipModel::getWaveformImage(int clipIndex, int width, int height) {
    const auto& tracks = m_engine.GetdataBase();
    if (m_trackIndex < 0 || m_trackIndex >= tracks.size() || clipIndex < 0 || clipIndex >= tracks[m_trackIndex].clips.size()) {
        qDebug() << "Invalid track or clip index:" << m_trackIndex << clipIndex;
        return "";
    }

    const auto& clip = tracks[m_trackIndex].clips[clipIndex];
    if (auto audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
        if (audioClip->waveformData.empty()) {
            qDebug() << "No waveform data for clip:" << clipIndex;
            return "";
        }

        // Используем папку в пользовательской директории для надёжности
        QDir projectDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/waveforms");
        if (!projectDir.exists()) {
            if (!projectDir.mkpath(".")) {
                qDebug() << "Failed to create waveforms directory:" << projectDir.absolutePath();
                return "";
            }
            qDebug() << "Created waveforms directory:" << projectDir.absolutePath();
        }

        QString fileName = QString("waveform_%1_%2.png").arg(m_trackIndex).arg(clipIndex);
        QString filePath = projectDir.absoluteFilePath(fileName);

        // Проверяем, существует ли файл
        if (QFileInfo(filePath).exists()) {
            qDebug() << "Using existing waveform image for clip:" << clipIndex << "at" << filePath;
            return QUrl::fromLocalFile(filePath).toString();
        }

        QImage image(width, height, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setPen(QPen(Qt::white, 1));
        painter.setRenderHint(QPainter::Antialiasing);

        int numPoints = audioClip->waveformData.size();
        float step = static_cast<float>(width) / numPoints;
        float centerY = height / 2.0f;
        float maxHeight = height * 0.8f / 2.0f;

        QPainterPath waveformPath;
        waveformPath.moveTo(0, centerY);
        for (int i = 0; i < numPoints; ++i) {
            float x = i * step;
            float amplitude = audioClip->waveformData[i] * maxHeight;
            waveformPath.lineTo(x, centerY - amplitude);
        }
        for (int i = numPoints - 1; i >= 0; --i) {
            float x = i * step;
            float amplitude = audioClip->waveformData[i] * maxHeight;
            waveformPath.lineTo(x, centerY + amplitude);
        }
        waveformPath.closeSubpath();
        painter.drawPath(waveformPath);

        if (!image.save(filePath)) {
            qDebug() << "Failed to save waveform image for clip:" << clipIndex << "at" << filePath;
            return "";
        }

        qDebug() << "Waveform image saved for clip:" << clipIndex << "at" << filePath;
        return QUrl::fromLocalFile(filePath).toString();
    }
    return "";
}