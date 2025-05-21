#include "TrackModel.h"
#include "ViewModel.h"
#include <QDebug>

TrackModel::TrackModel(Engine& engine, QObject* parent)
    : QAbstractListModel(parent), m_engine(engine) {

    const auto& tracks = m_engine.GetdataBase();
    for (int i = 0; i < tracks.size(); ++i) {
        ensureClipModel(i);
        qDebug() << "Initialized ClipModel for track" << i;
    }
}

int TrackModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return m_engine.GetdataBase().size();
}

QVariant TrackModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

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
    default:
        return QVariant();
    }
}

QVariant TrackModel::clipData(int trackIndex, int clipIndex, int role) const {
    const auto& tracks = m_engine.GetdataBase();
    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        qWarning() << "Invalid track index in clipData:" << trackIndex;
        return QVariant();
    }

    const auto& track = tracks[trackIndex];
    if (clipIndex < 0 || clipIndex >= track.clips.size()) {
        qWarning() << "Invalid clip index:" << clipIndex << "for track:" << trackIndex;
        return QVariant();
    }

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
        qWarning() << "Unknown role in clipData:" << role;
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
    //beginResetModel();
    //endResetModel();
    emit countChanged();
}