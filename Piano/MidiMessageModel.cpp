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
        const auto& database = m_engine.GetdataBase();
        if (m_trackIndex >= 0 && m_trackIndex < database.size()) {
            const auto& clips = database[m_trackIndex].clips;
            if (m_clipIndex >= 0 && m_clipIndex < clips.size()) {
                if (auto* midiClip = dynamic_cast<Engine::MidiClip*>(clips[m_clipIndex].get())) {
                    m_engine.cleanMidiSequence(midiClip);
                }
            }
        }
        emit clipIndexChanged();
        refresh();
    }
}

void MidiMessageModel::setRedlineStartime()
{
    qDebug() << "MidiMessageModel: set redline startime to" << m_clipIndex;
    const auto& database = m_engine.GetdataBase();
    if (m_trackIndex >= 0 && m_trackIndex < database.size()) {
        const auto& clips = database[m_trackIndex].clips;
        if (m_clipIndex >= 0 && m_clipIndex < clips.size()) {
            if (auto* ClipBase = dynamic_cast<Engine::ClipBase*>(clips[m_clipIndex].get())) {
                m_clipStartTime = ClipBase->startBeats; 
                qDebug() << "MidiMessageModel: set redline startime: " << m_clipDuration;
                emit clipStartTimeChanged();
            }
        }
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
                m_clipStartTime = midiClip->startBeats; 
                std::map<std::pair<int, int>, std::pair<double, float>> noteOnTimes;
                qDebug() << "Total events in midiSequence: " << midiClip->midiSequence.getNumEvents();
                for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
                    auto* event = midiClip->midiSequence.getEventPointer(i);
                    qDebug() << "Event " << i << ": type=" << (event->message.isNoteOn() ? "noteOn" : event->message.isNoteOff() ? "noteOff" : "other")
                        << ", noteNumber=" << event->message.getNoteNumber()
                        << ", channel=" << event->message.getChannel()
                        << ", time=" << event->message.getTimeStamp();
                    if (event->message.isNoteOn()) {
                        noteOnTimes[{event->message.getChannel(), event->message.getNoteNumber()}] = {
                            event->message.getTimeStamp(),
                            event->message.getVelocity() / 127.0f
                        };
                    }
                    else if (event->message.isNoteOff()) {
                        int noteNumber = event->message.getNoteNumber();
                        int channel = event->message.getChannel();
                        auto key = std::pair<int, int>{ channel, noteNumber };
                        if (noteOnTimes.count(key)) {
                            double startBeats = m_engine.SecondsToBeats(noteOnTimes[key].first);
                            double endBeats = m_engine.SecondsToBeats(event->message.getTimeStamp());
                            double durationBeats = endBeats - startBeats;
                            if (durationBeats <= 0) {
                                qWarning() << "Skipping invalid note: noteNumber=" << noteNumber
                                    << ", startBeats=" << startBeats
                                    << ", endBeats=" << endBeats;
                                continue;
                            }
                            float velocity = noteOnTimes[key].second;
                            m_notes.push_back({ noteNumber, startBeats, durationBeats, velocity, channel });
                            qDebug() << "Added note: noteNumber=" << noteNumber
                                << ", startBeats=" << startBeats
                                << ", durationBeats=" << durationBeats
                                << ", velocity=" << velocity
                                << ", channel=" << channel;
                            noteOnTimes.erase(key);
                        }
                        else {
                            qWarning() << "No matching noteOn for noteOff: noteNumber=" << noteNumber
                                << ", channel=" << channel
                                << ", time=" << event->message.getTimeStamp();
                        }
                    }
                }
                qDebug() << "MidiMessageModel: Loaded" << m_notes.size() << "notes, clipDuration=" << m_clipDuration
                    << ", clipStartTime=" << m_clipStartTime;
                emit clipDurationChanged();
                emit clipStartTimeChanged();
            }
            else {
                qWarning() << "MidiMessageModel: Clip is not a MidiClip at trackIndex=" << m_trackIndex << "clipIndex=" << m_clipIndex;
                m_clipDuration = 4.0;
                m_clipStartTime = 0.0;
                emit clipDurationChanged();
                emit clipStartTimeChanged();
            }
        }
        else {
            qWarning() << "MidiMessageModel: Invalid clipIndex=" << m_clipIndex << "for trackIndex=" << m_trackIndex;
            m_clipDuration = 4.0;
            m_clipStartTime = 0.0;
            emit clipDurationChanged();
            emit clipStartTimeChanged();
        }
    }
    else {
        qWarning() << "MidiMessageModel: Invalid trackIndex=" << m_trackIndex;
        m_clipDuration = 4.0;
        m_clipStartTime = 0.0;
        emit clipDurationChanged();
        emit clipStartTimeChanged();
    }
}

void MidiMessageModel::deleteNote(int index) {
    if (index >= 0 && index < m_notes.size()) {
        qDebug() << "MidiMessageModel: Deleting note at index=" << index
            << ", noteNumber=" << m_notes[index].noteNumber
            << ", startBeats=" << m_notes[index].startBeats;
        m_engine.DeleteMidiNote(m_trackIndex, m_clipIndex, index);
        refresh();
    }
    else {
        qWarning() << "MidiMessageModel: Cannot delete note, invalid index=" << index;
    }
}

void MidiMessageModel::onClipMoved(int trackIndex, int clipIndex, double newStartBeats) {
    qDebug() << "MidiMessageModel: Received clipMoved signal: trackIndex=" << trackIndex
        << ", clipIndex=" << clipIndex << ", newStartBeats=" << newStartBeats;
    if (trackIndex == m_trackIndex && clipIndex == m_clipIndex) {
        setRedlineStartime();
    }
}

void MidiMessageModel::addMidiNote(int trackIndex, int clipIndex, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    
    if (trackIndex < 0 || clipIndex < 0 || noteNumber < 0 || noteNumber > 127 ||
        startBeats < 0 || durationBeats <= 0 || velocity < 0 || velocity > 1.0 || channel < 1 || channel > 16) {
        qWarning() << "ViewModel: Invalid note parameters: noteNumber=" << noteNumber
            << "startBeats=" << startBeats << "durationBeats=" << durationBeats
            << "velocity=" << velocity << "channel=" << channel;
        return;
    }

    setTrackIndex(trackIndex);
    setClipIndex(clipIndex);

    const auto& database = m_engine.GetdataBase();
    if (trackIndex >= database.size() || clipIndex >= database[trackIndex].clips.size()) {
        qWarning() << "ViewModel: Invalid trackIndex=" << trackIndex << "or clipIndex=" << clipIndex;
        return;
    }

    if (!dynamic_cast<Engine::MidiClip*>(database[trackIndex].clips[clipIndex].get())) {
        qWarning() << "ViewModel: Clip at trackIndex=" << trackIndex << "clipIndex=" << clipIndex << "is not a MidiClip";
        return;
    }

    m_engine.AddMidiNote(trackIndex, clipIndex, noteNumber, startBeats, durationBeats, velocity, channel);

    refresh();
    qDebug() << "ViewModel: Added MIDI note: trackIndex=" << trackIndex << "clipIndex=" << clipIndex
        << "noteNumber=" << noteNumber << "startBeats=" << startBeats << "velocity=" << velocity;
}

void MidiMessageModel::deleteMidiNote(int trackIndex, int clipIndex, int index) {
    if (trackIndex == this->trackIndex() && clipIndex == this->clipIndex()) {
        m_engine.DeleteMidiNote(trackIndex, clipIndex, index);
        deleteNote(index);
        qDebug() << "ViewModel: Deleted MIDI note at index=" << index << "trackIndex=" << trackIndex << "clipIndex=" << clipIndex;
    }
    else {
        qWarning() << "Cannot delete note: trackIndex=" << trackIndex << "or clipIndex=" << clipIndex << "does not match midiModel";
    }
}

void MidiMessageModel::updateMidiNote(int trackIndex, int clipIndex, int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (trackIndex == this->trackIndex() && clipIndex == this->clipIndex()) {
        m_engine.UpdateMidiNote(trackIndex, clipIndex, index, noteNumber, startBeats, durationBeats, velocity, channel);
        updateNote(index, noteNumber, startBeats, durationBeats, velocity, channel);
        qDebug() << "ViewModel: Updated MIDI note at index=" << index << "trackIndex=" << trackIndex << "clipIndex=" << clipIndex << "noteNumber=" << noteNumber;
    }
    else {
        qWarning() << "Cannot update note: trackIndex=" << trackIndex << "or clipIndex=" << clipIndex << "does not match midiModel";
    }
}


