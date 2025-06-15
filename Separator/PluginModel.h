#pragma once
#include <QAbstractListModel>
#include <QString>
#include "Engine.h" 
#include "JuceHeader.h"
struct PluginData {
    int index;           
    std::string name;    
    bool isPinned;       
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
        IsPinnedRole, 
        Bypass 
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void setTrackIndex(int trackIndex);
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

private:
    Engine& engine;
    double redlineStartTime = 0;
    int currentTrackIndex;
    std::vector<PluginData> plugins; 
    QMap<QPair<int, int>, juce::Component*> m_openPluginEditors;
    QMap<int, std::vector<PluginData>> trackPluginData;
};

