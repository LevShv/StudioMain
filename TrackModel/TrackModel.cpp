#include "TrackModel.h"
#include "ViewModel.h"

TrackModel::TrackModel(Engine& engine, QObject* parent)
    : QAbstractListModel(parent), m_engine(engine) {

    ViewModel* viewModel = qobject_cast<ViewModel*>(parent);
    if (viewModel) {
        connect(viewModel, &ViewModel::clipAdded, this, &TrackModel::update);
        connect(viewModel, &ViewModel::clipMoved, this, &TrackModel::update);
    }

    qDebug() << "TrackModel initialized with" << m_engine.GetdataBase().size() << "tracks";
}

int TrackModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_engine.GetdataBase().size();
}

QVariant TrackModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        qDebug() << "Invalid index in TrackModel::data";
        return QVariant();
    }

    const auto& tracks = m_engine.GetdataBase();
    int trackIndex = index.row();

    if (trackIndex < 0 || trackIndex >= tracks.size()) {
        qDebug() << "Track index out of range:" << trackIndex;
        return QVariant();
    }

    qDebug() << "Processing track" << trackIndex << "for role" << role;

    switch (role) {
    case CountOfTracks:
        return tracks.size();

    case TrackIndexRole:
        return trackIndex;

    case ClipsRole: {
        QVariantList clips;
        const auto& trackClips = tracks[trackIndex].clips;
        qDebug() << "Track" << trackIndex << "has" << trackClips.size() << "clips";
        for (const auto& clip : trackClips) {
            QVariantMap clipData;
            clipData["startTime"] = clip->startTime;
            clipData["duration"] = clip->duration ? clip->duration : 1.0; // ”бедимс€, что duration не null
            clipData["gain"] = clip->gain;
            clipData["muted"] = clip->muted;
            if (auto* audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
                clipData["type"] = "audio";
                clipData["file"] = QString::fromStdString(audioClip->file.getFullPathName().toStdString());
            }
            else if (auto* midiClip = dynamic_cast<Engine::MidiClip*>(clip.get())) {
                clipData["type"] = "midi";
            }
            clips.append(clipData);
        }
        QVariantMap trackData;
        trackData["clips"] = clips.isEmpty() ? QVariant(QVariantList()) : clips; // явно задаем пустой список, если клипов нет
        return trackData;
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TrackModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[TrackIndexRole] = "trackIndex";
    roles[ClipsRole] = "data"; // ”бедимс€, что роль называетс€ "data"
    return roles;
}

void TrackModel::update() {
    int oldCount = rowCount();

    beginResetModel();
    endResetModel();

    if (rowCount() != oldCount) {
        emit countChanged();
    }
}