// ViewModel.cpp

#include "ViewModel.h"
#include <QtCore/QDebug>
#include <string>


ViewModel::ViewModel(QObject* parent) : QObject(parent)
{

    //juce::MidiMessageSequence sequence;

    //sequence.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0.0);   // Нота C4
    //sequence.addEvent(juce::MidiMessage::noteOff(1, 60), 0.5);                   // Выключение C4 через 0.5 сек

    //sequence.addEvent(juce::MidiMessage::noteOn(1, 64, (juce::uint8)100), 1.0);   // Нота E4
    //sequence.addEvent(juce::MidiMessage::noteOff(1, 64), 1.5);

    //sequence.addEvent(juce::MidiMessage::noteOn(1, 67, (juce::uint8)100), 2.0);   // Нота G4
    //sequence.addEvent(juce::MidiMessage::noteOff(1, 67), 2.5);

    engine.AddAudioClip(0, "Misc/Village_party.wav", 0.0, false);
  // engine.AddAudioClip(1, "Misc/Step5.wav", 1, true);
	//engine.AddMidiClip(2, sequence, 0.0);
}

Q_INVOKABLE void ViewModel::togglePlayback()
{
    if (engine.IsPlaying()) {
        engine.StopMix();
        m_isPlaying = false;
    }
    else {
        engine.PlayMix();
        m_isPlaying = true;
    }
    emit isPlayingChanged();

    qDebug() << (m_isPlaying ? "Track is playing" : "Track is stopped");
}

Q_INVOKABLE void ViewModel::moveClip(size_t trackIdx, size_t clipIdx, double newStartTime)
{
    engine.MoveClip(trackIdx, clipIdx, newStartTime);
}

bool ViewModel::isPlaying() const
{
    return m_isPlaying;
}

int ViewModel::volume() const
{
    return m_volume;
}

void ViewModel::setVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        // Здесь можно добавить установку громкости в engine
        emit volumeChanged();
        qDebug() << "Volume changed to:" << m_volume;
    }
}
Q_INVOKABLE void ViewModel::setPlayheadPosition(double position) {
    if (position != m_playheadPosition) {
        m_playheadPosition = position;
        engine.SetPlayheadPosition(position);
        emit playheadPositionChanged(position);
        qDebug() << "Playhead position changed to:" << position;
    }
}