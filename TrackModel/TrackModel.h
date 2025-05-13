// TrackModel.h
#pragma once
#include <QAbstractListModel>
#include "engine.h"

class TrackModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit TrackModel(Engine& engine, QObject* parent = nullptr);

    enum Roles {
        TrackIndexRole = Qt::UserRole + 1,
        ClipsRole,
        CountOfTracks
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void update(); // Для принудительного обновления модели

private:
    Engine& m_engine;
};

