import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts

Item {
    id: pianoRoll
    Layout.fillWidth: true
    Layout.fillHeight: true

    // Properties passed from MainWindow.qml
    property int trackIndex: 0
    property int clipIndex: 0
    property real clipDuration: viewModel.midiModel.clipDuration 
    property real beatWidth: 50 // 1/16th beat = 50 pixels

    onTrackIndexChanged: {
        console.log("PianoView: trackIndex changed to", trackIndex, "clipDuration=", clipDuration, "contentWidth=", pianoRollFlickable.contentWidth)
        viewModel.midiModel.setTrackIndex(trackIndex)
        viewModel.midiModel.refresh()
        clipDuration = viewModel.midiModel.clipDuration 
    }

    onClipIndexChanged: {
        console.log("PianoView: clipIndex changed to", clipIndex, "clipDuration=", clipDuration, "contentWidth=", pianoRollFlickable.contentWidth)
        viewModel.midiModel.setClipIndex(clipIndex)
        viewModel.midiModel.refresh()
        clipDuration = viewModel.midiModel.clipDuration 
    }

    onClipDurationChanged: {
        console.log("PianoView: clipDuration changed to", clipDuration, "contentWidth=", pianoRollFlickable.contentWidth)
        pianoRollFlickable.contentWidth = clipDuration * 16 * beatWidth
        if (viewModel.midiModel) {
            viewModel.midiModel.refresh()
        }
    }

    Component.onCompleted: {
        console.log("PianoView: Initializing with trackIndex=", trackIndex, "clipIndex=", clipIndex, "clipDuration=", clipDuration)
        console.log("PianoView: beatWidth=", beatWidth, "contentWidth=", pianoRollFlickable.contentWidth)
        viewModel.midiModel.setTrackIndex(trackIndex)
        viewModel.midiModel.setClipIndex(clipIndex)
        viewModel.midiModel.refresh()
        pianoRollFlickable.contentWidth = clipDuration * 16 * beatWidth
    }

    Connections {
        target: viewModel.midiModel
        function onTrackIndexChanged() {
            if (viewModel.midiModel.trackIndex !== pianoRoll.trackIndex) {
                viewModel.midiModel.setTrackIndex(pianoRoll.trackIndex)
                console.log("PianoView: viewModel.midiModel trackIndex updated to", pianoRoll.trackIndex)
            }
        }
        function onClipIndexChanged() {
            if (viewModel.midiModel.clipIndex !== pianoRoll.clipIndex) {
                viewModel.midiModel.setClipIndex(pianoRoll.clipIndex)
                console.log("PianoView: viewModel.midiModel clipIndex updated to", pianoRoll.clipIndex)
            }
        }
        function onClipDurationChanged() {
            console.log("PianoView: midiModel.clipDuration changed to", viewModel.midiModel.clipDuration, "contentWidth=", pianoRollFlickable.contentWidth)
        }
    }

    // Main container
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#2E3440"

            Label {
                anchors.centerIn: parent
                text: "Piano Roll: Track " + (trackIndex + 1) + ", Clip " + (clipIndex + 1)
                color: "#ECEFF4"
                font.pixelSize: 12
            }
        }

        // Ruler
        // Ruler
        // Ruler
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 20
            color: "#2E3440"

            RowLayout {
                anchors.fill: parent
                spacing: 0

                // Empty space for piano keys
                Rectangle {
                    width: 40
                    Layout.fillHeight: true
                    color: "#2E3440"
                }

                // Ruler flickable
                Flickable {
                    id: rulerFlickable
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                    contentWidth: pianoRollFlickable.contentWidth
                    contentHeight: 20
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.HorizontalFlick

                    // Sync ruler with piano roll horizontally
                    Binding {
                        target: rulerFlickable
                        property: "contentX"
                        value: pianoRollFlickable.contentX
                        when: !rulerFlickable.movingHorizontally
                    }
                    Binding {
                        target: pianoRollFlickable
                        property: "contentX"
                        value: rulerFlickable.contentX
                        when: !pianoRollFlickable.movingHorizontally
                    }

                    Rectangle {
                        width: pianoRollFlickable.contentWidth
                        height: 20
                        color: "#2E3440"

                        // Ruler beat divisions
                        Repeater {
                            model: Math.ceil(clipDuration * 16) + 1 // Number of 1/16th beats
                            delegate: Item {
                                x: index * beatWidth
                                width: beatWidth
                                height: 20

                                property bool isStrongBeat: index % 16 === 0 // Whole beat
                                property bool isQuarterBeat: index % 4 === 0 // Quarter beat (1/4)

                                // Vertical line
                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: "#444"
                                    opacity: parent.isStrongBeat ? 0.8 : (parent.isQuarterBeat ? 0.6 : 0.4)
                                    visible: parent.isStrongBeat || parent.isQuarterBeat
                                }

                                // Beat number
                                Label {
                                    x: 2
                                    y: 2
                                    text: {
                                        if (parent.isStrongBeat) {
                                            return Math.floor(index / 16)  // Whole beat number (e.g., "1", "2")
                                        } else if (parent.isQuarterBeat) {
                                            let quarterBeat = (index % 16) / 4 + 1 // Quarter beat within whole beat
                                            return Math.floor(index / 16) + 1 + "." + quarterBeat // e.g., "1.1", "1.2", "1.3", "1.4"
                                        }
                                        return "" // No label for other 1/16th beats
                                    }
                                    color: "#ECEFF4"
                                    font.pixelSize: 10
                                    font.bold: parent.isStrongBeat
                                    visible: parent.isStrongBeat || parent.isQuarterBeat
                                }
                            }
                        }
                    }
                }
            }
        }

        // Main area
        RowLayout {
            id: pianoRollLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Piano keys column
            Flickable {
                id: pianoKeysFlickable
                width: 40
                Layout.fillHeight: true
                z: 2
                contentHeight: pianoKeysColumn.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick

                Binding {
                    target: pianoKeysFlickable
                    property: "contentY"
                    value: pianoRollFlickable.contentY
                    when: !pianoKeysFlickable.movingVertically
                }

                Column {
                    id: pianoKeysColumn
                    width: parent.width

                    Repeater {
                        model: 128 // MIDI note range
                        delegate: Rectangle {
                            width: pianoKeysColumn.width
                            height: 20
                            color: {
                                let note = 127 - index
                                let octaveNote = note % 12
                                if ([1, 3, 6, 8, 10].includes(octaveNote)) {
                                    return "#333333" // Black keys
                                } else {
                                    return "#555555" // White keys
                                }
                            }
                            border.color: "#444"

                            // Octave separation
                            Rectangle {
                                width: parent.width
                                height: 1
                                color: "#666"
                                visible: (127 - index) % 12 === 0
                                anchors.bottom: parent.bottom
                            }

                            Label {
                                anchors.centerIn: parent
                                text: {
                                    let note = 127 - index
                                    let octave = Math.floor(note / 12)
                                    let noteName = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"][note % 12]
                                    return noteName === "C" ? "C" + octave : ""
                                }
                                color: "white"
                                font.pixelSize: 8
                                font.bold: (127 - index) % 12 === 0
                                visible: noteName === "C" // Show only for C notes
                            }
                        }
                    }
                }
            }

            // Piano Roll grid
            Flickable {
                id: pianoRollFlickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: clipDuration * 16 * beatWidth
                contentHeight: 128 * 20
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalAndVerticalFlick

                Component.onCompleted: {
                    // Центрируем по высоте
                    let contentHeight = pianoRollFlickable.contentHeight
                    let visibleHeight = pianoRollFlickable.height
                    pianoRollFlickable.contentY = (contentHeight - visibleHeight) / 2
                    console.log("PianoView: Centered vertically, contentY=", pianoRollFlickable.contentY)
                }

                Binding {
                    target: pianoKeysFlickable
                    property: "contentY"
                    value: pianoRollFlickable.contentY
                    when: !pianoKeysFlickable.movingVertically
                }

                Binding {
                    target: pianoRollFlickable
                    property: "contentX"
                    value: flickableArea.contentX * (beatWidth / (flickableArea.baseBeatWidth / 16))
                    when: !pianoRollFlickable.movingHorizontally
                }
                Binding {
                    target: flickableArea
                    property: "contentX"
                    value: pianoRollFlickable.contentX * ((flickableArea.baseBeatWidth / 16) / beatWidth)
                    when: !flickableArea.movingHorizontally
                }

                Repeater {
                    model: 128
                    delegate: Rectangle {
                        width: pianoRollFlickable.contentWidth
                        height: 20
                        y: index * 20
                        color: {
                            let note = 127 - index
                            let octaveNote = note % 12
                            if ([1, 3, 6, 8, 10].includes(octaveNote)) {
                                return "#252525" // Darker for black keys
                            } else {
                                return "#2D2D2D" // Lighter for white keys
                            }
                        }

                        // Octave separation line
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#666"
                            visible: (127 - index) % 12 === 0
                            anchors.bottom: parent.bottom
                        }
                    }
                }

                // Vertical lines (beat divisions)
                Repeater {
                    model: Math.ceil(clipDuration * 16) + 1 // Number of 1/16th beats
                    delegate: Rectangle {
                        width: 1
                        height: pianoRollFlickable.contentHeight
                        x: index * beatWidth
                        color: "#444"
                        property bool isStrongBeat: index % 16 === 0 // Every full beat
                        opacity: isStrongBeat ? 0.8 : 0.4
                        visible: isStrongBeat || (index % 2 === 0)
                    }
                }

                // Notes
// Notes
                Repeater {
                    model: viewModel.midiModel
                    delegate: Rectangle {
                        x: model.startBeats * beatWidth * 8 // startBeats в 1/16-х долях
                        y: (127 - model.noteNumber) * 20
                        width: model.durationBeats * beatWidth * 8 // durationBeats в целых битах, умножаем на 16
                        height: 20
                        color: "#D08770"
                        border.color: "#BF616A"
                        border.width: 1
                        z: 4

                        Component.onCompleted: {
                            console.log("PianoView: Note loaded: noteNumber=", model.noteNumber, "startBeats=", model.startBeats, "durationBeats=", model.durationBeats)
                        }

                        MouseArea {
                            anchors.fill: parent
                            drag.target: parent
                            drag.axis: Drag.XAxis
                            drag.minimumX: 0
                            drag.maximumX: pianoRollFlickable.contentWidth - parent.width

                            onPressed: {
                                console.log("PianoView: Note selected: noteNumber=", model.noteNumber, "startBeats=", model.startBeats)
                            }

                            onReleased: {
                                let newStartBeats = parent.x / beatWidth
                                let snappedStart = Math.round(newStartBeats * 16) / 16
                                parent.x = snappedStart * beatWidth
                                viewModel.midiModel.updateNote(index, model.noteNumber, snappedStart,
                                                            model.durationBeats, model.velocity, model.channel)
                                console.log("PianoView: Note moved: newStartBeats=", snappedStart)
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            drag.target: parent
                            drag.axis: Drag.XAxis
                            drag.minimumX: 0
                            drag.maximumX: pianoRollFlickable.contentWidth - parent.width

                            onPressed: {
                                console.log("PianoView: Note selected: noteNumber=", model.noteNumber, "startBeats=", model.startBeats);
                            }

                            onReleased: {
                                let newStartBeats = parent.x / (beatWidth * 8); // Учитываем, что startBeats в 1/16 долях
                                let snappedStart = Math.round(newStartBeats * 16) / 16;
                                parent.x = snappedStart * beatWidth * 8;
                                viewModel.midiModel.updateNote(index, model.noteNumber, snappedStart, model.durationBeats, model.velocity, model.channel);
                                console.log("PianoView: Note moved: newStartBeats=", snappedStart);
                            }
                        }
                    }
                }

                // Playback indicator
                Rectangle {
                    id: playheadIndicatorЫ
                    width: 2
                    height: parent.height
                    color: "red"
                    x: viewModel.playheadPosition * beatWidth * 8// Предполагается, что playheadPosition в 1/16-х долях
                    z: 5
                    visible: viewModel.isPlaying
                }
                // Adding new note on click
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: (mouse) => {
                        let beat = mouse.x / (beatWidth * 16) // beatWidth для 1/16, делим на 2 для 1/32
                        let snappedBeat = Math.round(beat * 16) / 16 // Привязка к 1/8 бита (1/32 четвертной)
                        let noteNumber = 127 - Math.floor(mouse.y / 20)
                        if (noteNumber >= 0 && noteNumber <= 127 && snappedBeat >= 0 && trackIndex >= 0 && clipIndex >= 0) {
                            let durationBeats = 1.0 // 1/8 бита = 8 * (1/32)
                            let velocity = 100 / 127.0 // Приводим к [0, 1]
                            let channel = 1
                            viewModel.midiModel.addNote(noteNumber, snappedBeat, durationBeats, velocity, channel)
                            console.log("Added note: noteNumber=", noteNumber, "startBeats=", snappedBeat,
                                        "durationBeats=", durationBeats, "velocity=", velocity, "channel=", channel,
                                        "trackIndex=", trackIndex, "clipIndex=", clipIndex)
                        } else {
                            console.log("Invalid note parameters: noteNumber=", noteNumber, "startBeats=", snappedBeat,
                                        "trackIndex=", trackIndex, "clipIndex=", clipIndex)
                        }
                    }
                }
            }
        }
    }

    // Vertical scrollbar
    ScrollBar {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        policy: ScrollBar.AsNeeded
        active: true
        orientation: Qt.Vertical
        size: pianoRollFlickable.height / pianoRollFlickable.contentHeight
        position: pianoRollFlickable.contentY / pianoRollFlickable.contentHeight
        onPositionChanged: {
            pianoRollFlickable.contentY = position * pianoRollFlickable.contentHeight
        }
    }

    // Horizontal scrollbar
    ScrollBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        policy: ScrollBar.AsNeeded
        active: true
        orientation: Qt.Horizontal
        size: pianoRollFlickable.width / pianoRollFlickable.contentWidth
        position: pianoRollFlickable.contentX / pianoRollFlickable.contentWidth
        onPositionChanged: {
            pianoRollFlickable.contentX = position * pianoRollFlickable.contentWidth
        }
    }
}