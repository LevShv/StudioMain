#include "MidiMessageModel.h"
#include "engine.h"
#include <QDebug>

MidiMessageModel::MidiMessageModel(Engine& engine, QObject* parent)
    : QAbstractListModel(parent), m_engine(engine), m_trackIndex(0), m_clipIndex(0) {
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
    qDebug() << "MidiMessageModel: refreshed, note count=" << m_notes.size();
}

void MidiMessageModel::addNote(int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (m_trackIndex < 0 || m_clipIndex < 0) {
        qWarning() << "Cannot add note: invalid trackIndex=" << m_trackIndex << "or clipIndex=" << m_clipIndex;
        return;
    }
    beginInsertRows({}, m_notes.size(), m_notes.size());
    m_notes.push_back({ noteNumber, startBeats, durationBeats, velocity, channel });
    endInsertRows();
    qDebug() << "MidiMessageModel: Added note: noteNumber=" << noteNumber << "startBeats=" << startBeats;
}

void MidiMessageModel::deleteNote(int index) {
    if (index < 0 || index >= m_notes.size()) {
        qWarning() << "Cannot delete note: invalid index=" << index;
        return;
    }
    beginRemoveRows({}, index, index);
    m_notes.erase(m_notes.begin() + index);
    endRemoveRows();
    qDebug() << "MidiMessageModel: Deleted note at index=" << index;
}

void MidiMessageModel::updateNote(int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (index < 0 || index >= m_notes.size()) {
        qWarning() << "Cannot update note: invalid index=" << index;
        return;
    }
    m_notes[index] = { noteNumber, startBeats, durationBeats, velocity, channel };
    emit dataChanged(this->index(index), this->index(index));
    qDebug() << "MidiMessageModel: Updated note at index=" << index << "noteNumber=" << noteNumber;
}

void MidiMessageModel::rebuildNoteList() {
    m_notes.clear();
    const auto& database = m_engine.GetdataBase();
    if (m_trackIndex >= 0 && m_trackIndex < database.size()) {
        const auto& clips = database[m_trackIndex].clips;
        if (m_clipIndex >= 0 && m_clipIndex < clips.size()) {
            if (auto* midiClip = dynamic_cast<Engine::MidiClip*>(clips[m_clipIndex].get())) {
                for (const auto* event : midiClip->midiSequence) {
                    if (event->message.isNoteOn()) {
                        double startBeats = event->message.getTimeStamp();
                        int noteNumber = event->message.getNoteNumber();
                        int channel = event->message.getChannel();
                        float velocity = event->message.getVelocity() / 127.0f;

                        // Найти соответствующее NoteOff событие
                        double durationBeats = 1.0; // По умолчанию 1 бит
                        for (const auto* offEvent : midiClip->midiSequence) {
                            if (offEvent->message.isNoteOff() &&
                                offEvent->message.getNoteNumber() == noteNumber &&
                                offEvent->message.getChannel() == channel &&
                                offEvent->message.getTimeStamp() > startBeats) {
                                durationBeats = offEvent->message.getTimeStamp() - startBeats;
                                break;
                            }
                        }

                        m_notes.push_back({ noteNumber, startBeats, durationBeats, velocity, channel });
                    }
                }
                qDebug() << "MidiMessageModel: Loaded" << m_notes.size() << "notes for trackIndex=" << m_trackIndex << "clipIndex=" << m_clipIndex;
            }
            else {
                qWarning() << "Clip at trackIndex=" << m_trackIndex << "clipIndex=" << m_clipIndex << "is not a MidiClip";
            }
        }
        else {
            qWarning() << "Invalid clipIndex=" << m_clipIndex << "for trackIndex=" << m_trackIndex;
        }
    }
    else {
        qWarning() << "Invalid trackIndex=" << m_trackIndex;
    }
}