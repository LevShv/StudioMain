#include "MidiMessageModel.h"
#include <QDebug>

MidiMessageModel::MidiMessageModel(Engine& engine, QObject* parent)
    : QAbstractListModel(parent), m_engine(engine) {
    refresh();
}

void MidiMessageModel::setClipDuration(double duration) {
    if (m_clipDuration != duration) {
        m_clipDuration = duration;
        qDebug() << "MidiMessageModel: Set clipDuration to" << m_clipDuration;
        // Синхронизируем с Engine
        if (m_trackIndex >= 0 && m_clipIndex >= 0) {
            m_engine.ChangeDuration(m_trackIndex, m_clipIndex, duration);
            qDebug() << "MidiMessageModel: Updated Engine clip duration for trackIndex=" << m_trackIndex << " clipIndex=" << m_clipIndex;
        }
        emit clipDurationChanged();
        refresh();
    }
}

void MidiMessageModel::setTrackIndex(int index) {
    if (m_trackIndex != index) {
        m_trackIndex = index;
        qDebug() << "MidiMessageModel: Set trackIndex to" << m_trackIndex;
        emit trackIndexChanged();
        refresh();
    }
}

void MidiMessageModel::setClipIndex(int index) {
    if (m_clipIndex != index) {
        m_clipIndex = index;
        qDebug() << "MidiMessageModel: Set clipIndex to" << m_clipIndex;
        emit clipIndexChanged();
        refresh();
    }
}

void MidiMessageModel::refresh() {
    beginResetModel();
    rebuildNoteList();
    endResetModel();
    qDebug() << "MidiMessageModel: Refreshed with clipDuration=" << m_clipDuration;
}

int MidiMessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_notes.size();
}

QVariant MidiMessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_notes.size()) return QVariant();

    const auto& note = m_notes[index.row()];
    switch (role) {
    case NoteNumberRole: return note.noteNumber;
    case StartBeatsRole: return note.startBeats;
    case DurationBeatsRole: return note.durationBeats;
    case VelocityRole: return note.velocity;
    case ChannelRole: return note.channel;
    default: return QVariant();
    }
}

QHash<int, QByteArray> MidiMessageModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NoteNumberRole] = "noteNumber";
    roles[StartBeatsRole] = "startBeats";
    roles[DurationBeatsRole] = "durationBeats";
    roles[VelocityRole] = "velocity";
    roles[ChannelRole] = "channel";
    return roles;
}

void MidiMessageModel::addNote(int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (startBeats + durationBeats <= m_clipDuration) {
        m_engine.AddMidiNote(m_trackIndex, m_clipIndex, noteNumber, startBeats, durationBeats, velocity, channel);
        refresh();
    }
    else {
        qWarning() << "MidiMessageModel: Cannot add note, exceeds clipDuration=" << m_clipDuration;
    }
}

void MidiMessageModel::deleteNote(int index) {
    if (index >= 0 && index < m_notes.size()) {
        m_engine.DeleteMidiNote(m_trackIndex, m_clipIndex, index);
        refresh();
    }
}

void MidiMessageModel::updateNote(int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (index >= 0 && index < m_notes.size() && startBeats + durationBeats <= m_clipDuration) {
        m_engine.UpdateMidiNote(m_trackIndex, m_clipIndex, index, noteNumber, startBeats, durationBeats, velocity, channel);
        refresh();
    }
    else {
        qWarning() << "MidiMessageModel: Cannot update note, invalid index or exceeds clipDuration=" << m_clipDuration;
    }
}

void MidiMessageModel::rebuildNoteList() {
    m_notes.clear();
    qDebug() << "MidiMessageModel: Rebuilding note list for trackIndex=" << m_trackIndex << "clipIndex=" << m_clipIndex;

    const auto& database = m_engine.GetdataBase();
    if (m_trackIndex >= 0 && m_trackIndex < database.size()) {
        const auto& clips = database[m_trackIndex].clips;
        if (m_clipIndex >= 0 && m_clipIndex < clips.size()) {
            if (auto* midiClip = dynamic_cast<Engine::MidiClip*>(clips[m_clipIndex].get())) {
                m_clipDuration = midiClip->durationBeats;
                std::map<std::pair<int, int>, double> noteOnTimes;
                qDebug() << "Total events in midiSequence: " << midiClip->midiSequence.getNumEvents();
                for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
                    auto* event = midiClip->midiSequence.getEventPointer(i);
                    qDebug() << "Event " << i << ": type=" << (event->message.isNoteOn() ? "noteOn" : event->message.isNoteOff() ? "noteOff" : "other")
                        << ", noteNumber=" << event->message.getNoteNumber()
                        << ", channel=" << event->message.getChannel()
                        << ", time=" << event->message.getTimeStamp();
                    if (event->message.isNoteOn()) {
                        noteOnTimes[{event->message.getChannel(), event->message.getNoteNumber()}] = event->message.getTimeStamp();
                    }
                    else if (event->message.isNoteOff()) {
                        int noteNumber = event->message.getNoteNumber();
                        int channel = event->message.getChannel();
                        auto key = std::pair<int, int>{ channel, noteNumber };
                        if (noteOnTimes.count(key)) {
                            double startBeats = m_engine.SecondsToBeats(noteOnTimes[key]);
                            double endBeats = m_engine.SecondsToBeats(event->message.getTimeStamp());
                            double durationBeats = endBeats - startBeats;
                            if (durationBeats <= 0) {
                                qWarning() << "Skipping invalid note: noteNumber=" << noteNumber
                                    << ", startBeats=" << startBeats
                                    << ", endBeats=" << endBeats;
                                continue;
                            }
                            float velocity = event->message.getVelocity() / 127.0f;
                            m_notes.push_back({ noteNumber, startBeats, durationBeats, velocity, channel });
                            qDebug() << "Added note: noteNumber=" << noteNumber
                                << ", startBeats=" << startBeats
                                << ", durationBeats=" << durationBeats
                                << ", velocity=" << velocity
                                << ", channel=" << channel;
                        }
                        else {
                            qWarning() << "No matching noteOn for noteOff: noteNumber=" << noteNumber
                                << ", channel=" << channel
                                << ", time=" << event->message.getTimeStamp();
                        }
                    }
                }
                qDebug() << "MidiMessageModel: Loaded" << m_notes.size() << "notes, clipDuration=" << m_clipDuration;
                emit clipDurationChanged();
            }
            else {
                qWarning() << "MidiMessageModel: Clip is not a MidiClip at trackIndex=" << m_trackIndex << "clipIndex=" << m_clipIndex;
                m_clipDuration = 4.0;
                emit clipDurationChanged();
            }
        }
        else {
            qWarning() << "MidiMessageModel: Invalid clipIndex=" << m_clipIndex << "for trackIndex=" << m_trackIndex;
            m_clipDuration = 4.0;
            emit clipDurationChanged();
        }
    }
    else {
        qWarning() << "MidiMessageModel: Invalid trackIndex=" << m_trackIndex;
        m_clipDuration = 4.0;
        emit clipDurationChanged();
    }
}