#include "TrackModel.h"
#include "ViewModel.h"
#include <QDebug>

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
    case ClipsRole: {
        QVariantMap trackData;
        QVariantList clipsList;

        for (const auto& clip : track.clips) {
            QVariantMap clipData;
            clipData["startBeats"] = clip->startBeats; // Проверьте точное написание!
            clipData["durationBeats"] = clip->durationBeats;

            if (auto audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
                clipData["type"] = "audio";
                clipData["file"] = QString::fromUtf8(
                    audioClip->file.getFullPathName().toRawUTF8(),
                    audioClip->file.getFullPathName().getNumBytesAsUTF8()
                );
            }
            else {
                clipData["type"] = "midi";
            }
            clipsList.append(clipData);
        }

        trackData["clips"] = clipsList;
        //qDebug() << "Prepared track data:" << trackData;
        return trackData;
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TrackModel::roleNames() const {
    return {
        {TrackIndexRole, "trackIndex"},
        {ClipsRole, "data"} // Именно "data" ожидается в QML
    };
}

void TrackModel::update() {
    beginResetModel();
    endResetModel();
    emit countChanged();
}