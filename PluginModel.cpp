#include "PluginModel.h"
#include <QDebug>

PluginModel::PluginModel(Engine& engine, QObject* parent)
    : QAbstractListModel(parent), engine(engine), currentTrackIndex(-1) {
}

int PluginModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return plugins.size();
}

QVariant PluginModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= plugins.size()) {
        return QVariant();
    }

    const auto& plugin = plugins[index.row()];
    switch (role) {
    case NameRole:
        return QString::fromStdString(plugin.second); // Имя плагина
    case IndexRole:
        return plugin.first; // Индекс плагина
    case TrackIndexRole:
        return currentTrackIndex;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PluginModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IndexRole] = "pluginIndex";
    roles[TrackIndexRole] = "trackIndex";
    return roles;
}

void PluginModel::setTrackIndex(int trackIndex) {
    if (currentTrackIndex != trackIndex) {
        currentTrackIndex = trackIndex;
        qDebug() << "PluginModel: setTrackIndex to" << trackIndex;
        refresh();
    }
    else {
        qDebug() << "PluginModel: setTrackIndex called but no change, trackIndex:" << trackIndex;
    }
}

void PluginModel::refresh() {
    qDebug() << "PluginModel: Refreshing for trackIndex" << currentTrackIndex;
    beginResetModel();
    plugins.clear();

    if (currentTrackIndex >= 0 && currentTrackIndex < engine.GetdataBase().size()) {
        const auto& track = engine.GetdataBase()[currentTrackIndex];
        for (size_t i = 0; i < track.plugins.size(); ++i) {
            if (track.plugins[i]) {
                std::string pluginName = track.plugins[i]->Path;
                size_t lastSlash = pluginName.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    pluginName = pluginName.substr(lastSlash + 1);
                }
                size_t dotPos = pluginName.find_last_of(".");
                if (dotPos != std::string::npos) {
                    pluginName = pluginName.substr(0, dotPos);
                }
                plugins.emplace_back(static_cast<int>(i), pluginName);
                qDebug() << "Plugin found:" << pluginName.c_str() << "at index" << i;
            }
        }
    }
    else {
        qDebug() << "PluginModel: Invalid trackIndex" << currentTrackIndex << "database size:" << engine.GetdataBase().size();
    }

    endResetModel();
    qDebug() << "PluginModel refreshed for trackIndex:" << currentTrackIndex << "plugin count:" << plugins.size();
}