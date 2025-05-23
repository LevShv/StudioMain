#pragma once
#include <QAbstractListModel>
#include "engine.h"
#include <log.h>
#include "ClipModel.h"

class TrackModel : public QAbstractListModel {
    Q_OBJECT
        Q_PROPERTY(int countOfTracks READ rowCount NOTIFY countChanged)

public:
    explicit TrackModel(Engine& engine, QObject* parent = nullptr);

    enum Roles {
        TrackIndexRole = Qt::UserRole + 1,
        ClipsModelRole,
        CountOfTracks,
        StartBeatsRole,
        DurationBeatsRole,
        ClipTypeRole,
        FilePathRole,
        TrackTypeRole = Qt::UserRole + 8
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant clipData(int trackIndex, int clipIndex, int role) const;

    QHash<int, QByteArray> roleNames() const override;

    // Новый метод для создания ClipModel
    void ensureClipModel(int trackIndex);
    Q_INVOKABLE void update(int newTrackIndex);
    void addTrack(QString type, int trackIndex);
	void deleteTrack(int trackIndex);
   
    // Const метод для получения ClipModel
    ClipModel* getClipModel(int trackIndex) const;

signals:
    void countChanged();

private:
    Engine& m_engine;
    int m_rowCount = 0;
    QMap<int, ClipModel*> m_clipModels; // Храним ClipModel для каждой дорожки
};