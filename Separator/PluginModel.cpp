#include "PluginModel.h"
#include <QDebug>
#include <QRect>

void PluginModel::addPlugin(int trackIndex, const QString& pluginPath) {
    qDebug() << "ViewModel: Adding plugin to track" << trackIndex << "path:" << pluginPath;
    engine.AddPluginToTrack(trackIndex, pluginPath.toStdString());
    qDebug() << "ViewModel: Plugin added to Engine for track" << trackIndex;
    emit pluginAdded(trackIndex);
    qDebug() << "ViewModel: Emitted pluginAdded for track" << trackIndex;
    refresh();
}

void PluginModel::togglePluginBypass(int trackIndex, int pluginIndex) {
    engine.TogglePluginBypass(trackIndex, pluginIndex);
    emit pluginBypassed(trackIndex, pluginIndex);
}

void PluginModel::deletePlugin(int trackIndex, int pluginIndex) {
    if (trackIndex >= 0 && trackIndex < engine.GetdataBase().size()) {
        // Проверяем, открыт ли редактор плагина
        QPair<int, int> key = { trackIndex, pluginIndex };
        if (m_openPluginEditors.contains(key)) {
            juce::Component* component = m_openPluginEditors[key];
            if (component) {
                // Удаляем компонент с рабочего стола
                if (component->isOnDesktop()) {
                    component->removeFromDesktop();
                    qDebug() << "Plugin editor removed from desktop: track=" << trackIndex << ", plugin=" << pluginIndex;
                }
                // Удаляем из отслеживания
                m_openPluginEditors.remove(key);
                qDebug() << "Plugin editor removed from tracking: track=" << trackIndex << ", plugin=" << pluginIndex;
            }
        }

        // Удаляем плагин из движка
        engine.RemovePluginFromTrack(trackIndex, pluginIndex);
        // Удаляем плагин из trackPluginData
        if (trackPluginData.contains(trackIndex)) {
            auto& trackPlugins = trackPluginData[trackIndex];
            trackPlugins.erase(std::remove_if(trackPlugins.begin(), trackPlugins.end(),
                [pluginIndex](const PluginData& p) { return p.index == pluginIndex; }), trackPlugins.end());
        }
        refresh();
        emit pluginRemoved(trackIndex, pluginIndex);
        qDebug() << "Plugin deleted: trackIndex=" << trackIndex << ", pluginIndex=" << pluginIndex;
    }
    else {
        qWarning() << "Invalid track index for plugin deletion:" << trackIndex;
    }
    emit pluginAdded(trackIndex);  // Это может быть ошибкой, возможно, стоит убрать или заменить на pluginRemoved
}

void PluginModel::HidePlugin(int trackIndex, int pluginIndex)
{
    if (trackIndex >= 0 && trackIndex < engine.GetdataBase().size()) {
        // Проверяем, открыт ли редактор плагина
        QPair<int, int> key = { trackIndex, pluginIndex };
        if (m_openPluginEditors.contains(key)) {
            juce::Component* component = m_openPluginEditors[key];
            if (component) {
                // Удаляем компонент с рабочего стола
                component->setVisible(false);
                qDebug() << "Visible off";
            }
        }

    }
}

void PluginModel::openPluginEditor(int trackIndex, int pluginIndex) {
    qDebug() << "Opening plugin editor: track=" << trackIndex << ", plugin=" << pluginIndex;

    QPair<int, int> key = { trackIndex, pluginIndex };

    // Проверяем, существует ли уже редактор
    if (m_openPluginEditors.contains(key)) {
        auto* component = m_openPluginEditors[key];
        if (component->isVisible()) {
            qDebug() << "Plugin editor already visible, bringing to front: track=" << trackIndex << ", plugin=" << pluginIndex;
            component->toFront(true);
            
        }
        else {
            qDebug() << "Plugin editor exists but is hidden, showing: track=" << trackIndex << ", plugin=" << pluginIndex;
            component->setVisible(true);
            component->toFront(true);
        }
        //   emit pluginEditorOpened(trackIndex, pluginIndex, nullptr);
        return;
    }

    // Получаем редактор плагина из движка
    if (auto* editor = engine.GetPluginEditor(trackIndex, pluginIndex)) {
        auto* component = dynamic_cast<juce::Component*>(editor);
        if (component) {
            // Устанавливаем размеры по умолчанию, если они не заданы
            int width = component->getWidth() > 0 ? component->getWidth() : 400;
            int height = component->getHeight() > 0 ? component->getHeight() : 300;

            // Добавляем компонент на рабочий стол
            component->addToDesktop(
                juce::ComponentPeer::windowHasTitleBar |
                juce::ComponentPeer::windowHasDropShadow );
            component->setBounds(100, 100, width, height);
            component->setVisible(true);
            component->setAlwaysOnTop(true); 
            component->toFront(true);

            // Сохраняем компонент в список открытых редакторов
            m_openPluginEditors[key] = component;

            qDebug() << "Plugin editor opened: track=" << trackIndex << ", plugin=" << pluginIndex;
            //  emit pluginEditorOpened(trackIndex, pluginIndex, nullptr);
        }
        else {
            qWarning() << "Failed to cast editor to JUCE Component: track=" << trackIndex << ", plugin=" << pluginIndex;
        }
    }
    else {
        qWarning() << "Failed to get plugin editor: track=" << trackIndex << ", plugin=" << pluginIndex;
    }
}

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
        return QString::fromStdString(plugin.name); // Имя плагина
    case IndexRole:
        return plugin.index; // Индекс плагина
    case TrackIndexRole:
        return currentTrackIndex;
    case IsPinnedRole:  // Добавляем поддержку новой роли
        return plugin.isPinned;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PluginModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IndexRole] = "pluginIndex";
    roles[TrackIndexRole] = "trackIndex";
    roles[IsPinnedRole] = "isPinned";  // Добавляем роль для QML
    return roles;
}

void PluginModel::setTrackIndex(int trackIndex) {
    if (currentTrackIndex != trackIndex) {
        if (currentTrackIndex >= 0) {
            trackPluginData[currentTrackIndex] = plugins;
        }
        // Скрываем все открытые редакторы плагинов для текущей дорожки
        for (auto it = m_openPluginEditors.begin(); it != m_openPluginEditors.end(); ++it) {
            const QPair<int, int>& key = it.key();
            if (key.first == currentTrackIndex) { // Проверяем, что редактор принадлежит текущей дорожке
                juce::Component* component = it.value();
                if (component) {
                    component->setVisible(false);
                    qDebug() << "Hiding plugin editor for track=" << key.first << ", plugin=" << key.second;
                }
            }
        }
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

    // Загружаем сохранённые данные
    std::vector<PluginData> savedPlugins;
    if (trackPluginData.contains(currentTrackIndex)) {
        savedPlugins = trackPluginData[currentTrackIndex];
    }

    plugins.clear();

    if (currentTrackIndex >= 0 && currentTrackIndex < engine.GetdataBase().size()) {
        const auto& track = engine.GetdataBase()[currentTrackIndex];
        std::vector<PluginData> newPlugins;

        // Получаем плагины из движка
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

                // Проверяем, есть ли плагин в сохранённых данных
                bool isPinned = false;
                for (const auto& savedPlugin : savedPlugins) {
                    if (savedPlugin.index == static_cast<int>(i)) {
                        isPinned = savedPlugin.isPinned;
                        break;
                    }
                }
                newPlugins.emplace_back(static_cast<int>(i), pluginName, isPinned);
                qDebug() << "Plugin found:" << pluginName.c_str() << "at index" << i << "isPinned:" << isPinned;
            }
        }

        // Восстанавливаем порядок из savedPlugins
        if (!savedPlugins.empty()) {
            plugins.reserve(newPlugins.size());
            for (const auto& savedPlugin : savedPlugins) {
                for (auto it = newPlugins.begin(); it != newPlugins.end(); ++it) {
                    if (it->index == savedPlugin.index) {
                        plugins.push_back(*it);
                        newPlugins.erase(it);
                        break;
                    }
                }
            }
            // Добавляем новые плагины в конец
            plugins.insert(plugins.end(), newPlugins.begin(), newPlugins.end());
        }
        else {
            // Если нет сохранённого порядка, сортируем
            plugins = newPlugins;
            std::stable_sort(plugins.begin(), plugins.end(),
                [](const PluginData& a, const PluginData& b) {
                    return a.isPinned && !b.isPinned;
                });
        }
    }
    else {
        qDebug() << "PluginModel: Invalid trackIndex" << currentTrackIndex << "database size:" << engine.GetdataBase().size();
    }

    // Сохраняем данные в trackPluginData
    trackPluginData[currentTrackIndex] = plugins;

    endResetModel();
    qDebug() << "PluginModel refreshed for trackIndex:" << currentTrackIndex << "plugin count:" << plugins.size();
    emit rowCountChanged();
}
void PluginModel::togglePin(int trackIndex, int pluginIndex) {
    if (trackIndex != currentTrackIndex) {
        qWarning() << "togglePin: Track index mismatch, expected" << currentTrackIndex << "but got" << trackIndex;
        return;
    }

    for (auto& plugin : plugins) {
        if (plugin.index == pluginIndex) {
            plugin.isPinned = !plugin.isPinned;
            qDebug() << "Plugin pinned state changed: trackIndex=" << trackIndex
                << ", pluginIndex=" << pluginIndex << ", isPinned=" << plugin.isPinned;

            // Сортировка
            std::stable_sort(plugins.begin(), plugins.end(),
                [](const PluginData& a, const PluginData& b) {
                    return a.isPinned && !b.isPinned;
                });

            // Обновляем trackPluginData
            trackPluginData[currentTrackIndex] = plugins;

            // Уведомляем QML
            emit dataChanged(index(0), index(plugins.size() - 1));
            emit pluginPinned(trackIndex, pluginIndex, plugin.isPinned);
            break;
        }
    }
}
