#include "ClipModel.h"
#include <QDebug>

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
            // Проверяем кэш
            if (m_waveformDataCache.contains(clipIndex)) {
                return m_waveformDataCache[clipIndex];
            }

            QVariantList waveformData;
            const auto& buffer = audioClip->buffer;
            int numSamples = buffer.getNumSamples();
            int numChannels = buffer.getNumChannels();
            int sampleCount = 100;

            if (numSamples == 0) {
                qDebug() << "Empty audio buffer for clip:" << clipIndex;
                return waveformData;
            }

            int step = numSamples / sampleCount;
            if (step < 1) step = 1;

            for (int i = 0; i < sampleCount && i * step < numSamples; ++i) {
                float maxAmplitude = 0.0f;
                for (int j = 0; j < step; ++j) {
                    int sampleIdx = i * step + j;
                    float amplitude = 0.0f;
                    for (int c = 0; c < numChannels; ++c) {
                        if (sampleIdx < numSamples) {
                            amplitude += std::abs(buffer.getSample(c, sampleIdx));
                        }
                    }
                    amplitude /= numChannels;
                    maxAmplitude = std::max(maxAmplitude, amplitude);
                }
                waveformData.append(maxAmplitude);
            }

            // Сохраняем в кэш
            m_waveformDataCache[clipIndex] = waveformData;
            qDebug() << "Computed waveform data for clip" << clipIndex << ":" << waveformData.size() << "points";
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