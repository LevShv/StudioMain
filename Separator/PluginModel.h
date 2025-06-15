#pragma once
#include <QAbstractListModel>
#include <QString>
#include "Engine.h" // Предполагается, что ваш движок доступен
#include "JuceHeader.h"
struct PluginData {
    int index;           // Индекс плагина
    std::string name;    // Имя плагина
    bool isPinned;       // Флаг закрепления
    PluginData(int idx, const std::string& n, bool pinned = false)
        : index(idx), name(n), isPinned(pinned) {
    }
};
class PluginModel : public QAbstractListModel {
    Q_OBJECT
public:

    explicit PluginModel(Engine& engine, QObject* parent = nullptr);
    Q_INVOKABLE void addPlugin(int trackIndex, const QString& pluginPath);
    Q_INVOKABLE void togglePluginBypass(int trackIndex, int pluginIndex);
    Q_INVOKABLE void deletePlugin(int trackIndex, int pluginIndex);
    Q_INVOKABLE void hidePlugin(int trackIndex, int pluginIndex);
    Q_INVOKABLE void openPluginEditor(int trackIndex, int pluginIndex);
    Q_INVOKABLE void togglePin(int trackIndex, int pluginIndex);

   

    enum PluginRoles {
        NameRole = Qt::UserRole + 1,
        IndexRole,
        TrackIndexRole,
        IsPinnedRole  // Новая роль для флага закрепления
    };

    // Реализация QAbstractListModel
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Обновление модели для конкретной дорожки
    Q_INVOKABLE void setTrackIndex(int trackIndex);

    // Обновление данных модели

    int getTrackIndex() const { return currentTrackIndex; }
    void refresh();

signals:
    void rowCountChanged();
    void pluginAdded(int trackIndex);
    void pluginRemoved(int trackIndex, int pluginIndex);
    void clipAdded(int trackIndex);
    void clipMoved(int trackIndex, int clipIndex, double newStartTime);
    void pluginBypassed(int trackIndex, int pluginIndex);
    void trackIndexChanged();
    void pluginPinned(int trackIndex, int pluginIndex, bool isPinned);
  //  void pluginEditorOpened(int trackIndex, int pluginIndex, QWindow* window);

private:
    Engine& engine;
    double redlineStartTime = 0;
    int currentTrackIndex;
    std::vector<PluginData> plugins; // Пара: индекс плагина и имя
    QMap<QPair<int, int>, juce::Component*> m_openPluginEditors;
    QMap<int, std::vector<PluginData>> trackPluginData; // Хранит данные плагинов для каждой дорожки
};

