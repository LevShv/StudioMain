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
        FilePathRole
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant clipData(int trackIndex, int clipIndex, int role) const;

    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void update();

    // Новый метод для создания ClipModel
    void ensureClipModel(int trackIndex);
    // Const метод для получения ClipModel
    ClipModel* getClipModel(int trackIndex) const;

signals:
    void countChanged();

private:
    Engine& m_engine;
    QMap<int, ClipModel*> m_clipModels; // Храним ClipModel для каждой дорожки
};