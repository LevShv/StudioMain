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
    qDebug() << "ClipModel rowCount for track" << m_trackIndex << ": " << count;
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
    return roles;
}

void ClipModel::addClip(const Engine::ClipPtr& clip) {
    int newIndex = m_engine.GetdataBase()[m_trackIndex].clips.size() - 1; // Новый клип добавлен в конец
    beginInsertRows(QModelIndex(), newIndex, newIndex);
    // Данные уже добавлены в Engine, просто уведомляем QML
    endInsertRows();
}

void ClipModel::updateClip(int clipIndex) {
    QModelIndex idx = createIndex(clipIndex, 0);
    emit dataChanged(idx, idx, { StartBeatsRole, DurationBeatsRole, ClipTypeRole, FilePathRole });
}