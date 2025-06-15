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
        qDebug() << "ClipModel::data: trackIndex:" << m_trackIndex << "clipIndex:" << clipIndex << "startBeats:" << clip->startBeats;
        return clip->startBeats;
    case DurationBeatsRole:
        return clip->durationBeats;
    case ClipTypeRole:
        if (dynamic_cast<Engine::AudioClip*>(clip.get()))
            return "audio";
        if (auto cloneClip = dynamic_cast<Engine::CloneClip*>(clip.get())) {
            if (dynamic_cast<Engine::AudioClip*>(cloneClip->masterClip))
                return "audio";
            return track.isSamplerTrack ? "sampler" : "midi";
        }
        return track.isSamplerTrack ? "sampler" : "midi";
    case FilePathRole:
        if (auto audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
            return QString::fromUtf8(
                audioClip->file.getFullPathName().toRawUTF8(),
                audioClip->file.getFullPathName().getNumBytesAsUTF8()
            );
        }
        else if (auto cloneClip = dynamic_cast<Engine::CloneClip*>(clip.get())) {
            if (auto masterAudioClip = dynamic_cast<Engine::AudioClip*>(cloneClip->masterClip)) {
                return QString::fromUtf8(
                    masterAudioClip->file.getFullPathName().toRawUTF8(),
                    masterAudioClip->file.getFullPathName().getNumBytesAsUTF8()
                );
            }
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
        else if (auto cloneClip = dynamic_cast<Engine::CloneClip*>(clip.get())) {
            if (auto masterAudioClip = dynamic_cast<Engine::AudioClip*>(cloneClip->masterClip)) {
                QVariantList waveformData;
                for (float amplitude : masterAudioClip->waveformData) {
                    waveformData.append(amplitude);
                }
                return waveformData;
            }
        }
        return QVariant();
    case MasterClipIndexRole:
        if (auto* cloneClip = dynamic_cast<Engine::CloneClip*>(clip.get())) {
            for (size_t i = 0; i < track.clips.size(); ++i) {
                if (track.clips[i]->clipID == cloneClip->masterClipID) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }
        return -1;
    case Color: 
        if (auto* clipBase = dynamic_cast<Engine::ClipBase*>(clip.get())) {
            return QString::fromStdString(clipBase->color);
        }
        return -1;
    
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
    roles[MasterClipIndexRole] = "masterClipIndex";
    roles[Color] = "color";
    return roles;
}

void ClipModel::addClip(const Engine::ClipPtr& clip) {
    int newIndex = m_engine.GetdataBase()[m_trackIndex].clips.size() - 1;
    beginInsertRows(QModelIndex(), newIndex, newIndex);
    endInsertRows();
}

void ClipModel::updateClip(int clipIndex)
{
    m_waveformDataCache.remove(clipIndex);

    const auto& tracks = m_engine.GetdataBase();
    if (m_trackIndex < 0 || m_trackIndex >= tracks.size() || clipIndex < 0 || clipIndex >= tracks[m_trackIndex].clips.size()) {
        qDebug() << "Invalid track or clip index in updateClip:" << m_trackIndex << clipIndex;
        return;
    }

    const auto& clip = tracks[m_trackIndex].clips[clipIndex];
    std::string masterClipID = clip->clipID;
    {
        QModelIndex idx = createIndex(clipIndex, 0);
        emit dataChanged(idx, idx, { StartBeatsRole, DurationBeatsRole, ClipTypeRole, FilePathRole, WaveformDataRole, Color });
        qDebug() << "ClipModel::updateClip called for master clip at trackIndex:" << m_trackIndex << ", clipIndex:" << clipIndex;
    }

    for (int i = 0; i < tracks[m_trackIndex].clips.size(); ++i) {
        if (i != clipIndex) {
            const auto& otherClip = tracks[m_trackIndex].clips[i];
            if (auto* cloneClip = dynamic_cast<Engine::CloneClip*>(otherClip.get())) {
                if (cloneClip->masterClipID == masterClipID) {
                    QModelIndex cloneIdx = createIndex(i, 0);
                    emit dataChanged(cloneIdx, cloneIdx, { StartBeatsRole, DurationBeatsRole, ClipTypeRole, FilePathRole, WaveformDataRole, Color });
                    qDebug() << "ClipModel::updateClip called for clone at trackIndex:" << m_trackIndex << ", clipIndex:" << i;
                }
            }
        }
    }
}

void ClipModel::setTrackIndex(int trackIndex)
{
    if (m_trackIndex != trackIndex) {
        m_trackIndex = trackIndex;
        m_waveformDataCache.clear();
        qDebug() << "ClipModel trackIndex changed to:" << m_trackIndex;
        if (rowCount() > 0) {
            QModelIndex topLeft = createIndex(0, 0);
            QModelIndex bottomRight = createIndex(rowCount() - 1, 0);
            emit dataChanged(topLeft, bottomRight, { StartBeatsRole, DurationBeatsRole, ClipTypeRole, FilePathRole, WaveformDataRole });
        }
    }
}

void ClipModel::deleteClip(int clipIndex) {
    m_waveformDataCache.remove(clipIndex);
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
    Engine::AudioClip* audioClip = nullptr;

    if (auto directAudioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
        audioClip = directAudioClip;
    }
    else if (auto cloneClip = dynamic_cast<Engine::CloneClip*>(clip.get())) {
        audioClip = dynamic_cast<Engine::AudioClip*>(cloneClip->masterClip);
    }

    if (audioClip && !audioClip->waveformData.empty()) {
        const std::vector<int> targetWidths = { 100, 200, 400, 800, 1600 };
        int targetWidth = targetWidths[0];
        int minDiff = std::abs(width - targetWidth);
        for (int w : targetWidths) {
            int diff = std::abs(width - w);
            if (diff < minDiff) {
                minDiff = diff;
                targetWidth = w;
            }
        }

        QDir projectDir(QDir::currentPath() + "/waveforms");
        if (!projectDir.exists()) {
            if (!projectDir.mkpath(".")) {
                qDebug() << "Failed to create waveforms directory:" << projectDir.absolutePath();
                return "";
            }
            qDebug() << "Created waveforms directory:" << projectDir.absolutePath();
        }

        QString fileName = QString("waveform_%1_%2.png").arg(QString::fromStdString(audioClip->clipID)).arg(targetWidth);
        QString filePath = projectDir.absoluteFilePath(fileName);

        QFileInfo fileInfo(filePath);
        if (fileInfo.exists()) {
            qDebug() << "Using existing waveform image for clip ID:" << audioClip->clipID.c_str() << "at" << filePath;
            return QUrl::fromLocalFile(filePath).toString();
        }

        QStringList existingFiles = projectDir.entryList(
            QStringList() << QString("waveform_%1_*.png").arg(QString::fromStdString(audioClip->clipID)),
            QDir::Files, QDir::Name
        );
        if (existingFiles.size() >= 5) {
            QString oldestFile = projectDir.absoluteFilePath(existingFiles.first());
            QFile::remove(oldestFile);
            qDebug() << "Removed oldest waveform image:" << oldestFile;
        }

        QImage image(targetWidth, height, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setPen(QPen(Qt::white, 1));
        painter.setRenderHint(QPainter::Antialiasing);

        int numPoints = std::min((int)audioClip->waveformData.size(), targetWidth);
        float step = numPoints > 1 ? static_cast<float>(targetWidth) / (numPoints - 1) : targetWidth;
        float centerY = height / 2.0f;
        float maxHeight = height * 0.8f / 2.0f;

        QPainterPath waveformPath;
        waveformPath.moveTo(0, centerY);
        for (int i = 0; i < numPoints; ++i) {
            float x = i * step;
            float amplitude = audioClip->waveformData[i * audioClip->waveformData.size() / numPoints] * maxHeight * 2;
            waveformPath.lineTo(x, centerY - amplitude);
        }
        for (int i = numPoints - 1; i >= 0; --i) {
            float x = i * step;
            float amplitude = audioClip->waveformData[i * audioClip->waveformData.size() / numPoints] * maxHeight * 2;
            waveformPath.lineTo(x, centerY + amplitude);
        }
        waveformPath.closeSubpath();
        painter.drawPath(waveformPath);

        if (!image.save(filePath)) {
            qDebug() << "Failed to save waveform image for clip ID:" << audioClip->clipID.c_str() << "at" << filePath;
            return "";
        }

        qDebug() << "Waveform image saved for clip ID:" << audioClip->clipID.c_str() << "at" << filePath;
        QString url = QUrl::fromLocalFile(filePath).toString();
        qDebug() << "Returning URL for clip ID:" << audioClip->clipID.c_str() << "url:" << url;
        return url;
    }
    qDebug() << "No waveform data for clip:" << clipIndex;
    return "";
}