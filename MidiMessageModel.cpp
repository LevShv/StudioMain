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
    }
}

void MidiMessageModel::setClipIndex(int index) {
    if (m_clipIndex != index) {
        m_clipIndex = index;
        refresh();
        emit clipIndexChanged();
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

void MidiMessageModel::addNote(int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    beginInsertRows({}, m_notes.size(), m_notes.size());
    m_notes.push_back({ noteNumber, startBeats, durationBeats, velocity, channel });
    // TODO: Добавить ноту через Engine
    // m_engine.AddMidiNote(m_trackIndex, m_clipIndex, noteNumber, startBeats, durationBeats, velocity, channel);
    endInsertRows();
}

void MidiMessageModel::deleteNote(int index) {
    if (index < 0 || index >= m_notes.size()) return;
    beginRemoveRows({}, index, index);
    m_notes.erase(m_notes.begin() + index);
    // TODO: Удалить ноту через Engine
    // m_engine.DeleteMidiNote(m_trackIndex, m_clipIndex, index);
    endRemoveRows();
}

void MidiMessageModel::updateNote(int index, int noteNumber, double startBeats, double durationBeats, float velocity, int channel) {
    if (index < 0 || index >= m_notes.size()) return;
    m_notes[index] = { noteNumber, startBeats, durationBeats, velocity, channel };
    // TODO: Обновить ноту через Engine
    // m_engine.UpdateMidiNote(m_trackIndex, m_clipIndex, index, noteNumber, startBeats, durationBeats, velocity, channel);
    emit dataChanged(this->index(index), this->index(index));
}

void MidiMessageModel::refresh() {
    beginResetModel();
    rebuildNoteList();
    endResetModel();
}

void MidiMessageModel::rebuildNoteList() {
    m_notes.clear();
    const auto& database = m_engine.GetdataBase();
    if (m_trackIndex >= 0 && m_trackIndex < database.size()) {
        const auto& clips = database[m_trackIndex].clips;
        if (m_clipIndex >= 0 && m_clipIndex < clips.size()) {
            // TODO: Адаптировать под структуру clips
            // for (const auto& midiNote : clips[m_clipIndex].midiNotes) {
            //     m_notes.push_back({
            //         midiNote.noteNumber,
            //         midiNote.startBeats,
            //         midiNote.durationBeats,
            //         midiNote.velocity,
            //         midiNote.channel
            //     });
            // }
        }
    }
}