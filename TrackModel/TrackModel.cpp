#include "TrackModel.h"
#include "ViewModel.h"
#include <QDebug>

TrackModel::TrackModel(Engine& engine, QObject* parent)
    : QAbstractListModel(parent), m_engine(engine) {

    const auto& tracks = m_engine.GetdataBase();
	m_rowCount = static_cast<int>(tracks.size());

    m_clipModels.clear();

    for (int i = 0; i < tracks.size(); ++i) {
        ensureClipModel(i);
        qDebug() << "Initialized ClipModel for track" << i;
    }
}

int TrackModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    qDebug() << "TrackModel rowCount:" << m_rowCount;
    return m_rowCount;
}

QVariant TrackModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rowCount)
        return QVariant();  // Важно!

    const auto& tracks = m_engine.GetdataBase();
    int row = index.row();

    if (row < 0 || row >= tracks.size()) {
        qWarning() << "Invalid track index:" << row;
        return QVariant();
    }

    const auto& track = tracks[row];
    //qDebug() << "Processing track" << row << "with" << track.clips.size() << "clips";

    switch (role) {
    case TrackIndexRole:
        return row;
    case ClipsModelRole: {
        return QVariant::fromValue(getClipModel(row));
    }
    case TrackTypeRole: {
        if (tracks[row].isMidiTrack) {
            return tracks[row].isSamplerTrack ? "Sampler" : "Midi";
        }
        return "Audio"; 
    }
    default:
        return QVariant();
    }
}

//QVariant TrackModel::clipData(int trackIndex, int clipIndex, int role) const {
//    const auto& tracks = m_engine.GetdataBase();
//    if (trackIndex < 0 || trackIndex >= tracks.size()) {
//        qWarning() << "Invalid track index in clipData:" << trackIndex;
//        return QVariant();
//    }
//
//    const auto& track = tracks[trackIndex];
//    if (clipIndex < 0 || clipIndex >= track.clips.size()) {
//        qWarning() << "Invalid clip index:" << clipIndex << "for track:" << trackIndex;
//        return QVariant();
//    }
//
//    const auto& clip = track.clips[clipIndex];
//
//    switch (role) {
//    case StartBeatsRole:
//        return clip->startBeats;
//    case DurationBeatsRole:
//        return clip->durationBeats;
//    case ClipTypeRole:
//        if (dynamic_cast<Engine::AudioClip*>(clip.get())) {
//            return "audio";
//        }
//        return "midi";
//    case FilePathRole:
//        if (auto audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
//            return QString::fromUtf8(
//                audioClip->file.getFullPathName().toRawUTF8(),
//                audioClip->file.getFullPathName().getNumBytesAsUTF8()
//            );
//        }
//        return QVariant();
//    default:
//        qWarning() << "Unknown role in clipData:" << role;
//        return QVariant();
//    }
//}

QHash<int, QByteArray> TrackModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[TrackIndexRole] = "trackIndex";
    roles[ClipsModelRole] = "clipsModel";
    roles[CountOfTracks] = "countOfTracks";
    roles[StartBeatsRole] = "startBeats";
    roles[DurationBeatsRole] = "durationBeats";
    roles[ClipTypeRole] = "type";
    roles[FilePathRole] = "file";
    roles[TrackTypeRole] = "trackType"; // Добавляем новую роль

    return roles;
}

ClipModel* TrackModel::getClipModel(int trackIndex) const {
    return m_clipModels.value(trackIndex, nullptr); // Возвращаем существующий или nullptr
}

void TrackModel::ensureClipModel(int trackIndex) {
    if (!m_clipModels.contains(trackIndex)) {
        m_clipModels[trackIndex] = new ClipModel(m_engine, trackIndex, this);
    }
}

void TrackModel::update(int newTrackIndex) {
    int currentCount = m_engine.GetdataBase().size();
    qDebug() << "TrackModel update: m_rowCount =" << m_rowCount << ", currentCount =" << currentCount;
    if (currentCount > m_rowCount) {
        qDebug() << "Inserting rows from" << m_rowCount << "to" << (currentCount - 1);
        beginInsertRows(QModelIndex(), m_rowCount, currentCount - 1);
        m_rowCount = currentCount;
        endInsertRows();
        qDebug() << "Rows inserted, new count:" << m_rowCount;
    }
    else if (currentCount < m_rowCount) {
        qDebug() << "Removing rows from" << currentCount << "to" << (m_rowCount - 1);
        beginRemoveRows(QModelIndex(), currentCount, m_rowCount - 1);
        m_rowCount = currentCount; // Обновляем m_rowCount
        endRemoveRows();
        qDebug() << "Rows removed, new count:" << m_rowCount;

        // Удаляем ClipModel для удалённых дорожек
        for (int i = m_rowCount; i <= newTrackIndex; ++i) {
            if (m_clipModels.contains(i)) {
                delete m_clipModels.take(i);
                qDebug() << "Deleted ClipModel for track" << i;
            }
        }
    }
    emit countChanged();
    qDebug() << "TrackModel updated, final count:" << m_rowCount;
}

void TrackModel::addTrack(QString type, int trackIndex)
{
    ensureClipModel(trackIndex);
    beginInsertRows(QModelIndex(), trackIndex, trackIndex);
    m_rowCount++;
    endInsertRows();
    emit countChanged();
    qDebug() << "Track added at index:" << trackIndex;
}

void TrackModel::deleteTrack(int trackIndex)
{
    beginRemoveRows(QModelIndex(), trackIndex, trackIndex);

    if (m_clipModels.contains(trackIndex)) {
        delete m_clipModels.take(trackIndex);
        qDebug() << "Deleted ClipModel for track" << trackIndex;
    }

    QMap<int, ClipModel*> updatedClipModels;
    for (auto it = m_clipModels.constBegin(); it != m_clipModels.constEnd(); ++it) {
        int oldIndex = it.key();
        ClipModel* clipModel = it.value();
        if (oldIndex > trackIndex) {
            updatedClipModels[oldIndex - 1] = clipModel;
            clipModel->setTrackIndex(oldIndex - 1); // Обновляем индекс в ClipModel
            qDebug() << "Updated ClipModel index from" << oldIndex << "to" << (oldIndex - 1);
        }
        else if (oldIndex < trackIndex) {
            updatedClipModels[oldIndex] = clipModel;
        }
    }
    m_clipModels = updatedClipModels;

    m_rowCount--;
    
    endRemoveRows();
    emit countChanged();
}
