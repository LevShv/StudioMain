#pragma once
#include <QAbstractListModel>
#include <QString>
#include "Engine.h" // Предполагается, что ваш движок доступен
#include "JuceHeader.h"
class PluginModel : public QAbstractListModel {
    Q_OBJECT
public:

    explicit PluginModel(Engine& engine, QObject* parent = nullptr);

    Q_INVOKABLE void addPlugin(int trackIndex, const QString& pluginPath);
    Q_INVOKABLE void togglePluginBypass(int trackIndex, int pluginIndex);
    Q_INVOKABLE void deletePlugin(int trackIndex, int pluginIndex);
    Q_INVOKABLE void HidePlugin(int trackIndex, int pluginIndex);
    Q_INVOKABLE void openPluginEditor(int trackIndex, int pluginIndex);
   

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

signals:
    void rowCountChanged();
    void pluginAdded(int trackIndex);
    void pluginRemoved(int trackIndex, int pluginIndex);
    void clipAdded(int trackIndex);
    void clipMoved(int trackIndex, int clipIndex, double newStartTime);
    void pluginBypassed(int trackIndex, int pluginIndex);
  //  void pluginEditorOpened(int trackIndex, int pluginIndex, QWindow* window);

private:
    Engine& engine;

    int currentTrackIndex;
    std::vector<std::pair<int, std::string>> plugins; // Пара: индекс плагина и имя
    QMap<QPair<int, int>, juce::Component*> m_openPluginEditors;
};

