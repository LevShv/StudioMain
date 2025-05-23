#pragma once
#include <QAbstractListModel>
#include "engine.h"

class ClipModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit ClipModel(Engine& engine, int trackIndex, QObject* parent = nullptr);

    enum Roles {
        StartBeatsRole = Qt::UserRole + 1,
        DurationBeatsRole,
        ClipTypeRole,
        FilePathRole
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addClip(const Engine::ClipPtr& clip); // Метод для добавления клипа
    void updateClip(int clipIndex); // Метод для обновления клипа
    void setTrackIndex(int trackIndex);
	void deleteClip(int clipIndex); 

private:
    Engine& m_engine;
    int m_trackIndex;
};