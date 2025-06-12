#include "TrackModel.h"
#include "ViewModel.h"
#include <QDebug>

TrackModel::TrackModel(Engine& engine, QObject* parent)
    : QAbstractListModel(parent), m_engine(engine) {

    const auto& tracks = m_engine.GetdataBase();
	m_rowCount = static_cast<int>(tracks.size());

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
    case GainRole:
        return track.gain; 
    case Mute:
        return track.muted;
    default:
        return QVariant();
    }
}

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
    roles[GainRole] = "gain";
    roles[Mute] = "muted";

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

void TrackModel::update() {

    const auto& tracks = m_engine.GetdataBase();
    beginResetModel();

    int newRowCount = static_cast<int>(tracks.size());
    if (m_rowCount != newRowCount) {
        m_rowCount = newRowCount;
        emit countChanged();
    }

    for (auto* clipModel : m_clipModels) delete clipModel;
    m_clipModels.clear();

    for (int i = 0; i < m_rowCount; ++i) {
        ensureClipModel(i);
        LOG("Created ClipModel for track " << i);
    }

    endResetModel();

    LOG("TrackModel updated with " << m_rowCount << " tracks");
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

    // Удаляем ClipModel для удаляемой дорожки
    if (m_clipModels.contains(trackIndex)) {
        delete m_clipModels.take(trackIndex);
        qDebug() << "Deleted ClipModel for track" << trackIndex;
    }

    // Обновляем индексы для всех ClipModel с индексами > trackIndex
    QMap<int, ClipModel*> updatedClipModels;
    for (auto it = m_clipModels.constBegin(); it != m_clipModels.constEnd(); ++it) {
        int oldIndex = it.key();
        ClipModel* clipModel = it.value();
        if (oldIndex > trackIndex) {
            int newIndex = oldIndex - 1;
            updatedClipModels[newIndex] = clipModel;
            clipModel->setTrackIndex(newIndex);
            qDebug() << "Updated ClipModel index from" << oldIndex << "to" << newIndex;
        }
        else {
            updatedClipModels[oldIndex] = clipModel;
        }
    }
    m_clipModels = updatedClipModels;

    m_rowCount--;
    endRemoveRows();
    emit countChanged();

    // Уведомляем QML об изменении всех дорожек
    if (m_rowCount > 0) {
        QModelIndex topLeft = createIndex(0, 0);
        QModelIndex bottomRight = createIndex(m_rowCount - 1, 0);
        emit dataChanged(topLeft, bottomRight, { TrackIndexRole, ClipsModelRole, TrackTypeRole });
    }
 
}
