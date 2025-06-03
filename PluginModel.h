#pragma once
#include <QAbstractListModel>
#include <QString>
#include "Engine.h" // Предполагается, что ваш движок доступен

class PluginModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit PluginModel(Engine& engine, QObject* parent = nullptr);

    enum PluginRoles {
        NameRole = Qt::UserRole + 1,
        IndexRole,
        TrackIndexRole
    };

    // Реализация QAbstractListModel
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Обновление модели для конкретной дорожки
    Q_INVOKABLE void setTrackIndex(int trackIndex);

    // Обновление данных модели
    void refresh();

private:
    Engine& engine;
    int currentTrackIndex;
    std::vector<std::pair<int, std::string>> plugins; // Пара: индекс плагина и имя
};

