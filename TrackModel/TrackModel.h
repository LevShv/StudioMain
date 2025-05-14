#pragma once
#include <QAbstractListModel>
#include "engine.h"

class TrackModel : public QAbstractListModel {
    Q_OBJECT
        Q_PROPERTY(int countOfTracks READ rowCount NOTIFY countChanged)

public:
    explicit TrackModel(Engine& engine, QObject* parent = nullptr);

    enum Roles {
        TrackIndexRole = Qt::UserRole + 1,
        ClipsRole,
        CountOfTracks,
        StartBeatsRole,
        DurationBeatsRole,
        ClipTypeRole,
        FilePathRole
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void update();

signals:
    void countChanged();

private:
    Engine& m_engine;
};