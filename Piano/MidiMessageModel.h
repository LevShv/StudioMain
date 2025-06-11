#pragma once
#include <QAbstractListModel>
#include "engine.h"

class MidiMessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit MidiMessageModel(Engine& engine, QObject* parent = nullptr);

    enum Roles {
        NoteNumberRole = Qt::UserRole + 1,
        StartBeatsRole,
        DurationBeatsRole,
        VelocityRole,
        ChannelRole
    };

    Q_PROPERTY(int trackIndex READ trackIndex WRITE setTrackIndex NOTIFY trackIndexChanged)
    Q_PROPERTY(int clipIndex READ clipIndex WRITE setClipIndex NOTIFY clipIndexChanged)
    Q_PROPERTY(double clipDuration READ clipDuration WRITE setClipDuration NOTIFY clipDurationChanged)
    Q_PROPERTY(double clipStartTime READ getClipStartTime NOTIFY clipStartTimeChanged)
    
    Q_INVOKABLE void addMidiNote(int trackIndex, int clipIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);
    Q_INVOKABLE void deleteMidiNote(int trackIndex, int clipIndex, int index);
    Q_INVOKABLE void updateMidiNote(int trackIndex, int clipIndex, int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);
    
  
    int trackIndex() const { return m_trackIndex; }
    Q_INVOKABLE void setTrackIndex(int index);

    int clipIndex() const { return m_clipIndex; }
    Q_INVOKABLE void setClipIndex(int index);

    double clipDuration() const { return m_clipDuration; }
    Q_INVOKABLE void setClipDuration(double duration);

    double getClipStartTime() const { return m_clipStartTime; }
    Q_INVOKABLE void setRedlineStartime();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void addNote(int noteNumber, double startBeats, double durationBeats, float velocity, int channel);
    Q_INVOKABLE void deleteNote(int index);
    Q_INVOKABLE void updateNote(int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);

public slots:
    void onClipMoved(int trackIndex, int clipIndex, double newStartBeats);

signals:
    void trackIndexChanged();
    void clipIndexChanged();
    void clipDurationChanged();
    void clipStartTimeChanged();

private:
    struct Note {
        int noteNumber;
        double startBeats;
        double durationBeats;
        float velocity;
        int channel;
    };

    Engine& m_engine;
    int m_trackIndex = -1;
    int m_clipIndex = -1;
    double m_clipDuration = 4.0;
    double m_clipStartTime = 0.0;
    std::vector<Note> m_notes;

    void rebuildNoteList();
};