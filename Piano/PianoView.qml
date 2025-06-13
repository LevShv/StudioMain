import QtQuick 
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Shapes 
Item {
    id: pianoRoll
    Layout.fillWidth: true
    Layout.fillHeight: true

    // Properties passed from MainWindow.qml
    property int trackIndex: 0
    property int clipIndex: 0
    property real clipDuration: viewModel.midiModel.clipDuration
    property real beatWidth: 50 // 1/16th beat = 50 pixels
    property int divisionsPerBeat: 4 // Количество делений на один бит (1/4 ноты)
    property double lastNoteDuration: 1.0 / divisionsPerBeat // Начальное значение (1/4 бита)
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

    // Main container
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#2E3440"

            RowLayout {
                anchors.centerIn: parent
                spacing: 10
                implicitWidth: 300
                RoundButton {
                    id: muteButton
                    width: 30
                    height: 30
                    radius: width / 2
                    ToolTip.visible: hovered
                    ToolTip.delay: 500
                    ToolTip.text: isMuted ? "Unmute" : "Mute"

                    property bool isMuted: false // По умолчанию unmuted

                    background: Rectangle {
                        radius: parent.radius
                        color: muteButton.hovered ? "#d0d0d0" : "transparent"
                        border.color: muteButton.hovered ? "#a0a0a0" : "transparent"
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 100 } }
                        Behavior on border.color { ColorAnimation { duration: 100 } }
                    }

                    Image {
                        id: muteIcon
                        anchors.centerIn: parent
                        width: 18
                        height: 18
                        source: parent.isMuted ? imagesPath + "muted.png" : imagesPath + "unmuted.png" // Зависит от model.muted
                        opacity: muteButton.down ? 0.7 : 1.0
                        fillMode: Image.PreserveAspectFit
                        Behavior on opacity { NumberAnimation { duration: 100 } }
                    }

                    onClicked: {
                        isMuted = !isMuted
                        muteButton.scale = 0.95
                        console.log("Mute button toggled: isMuted=", isMuted)
                    }

                    Behavior on scale {
                        NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                    }
                }
                RoundButton {
                    id: colorButton
                    width: 30
                    height: 30
                    radius: width / 2
                    ToolTip.visible: hovered
                    ToolTip.delay: 500
                    ToolTip.text: "Выбрать цвет дорожки"

                    background: Rectangle {
                        radius: parent.radius
                        color: colorButton.hovered ? "#d0d0d0" : "transparent"
                        border.color: colorButton.hovered ? "#a0a0a0" : "transparent" // Обводка при наведении, как у muteButton в Main.qml
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 100 } }
                        Behavior on border.color { ColorAnimation { duration: 100 } }
                    }

                    Image {
                        id: colorIcon
                        anchors.centerIn: parent
                        width: 18
                        height: 18
                        source: imagesPath + "color.png"
                        opacity: colorButton.down ? 0.7 : 1.0
                        fillMode: Image.PreserveAspectFit
                        Behavior on opacity { NumberAnimation { duration: 100 } }
                    }
                    property color currentColor: "#FFFFFF"

                    onClicked: colorDialog.open()
                    Behavior on scale {
                        NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                    }

                    ColorDialog {
                        id: colorDialog
                        title: "Выберите цвет дорожки"
                        modality: Qt.WindowModal
                        onAccepted: {
                            colorButton.currentColor = selectedColor
                            viewModel.changeColor(trackIndex, clipIndex, selectedColor)
                            console.log("Color selected:", selectedColor, "for track:", trackIndex, "clip:", clipIndex)
                        }
                        onRejected: {
                            console.log("Color selection canceled")
                        }
                    }
                }
                Label {
                    text: "Редактор: Дорожка " + (trackIndex + 1) + ", Клип " + (mainWindow.displayedClipIndex + 1)
                    color: "#ECEFF4"
                    font.pixelSize: 12
                }
            }
        }


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
                    width: 60
                    Layout.fillHeight: true
                    color: "#2E3440"    
                    // Белая граница справа
                    Rectangle {
                        anchors.right: parent.right
                        width: 1
                        height: parent.height
                        color: "white"
                    }
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
                        color: "#2D2D2D"
                        border.color: "white"  // Белая обводка
                        border.width: 1 

                        // Ruler beat divisions
                        Repeater {
                            model: Math.ceil(clipDuration * divisionsPerBeat)  // Number of 1/16th beats
                            delegate: Item {
                                x: index * beatWidth
                                width: beatWidth
                                height: 20

                                property bool isStrongBeat: index % divisionsPerBeat === 0 // Whole beat
                                property bool isQuarterBeat: index % (divisionsPerBeat/4) === 0 // Quarter beat (1/4)

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
                                            return Math.floor(index / divisionsPerBeat)  // Whole beat number (e.g., "1", "2")
                                        } else if (parent.isQuarterBeat) {
                                            let quarterBeat = (index % divisionsPerBeat) / (divisionsPerBeat/4)  // Quarter beat within whole beat
                                            return Math.floor(index / divisionsPerBeat) + "." + quarterBeat // e.g., "1.1", "1.2", "1.3", "1.4"
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
                width: 60
                Layout.fillHeight: true
                z: 2
                contentHeight: pianoKeysColumn.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                interactive: false // Отключаем взаимодействие с пользователем
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
                contentWidth: clipDuration * divisionsPerBeat * beatWidth
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
                    model: Math.ceil(clipDuration * divisionsPerBeat)  // Number of 1/16th beats
                    delegate: Rectangle {
                        width: 1
                        height: pianoRollFlickable.contentHeight
                        x: index * beatWidth
                        color: "#444"
                        property bool isStrongBeat: index % divisionsPerBeat === 0 // Every full beat
                        opacity: isStrongBeat ? 0.8 : 0.4
                        visible: isStrongBeat || (index % 2 === 0)
                    }
                }

                // Notes
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

                        // Минимальная ширина ноты (1 деление)
                        readonly property real minWidth: 10

                        Component.onCompleted: {
                            console.log("PianoView: Note loaded: noteNumber=", model.noteNumber, 
                                      "startBeats=", model.startBeats, "durationBeats=", model.durationBeats)
                        }

                        property real tempDurationBeats: model.durationBeats
                        property var snapIndex: 16

                        // Белая зона для растягивания справа
                        Rectangle {
                            id: resizeHandle
                            width: 4
                            height: parent.height
                            anchors.right: parent.right
                            color: "white"
                            opacity: 0.5
                            visible: false
                        }

                        // Основная MouseArea для перемещения и удаления ноты
                        MouseArea {
                            id: dragArea
                            anchors.fill: parent
                            anchors.rightMargin: resizeHandle.width // Оставляем место для resizeHandle
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

                        // MouseArea только для растягивания (в белой зоне)
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
                    
                                    // Привязка к сетке
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
                                    // Обновляем длительность для новых нот
                                        pianoRoll.lastNoteDuration = snappedDuration
                                    viewModel.midiModel.updateNote(index, model.noteNumber, model.startBeats,
                                                                snappedDuration, model.velocity, model.channel)

                                    console.log("PianoView: Note resized: durationBeats=", snappedDuration)
                                }
                            }
                        }
                    }
                }

                // Playback indicator
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
                        return -5 // Показываем в начале клипа, если вне диапазона
                    }
                    z: 5
                    visible: true // Всегда видим
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
                    y: pianoRollFlickable.contentY // Фиксируем в верхней части видимой области
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
                // Adding new note on click
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton 
                    onClicked: (mouse) => {


                        // Учитываем прокрутку при расчете позиции
                        let absoluteX = mouse.x 
                        let absoluteY = mouse.y 
                        
                        let beat = absoluteX / (beatWidth * divisionsPerBeat)
                        let snappedBeat = Math.round(beat * divisionsPerBeat) / divisionsPerBeat // Привязка к 1/4 бита
                        
                        // Вычисляем номер ноты с учетом прокрутки
                        let noteNumber = 127 - Math.floor(absoluteY / 20)
                        noteNumber = Math.max(0, Math.min(127, noteNumber)) // Ограничиваем диапазон
                        
                        if (noteNumber >= 0 && noteNumber <= 127 && snappedBeat >= 0 && trackIndex >= 0 && clipIndex >= 0) {
                            // Используем сохраненную длительность
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