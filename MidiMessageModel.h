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
        Q_PROPERTY(double clipDuration READ clipDuration NOTIFY clipDurationChanged)

        int trackIndex() const { return m_trackIndex; }
    Q_INVOKABLE void setTrackIndex(int index);

    int clipIndex() const { return m_clipIndex; }
    Q_INVOKABLE void setClipIndex(int index);

    double clipDuration() const { return m_clipDuration; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void addNote(int noteNumber, double startBeats, double durationBeats, float velocity, int channel);
    Q_INVOKABLE void deleteNote(int index);
    Q_INVOKABLE void updateNote(int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel);

signals:
    void trackIndexChanged();
    void clipIndexChanged();
    void clipDurationChanged();

private:
    struct Note {
        int noteNumber;
        double startBeats;
        double durationBeats;
        float velocity;
        int channel;
    };

    Engine& m_engine;
    int m_trackIndex;
    int m_clipIndex;
    double m_clipDuration;
    std::vector<Note> m_notes;

    void rebuildNoteList();
};