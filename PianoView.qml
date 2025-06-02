import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: pianoRoll
    Layout.fillWidth: true
    Layout.fillHeight: true

    // Свойства, переданные из MainWindow.qml
    property int trackIndex: 0
    property int clipIndex: 0
    property real baseBeatWidth: 50
    property int countOfBeats: 10000
    property real zoomLevel: 1.0
    // Настройка midiModel из контекста
    Connections {
        target: viewModel.midiModel
        function onTrackIndexChanged() {
            if (viewModel.midiModel.trackIndex !== pianoRoll.trackIndex) {
                viewModel.midiModel.setTrackIndex(pianoRoll.trackIndex)
                console.log("PianoView: viewModel.viewModel.midiModel trackIndex updated to", pianoRoll.trackIndex)
            }
        }
        function onClipIndexChanged() {
            if (viewModel.midiModel.clipIndex !== pianoRoll.clipIndex) {
                viewModel.midiModel.setClipIndex(pianoRoll.clipIndex)
                console.log("PianoView: viewModel.viewModel.midiModel clipIndex updated to", pianoRoll.clipIndex)
            }
        }
    }

    Component.onCompleted: {
        viewModel.midiModel.setTrackIndex(pianoRoll.trackIndex)
        viewModel.viewModel.midiModel.setClipIndex(pianoRoll.clipIndex)
        viewModel.midiModel.refresh()
        console.log("PianoView: Initialized viewModel.midiModel with trackIndex=", viewModel.midiModel.trackIndex,
                    "clipIndex=", viewModel.midiModel.clipIndex)
        console.log("PianoView: baseBeatWidth=", baseBeatWidth, "countOfBeats=", countOfBeats,
                    "zoomFactor=", zoomFactor)
    }

    // Основной контейнер
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Панель инструментов
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

        // Основная область
        RowLayout {
            id: pianoRollLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Колонка с клавишами пианино
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
                        model: 128 // Диапазон MIDI нот
                        delegate: Rectangle {
                            width: pianoKeysColumn.width
                            height: 20
                            color: {
                                let note = 127 - index
                                let octaveNote = note % 12
                                if ([1, 3, 6, 8, 10].includes(octaveNote)) {
                                    return "#333333" // Чёрные клавиши
                                } else {
                                    return "#555555" // Белые клавиши
                                }
                            }
                            border.color: "#444"

                            // Разделение на октавы
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
                                    let noteNames = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
                                    let note = 127 - index
                                    let octave = Math.floor(note / 12)
                                    let noteName = noteNames[note % 12]
                                    return noteName + octave
                                }
                                color: "white"
                                font.pixelSize: 8
                                visible: height > 15
                                font.bold: (127 - index) % 12 === 0
                            }
                        }
                    }
                }
            }

            // Сетка Piano Roll
            Flickable {
                id: pianoRollFlickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: countOfBeats * beatWidth
                contentHeight: 128 * 20
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalAndVerticalFlick

                property real beatWidth: baseBeatWidth * zoomLevel > 0 ? baseBeatWidth * zoomLevel : 50
                property int countOfBeats: pianoRoll.countOfBeats > 0 ? pianoRoll.countOfBeats : 10000

                onBeatWidthChanged: {
                    contentWidth = countOfBeats * beatWidth
                    console.log("PianoRoll beatWidth updated: beatWidth=", beatWidth, "zoomLevel=", zoomLevel, "contentWidth=", contentWidth)
                }
                Binding {
                    target: pianoRollFlickable
                    property: "contentY"
                    value: pianoKeysFlickable.contentY
                    when: !pianoRollFlickable.movingVertically
                }

                Binding {
                    target: pianoRollFlickable
                    property: "contentX"
                    value: flickableArea.contentX
                    when: !pianoRollFlickable.movingHorizontally
                }
                Binding {
                    target: flickableArea
                    property: "contentX"
                    value: pianoRollFlickable.contentX
                    when: !flickableArea.movingHorizontally
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    hoverEnabled: true

                    onWheel: (wheel) => {
                        var cursorX = Math.max(0, Math.min(wheel.x - pianoRollFlickable.contentX, pianoRollFlickable.width))
                        if (isNaN(cursorX)) {
                            cursorX = pianoRollFlickable.width / 2
                        }
                        var contentCursorX = cursorX + pianoRollFlickable.contentX
                        var oldBeatWidth = baseBeatWidth * pianoRoll.zoomLevel
                        var currentBeat = oldBeatWidth > 0 ? contentCursorX / oldBeatWidth : 0
                        var delta = wheel.angleDelta.y / 120
                        var newZoom = Math.max(0.2, Math.min(10.0, pianoRoll.zoomLevel + delta * 0.1))
                        pianoRoll.zoomLevel = newZoom
                        var newBeatWidth = baseBeatWidth * pianoRoll.zoomLevel
                        pianoRollFlickable.contentWidth = countOfBeats * newBeatWidth
                        pianoRollFlickable.contentX = currentBeat * newBeatWidth - cursorX
                        pianoRollFlickable.contentX = Math.max(0, Math.min(pianoRollFlickable.contentX, pianoRollFlickable.contentWidth - pianoRollFlickable.width))
                        console.log("PianoRoll zoom: cursorX=", cursorX, "newZoom=", newZoom, "newContentX=", pianoRollFlickable.contentX)
                    }
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
                                return "#252525" // Темнее для чёрных клавиш
                            } else {
                                return "#2D2D2D" // Светлее для белых клавиш
                            }
                        }

                        // Линия разделения октав
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#666"
                            visible: (127 - index) % 12 === 0
                            anchors.bottom: parent.bottom
                        }
                    }
                }

                // Вертикальные линии (разделение битов)
                Repeater {
                    model: pianoRollFlickable.countOfBeats + 1
                    delegate: Rectangle {
                        width: 1
                        height: pianoRollFlickable.contentHeight
                        x: index * pianoRollFlickable.beatWidth
                        color: "#444"
                        property bool isStrongBeat: index % 4 === 0
                        opacity: isStrongBeat ? 0.8 : 0.4
                        visible: isStrongBeat || (index % 2 === 0)
                    }
                }

                // Ноты
                Repeater {
                    model: viewModel.midiModel
                    delegate: Rectangle {
                        x: model.startBeats * pianoRollFlickable.beatWidth
                        y: (127 - model.noteNumber) * 20
                        width: model.durationBeats * pianoRollFlickable.beatWidth
                        height: 20
                        color: "#D08770"
                        border.color: "#BF616A"
                        border.width: 1
                        z: 4

                        MouseArea {
                            anchors.fill: parent
                            drag.target: parent
                            drag.axis: Drag.XAxis
                            drag.minimumX: 0
                            drag.maximumX: pianoRollFlickable.contentWidth - parent.width

                            onPressed: {
                                parent.z = 5
                                console.log("Note selected: noteNumber=", model.noteNumber, "startBeats=", model.startBeats,
                                            "trackIndex=", trackIndex, "clipIndex=", clipIndex)
                            }

                            onReleased: {
                                let newStartBeats = parent.x / pianoRollFlickable.beatWidth
                                let snappedStart = Math.round(newStartBeats * 16) / 16
                                parent.x = snappedStart * pianoRollFlickable.beatWidth
                                viewModel.updateMidiNote(trackIndex, clipIndex, index, model.noteNumber, snappedStart,
                                                         model.durationBeats, model.velocity, model.channel)
                                parent.z = 4
                                console.log("Note moved: newStartBeats=", snappedStart, "trackIndex=", trackIndex, "clipIndex=", clipIndex)
                            }
                        }

                        // Изменение длительности
                        MouseArea {
                            width: 10
                            height: parent.height
                            anchors.right: parent.right
                            cursorShape: Qt.SizeHorCursor
                            drag.axis: Drag.XAxis
                            drag.minimumX: parent.x + 10

                            onPressed: {
                                parent.z = 5
                                console.log("Resizing note at index=", index, "trackIndex=", trackIndex, "clipIndex=", clipIndex)
                            }

                            onReleased: {
                                let newDurationBeats = parent.width / pianoRollFlickable.beatWidth
                                let snappedDuration = Math.round(newDurationBeats * 16) / 16
                                parent.width = snappedDuration * pianoRollFlickable.beatWidth
                                viewModel.updateMidiNote(trackIndex, clipIndex, index, model.noteNumber, model.startBeats,
                                                         snappedDuration, model.velocity, model.channel)
                                parent.z = 4
                                console.log("Note resized: durationBeats=", snappedDuration, "trackIndex=", trackIndex, "clipIndex=", clipIndex)
                            }
                        }
                    }
                }

                // Индикатор воспроизведения
                Rectangle {
                    id: playheadIndicator
                    width: 2
                    height: parent.height
                    color: "red"
                    x: viewModel.playheadPosition * pianoRollFlickable.beatWidth
                    z: 5
                    visible: viewModel.isPlaying
                }

                // Добавление новой ноты по клику
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: (mouse) => {
                        let beat = mouse.x / pianoRollFlickable.beatWidth
                        let snappedBeat = Math.round(beat * 16) / 16
                        let noteNumber = 127 - Math.floor(mouse.y / 20)
                        if (noteNumber >= 0 && noteNumber < 128 && snappedBeat >= 0) {
                            viewModel.addMidiNote(trackIndex, clipIndex, noteNumber, snappedBeat, 1.0, 100.0, 1)
                            console.log("Added note: noteNumber=", noteNumber, "startBeats=", snappedBeat,
                                        "trackIndex=", trackIndex, "clipIndex=", clipIndex)
                        }
                    }
                }
            }
        }
    }

    // Вертикальная прокрутка
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

    // Горизонтальная прокрутка
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

    // Отладка
    /* Component.onCompleted: {
        console.log("PianoView loaded: trackIndex=", trackIndex, "clipIndex=", clipIndex,
                    "baseBeatWidth=", baseBeatWidth, "countOfBeats=", countOfBeats,
                    "zoomFactor=", zoomFactor)
    } */
}