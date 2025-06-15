import QtQuick 
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Shapes 
Item {
    id: pianoRoll
    Layout.fillWidth: true
    Layout.fillHeight: true
    
    property int trackIndex: 0
    property int clipIndex: 0
    property real clipDuration: viewModel.midiModel.clipDuration
    property real beatWidth: 50 
    property int divisionsPerBeat: 4 
    property double lastNoteDuration: 1.0 / divisionsPerBeat
    property string imagesPath: ""


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
        pianoRollFlickable.contentWidth = clipDuration * divisionsPerBeat * beatWidth
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
        pianoRollFlickable.contentWidth = clipDuration * divisionsPerBeat * beatWidth
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
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
       
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#2E3440"

            Label {
                anchors.centerIn: parent
                text: "Пианино: Дорожка " + (trackIndex + 1) + ", Клип " + (mainWindow.displayedClipIndex + 1)
                color: "#ECEFF4"
                font.pixelSize: 12
            }
        }


        // Рулетка
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 20
            color: "#2E3440"

            RowLayout {
                anchors.fill: parent
                spacing: 0
                
                Rectangle {
                    width: 60
                    Layout.fillHeight: true
                    color: "#2E3440"
                    Rectangle {
                        anchors.right: parent.right
                        width: 1
                        height: parent.height
                        color: "white"
                    }
                }
                
                Flickable {
                    id: rulerFlickable
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                    contentWidth: pianoRollFlickable.contentWidth
                    contentHeight: 20
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.HorizontalFlick
                    
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
                        color: "#2D2D2D"
                        border.color: "white"
                        border.width: 1 
                        
                        Repeater {
                            model: Math.ceil(clipDuration * divisionsPerBeat)
                            delegate: Item {
                                x: index * beatWidth
                                width: beatWidth
                                height: 20

                                property bool isStrongBeat: index % divisionsPerBeat === 0 
                                property bool isQuarterBeat: index % (divisionsPerBeat/4) === 0
                                
                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: "#444"
                                    opacity: parent.isStrongBeat ? 0.8 : (parent.isQuarterBeat ? 0.6 : 0.4)
                                    visible: parent.isStrongBeat || parent.isQuarterBeat
                                }
                                
                                Label {
                                    x: 2
                                    y: 2
                                    text: {
                                        if (parent.isStrongBeat) {
                                            return Math.floor(index / divisionsPerBeat)
                                        } else if (parent.isQuarterBeat) {
                                            let quarterBeat = (index % divisionsPerBeat) / (divisionsPerBeat/4) 
                                            return Math.floor(index / divisionsPerBeat) + "." + quarterBeat 
                                        }
                                        return "" 
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

        // Основная область
        RowLayout {
            id: pianoRollLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Клавиши
            Flickable {
                id: pianoKeysFlickable
                width: 60
                Layout.fillHeight: true
                z: 2
                contentHeight: pianoKeysColumn.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                interactive: false 
                flickableDirection: Flickable.VerticalFlick

                Binding {
                    target: pianoKeysFlickable
                    property: "contentY"
                    value: pianoRollFlickable.contentY
                }

                Column {
                    id: pianoKeysColumn
                    width: parent.width

                    Repeater {
                        model: 128 
                        delegate: Rectangle {
                            width: pianoKeysColumn.width
                            height: 20
                            color: {
                                let note = 127 - index
                                let octaveNote = note % 12
                                if ([1, 3, 6, 8, 10].includes(octaveNote)) {
                                    return "#333333" 
                                } else {
                                    return "#555555" 
                                }
                            }
                            border.color: "#444"
                            
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
                                visible: noteName === "C" 
                            }
                        }
                    }
                }
            }

            // Сетка
            Flickable {
                id: pianoRollFlickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: clipDuration * divisionsPerBeat * beatWidth
                contentHeight: 128 * 20
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalAndVerticalFlick

                Component.onCompleted: {
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
                                return "#252525" 
                            } else {
                                return "#2D2D2D"
                            }
                        }
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#666"
                            visible: (127 - index) % 12 === 0
                            anchors.bottom: parent.bottom
                        }
                    }
                }

                Repeater {
                    model: Math.ceil(clipDuration * divisionsPerBeat)  
                    delegate: Rectangle {
                        width: 1
                        height: pianoRollFlickable.contentHeight
                        x: index * beatWidth
                        color: "#444"
                        property bool isStrongBeat: index % divisionsPerBeat === 0 
                        opacity: isStrongBeat ? 0.8 : 0.4
                        visible: isStrongBeat || (index % 2 === 0)
                    }
                }

                // Ноты
                Repeater {
                    model: viewModel.midiModel
                    delegate: Rectangle {
                        id: noteRect
                        x: model.startBeats * (beatWidth * divisionsPerBeat)
                        y: (127 - model.noteNumber) * 20
                        width: model.durationBeats * (beatWidth * divisionsPerBeat)
                        height: 20
                        color: "#D08770"
                        border.color: "#BF616A"
                        border.width: 1
                        z: 4
                        
                        readonly property real minWidth: 10                        
                        property real tempDurationBeats: model.durationBeats
                        property var snapIndex: 16

                        Component.onCompleted: {
                            console.log("PianoView: Note loaded: noteNumber=", model.noteNumber, 
                                      "startBeats=", model.startBeats, "durationBeats=", model.durationBeats)
                        }
                        
                        Rectangle {
                            id: resizeHandle
                            width: 4
                            height: parent.height
                            anchors.right: parent.right
                            color: "white"
                            opacity: 0.5
                            visible: false
                        }
                        
                        MouseArea {
                            id: dragArea
                            anchors.fill: parent
                            anchors.rightMargin: resizeHandle.width
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            drag.target: parent
                            drag.axis: Drag.XAndYAxis
                            drag.minimumX: 0
                            drag.maximumX: pianoRollFlickable.contentWidth - parent.width
                            drag.minimumY: (127 - 127) * 20
                            drag.maximumY: (127 - 0) * 20

                            onPressed: {
                                if (mouse.button === Qt.LeftButton) {
                                    console.log("PianoView: Note selected: noteNumber=", model.noteNumber, 
                                        "startBeats=", model.startBeats, "x=", parent.x, "y=", parent.y);
                                }
                            }

                            onPositionChanged: {
                                if (drag.active && mouse.buttons === Qt.LeftButton) {
                                    let newStartBeats = parent.x / (beatWidth * divisionsPerBeat)
                                    let snappedStart = Math.round(newStartBeats * snapIndex) / snapIndex
                                    let newNoteNumber = 127 - Math.floor(parent.y / 20)
                                    newNoteNumber = Math.max(0, Math.min(127, newNoteNumber))
                    
                                    parent.x = snappedStart * beatWidth * divisionsPerBeat
                                    parent.y = (127 - newNoteNumber) * 20
                    
                                    console.log("PianoView: Note dragging: noteNumber=", newNoteNumber, 
                                            "newStartBeats=", snappedStart, "x=", parent.x, "y=", parent.y)
                                }
                            }

                            onReleased: {
                                if (mouse.button === Qt.LeftButton) {
                                    let newStartBeats = parent.x / (beatWidth * divisionsPerBeat)
                                    let snappedStart = Math.round(newStartBeats * snapIndex) / snapIndex
                                    let newNoteNumber = 127 - Math.floor(parent.y / 20)
                                    newNoteNumber = Math.max(0, Math.min(127, newNoteNumber))
                    
                                    viewModel.midiModel.updateNote(index, newNoteNumber, snappedStart,
                                                                model.durationBeats, model.velocity, model.channel)
                                                                
                                    console.log("PianoView: Note moved: noteNumber=", newNoteNumber, 
                                            "newStartBeats=", snappedStart)
                                }
                            }

                            onClicked: {
                                if (mouse.button === Qt.RightButton) {
                                    console.log("PianoView: Right-clicked note: noteNumber=", model.noteNumber,
                                            "startBeats=", model.startBeats, "index=", index)
                                    viewModel.midiModel.deleteNote(index)
                                }
                            }
                        }
                        
                        MouseArea {
                            id: resizeMouseArea
                            anchors.right: parent.right
                            width: resizeHandle.width
                            height: parent.height
                            hoverEnabled: true
                            cursorShape: Qt.SizeHorCursor
                            preventStealing: true

                            onEntered: {
                                resizeHandle.visible = true
                            }
                            onExited: {
                                resizeHandle.visible = false
                            }
                            onPressed: {
                                if (mouse.button === Qt.LeftButton) {
                                    console.log("PianoView: Resizing note: noteNumber=", model.noteNumber,
                                            "startBeats=", model.startBeats, "durationBeats=", model.durationBeats,
                                            "index=", index)
                                }
                            }
                            onPositionChanged: {
                                if (pressed && mouse.buttons === Qt.LeftButton) {
                                    let mouseX = mapToItem(noteRect, mouse.x, mouse.y).x
                                    let newWidth = Math.max(noteRect.minWidth, mouseX)
                                    let newDurationBeats = newWidth / (beatWidth * divisionsPerBeat)
                                    let snappedDuration = Math.round(newDurationBeats * snapIndex) / snapIndex
                                    newWidth = snappedDuration * beatWidth * divisionsPerBeat
                    
                                    noteRect.width = newWidth
                                    noteRect.tempDurationBeats = snappedDuration
                    
                                    console.log("PianoView: Resizing note: index=", index,
                                            "newDurationBeats=", snappedDuration, "newWidth=", newWidth)
                                }
                            }
                            onReleased: {
                                if (mouse.button === Qt.LeftButton) {
                                    let newDurationBeats = noteRect.width / (beatWidth * divisionsPerBeat)
                                    let snappedDuration = Math.round(newDurationBeats * snapIndex) / snapIndex
                                        pianoRoll.lastNoteDuration = snappedDuration
                                    viewModel.midiModel.updateNote(index, model.noteNumber, model.startBeats,
                                                                snappedDuration, model.velocity, model.channel)

                                    console.log("PianoView: Note resized: durationBeats=", snappedDuration)
                                }
                            }
                        }
                    }
                }

                // Красная линия
                Rectangle {
                    id: playheadIndicator
                    width: 2
                    height: parent.height
                    color: "red"
                    x: {
                        let relativePosition = viewModel.playheadPosition - viewModel.midiModel.clipStartTime
                        if (relativePosition >= 0 && relativePosition <= clipDuration) {
                            return relativePosition * beatWidth * divisionsPerBeat
                        }
                        return -5
                    }
                    z: 5
                    visible: true
                    onXChanged: {
                        console.log("PlayheadIndicator: x=", x, 
                                    "playheadPosition=", viewModel.playheadPosition, 
                                    "clipStartTime=", viewModel.midiModel.clipStartTime, 
                                    "clipDuration=", clipDuration)
                    }
                }
                Canvas {
                    id: triangleHandle
                    width: 20
                    height: 10
                    x: playheadIndicator.x-9
                    y: pianoRollFlickable.contentY
                    z: 6

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.beginPath()
                        ctx.moveTo(0, 0)
                        ctx.lineTo(width / 2, height)
                        ctx.lineTo(width, 0)
                        ctx.closePath()
                        ctx.fillStyle = "red"
                        ctx.fill()
                    }

                    MouseArea {
                        id: triangleMouseArea
                        anchors.fill: parent
                        anchors.leftMargin: -14
                        anchors.rightMargin: -14
                        width: 30
                        drag.target: playheadIndicator
                        drag.axis: Drag.XAxis
                        drag.minimumX: 0
                        drag.maximumX: clipDuration * beatWidth * divisionsPerBeat

                        onPositionChanged: {
                            if (drag.active) {
                                let newRelativePosition = playheadIndicator.x / (beatWidth * divisionsPerBeat)
                                let newPlayheadPosition = viewModel.midiModel.clipStartTime + newRelativePosition
                                console.log("PlayheadIndicator: Dragging, newPlayheadPosition=", newPlayheadPosition)
                                viewModel.setPlayheadPosition(newPlayheadPosition)
                            }
                        }

                        onPressed: {
                            viewModel.isPlaying = false
                        }
                    }
                }
                //Добавление нот
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton 
                    onClicked: (mouse) => {
                        let absoluteX = mouse.x 
                        let absoluteY = mouse.y 
                        let beat = absoluteX / (beatWidth * divisionsPerBeat)
                        let snappedBeat = Math.round(beat * divisionsPerBeat) / divisionsPerBeat
                        let noteNumber = 127 - Math.floor(absoluteY / 20)
                        noteNumber = Math.max(0, Math.min(127, noteNumber))
                        
                        if (noteNumber >= 0 && noteNumber <= 127 && snappedBeat >= 0 && trackIndex >= 0 && clipIndex >= 0) {
                            let durationBeats = pianoRoll.lastNoteDuration
                            let velocity = 100 / 127.0
                            let channel = 1
                            viewModel.midiModel.addNote(noteNumber, snappedBeat, durationBeats, velocity, channel)
                            console.log("Added note: noteNumber=", noteNumber, 
                                    "startBeats=", snappedBeat,
                                    "xPos=", snappedBeat * beatWidth * divisionsPerBeat,
                                    "mouse.x=", mouse.x, 
                                    "contentX=", pianoRollFlickable.contentX,
                                    "absoluteX=", absoluteX,
                                    "mouse.y=", mouse.y,
                                    "contentY=", pianoRollFlickable.contentY,
                                    "absoluteY=", absoluteY)
                        } else {
                            console.log("Invalid note parameters: noteNumber=", noteNumber, 
                                    "startBeats=", snappedBeat,
                                    "trackIndex=", trackIndex, 
                                    "clipIndex=", clipIndex)
                        }
                    }
                }
            }
        }
    }
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