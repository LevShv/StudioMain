#include "MidiMessageModel.h"
#include "engine.h"
#include <QDebug>

MidiMessageModel::MidiMessageModel(Engine& engine, QObject* parent)
    : QAbstractListModel(parent), m_engine(engine), m_trackIndex(0), m_clipIndex(0), m_clipDuration(4.0) {
    qDebug() << "MidiMessageModel constructor called";
}

void MidiMessageModel::setTrackIndex(int index) {
    if (m_trackIndex != index) {
        m_trackIndex = index;
        refresh();
        emit trackIndexChanged();
        qDebug() << "MidiMessageModel: trackIndex set to" << m_trackIndex;
    }
}

void MidiMessageModel::setClipIndex(int index) {
    if (m_clipIndex != index) {
        m_clipIndex = index;
        refresh();
        emit clipIndexChanged();
        qDebug() << "MidiMessageModel: clipIndex set to" << m_clipIndex;
    }
}

int MidiMessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_notes.size();
}

QVariant MidiMessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_notes.size()) return {};
    const Note& note = m_notes[index.row()];
    switch (role) {
    case NoteNumberRole: return note.noteNumber;
    case StartBeatsRole: return note.startBeats;
    case DurationBeatsRole: return note.durationBeats;
    case VelocityRole: return note.velocity;
    case ChannelRole: return note.channel;
    }
    return {};
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

void MidiMessageModel::refresh() {
    beginResetModel();
    rebuildNoteList();
    endResetModel();
    qDebug() << "MidiMessageModel: refreshed, note count=" << m_notes.size() << "clipDuration=" << m_clipDuration;
}

void MidiMessageModel::addNote(int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (m_trackIndex < 0 || m_clipIndex < 0) {
        qWarning() << "Cannot add note: invalid trackIndex=" << m_trackIndex << "or clipIndex=" << m_clipIndex;
        return;
    }
    beginInsertRows({}, m_notes.size(), m_notes.size());
    m_notes.push_back({ noteNumber, startBeats, durationBeats, velocity, channel });
    endInsertRows();
    if (startBeats + durationBeats > m_clipDuration) {
        m_clipDuration = startBeats + durationBeats;
        emit clipDurationChanged();
    }
    refresh();
    qDebug() << "MidiMessageModel: Added note: noteNumber=" << noteNumber << "startBeats=" << startBeats
        << "trackIndex=" << m_trackIndex << "clipIndex=" << m_clipIndex << "new clipDuration=" << m_clipDuration;
}

void MidiMessageModel::deleteNote(int index) {
    if (index < 0 || index >= m_notes.size()) {
        qWarning() << "Cannot delete note: invalid index=" << index;
        return;
    }
    beginRemoveRows({}, index, index);
    m_notes.erase(m_notes.begin() + index);
    endRemoveRows();
    rebuildNoteList();
    qDebug() << "MidiMessageModel: Deleted note at index=" << index << "new clipDuration=" << m_clipDuration;
}

void MidiMessageModel::updateNote(int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (index < 0 || index >= m_notes.size()) {
        qWarning() << "Cannot update note: invalid index=" << index;
        return;
    }
    m_notes[index] = { noteNumber, startBeats, durationBeats, velocity, channel };
    emit dataChanged(this->index(index), this->index(index));
    if (startBeats + durationBeats > m_clipDuration) {
        m_clipDuration = startBeats + durationBeats;
        emit clipDurationChanged();
    }
    qDebug() << "MidiMessageModel: Updated note at index=" << index << "noteNumber=" << noteNumber << "new clipDuration=" << m_clipDuration;
}

void MidiMessageModel::rebuildNoteList() {
    m_notes.clear();
    m_clipDuration = 0.0;
    qDebug() << "MidiMessageModel: Rebuilding note list for trackIndex=" << m_trackIndex << "clipIndex=" << m_clipIndex;

    const auto& database = m_engine.GetdataBase();
    if (m_trackIndex >= 0 && m_trackIndex < database.size()) {
        const auto& clips = database[m_trackIndex].clips;
        qDebug() << "MidiMessageModel: Found" << clips.size() << "clips in track" << m_trackIndex;

        if (m_clipIndex >= 0 && m_clipIndex < clips.size()) {
            if (auto* midiClip = dynamic_cast<Engine::MidiClip*>(clips[m_clipIndex].get())) {
                qDebug() << "MidiMessageModel: Clip is a MidiClip, midiSequence size=" << midiClip->midiSequence.getNumEvents();

                std::map<std::pair<int, int>, double> noteOnTimes;
                for (int i = 0; i < midiClip->midiSequence.getNumEvents(); ++i) {
                    auto* event = midiClip->midiSequence.getEventPointer(i);
                    qDebug() << "MidiMessageModel: Event" << i << "type=" << (event->message.isNoteOn() ? "NoteOn" : event->message.isNoteOff() ? "NoteOff" : "Other")
                        << "timeStamp=" << event->message.getTimeStamp();
                    if (event->message.isNoteOn()) {
                        noteOnTimes[{event->message.getChannel(), event->message.getNoteNumber()}] = event->message.getTimeStamp();
                    }
                    else if (event->message.isNoteOff()) {
                        int noteNumber = event->message.getNoteNumber();
                        int channel = event->message.getChannel();
                        auto key = std::pair<int, int>{ channel, noteNumber };
                        if (noteOnTimes.find(key) != noteOnTimes.end()) {
                            double startBeats = m_engine.SecondsToBeats(noteOnTimes[key]);
                            double endBeats = m_engine.SecondsToBeats(event->message.getTimeStamp());
                            double durationBeats = endBeats - startBeats;
                            float velocity = event->message.getVelocity();

                            m_notes.push_back({ noteNumber, startBeats, durationBeats, velocity, channel });
                            if (endBeats > m_clipDuration) {
                                m_clipDuration = endBeats;
                            }
                            qDebug() << "MidiMessageModel: Added note: noteNumber=" << noteNumber
                                << "startBeats=" << startBeats << "durationBeats=" << durationBeats
                                << "velocity=" << velocity << "endBeats=" << endBeats;
                            noteOnTimes.erase(key);
                        }
                    }
                }
                if (m_clipDuration == 0.0) {
                    m_clipDuration = 4.0;
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