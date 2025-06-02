
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
    property real zoomFactor: 1.0

    // Настройка midiModel из контекста
    Connections {
        target: midiModel
        function onTrackIndexChanged() {
            if (midiModel.trackIndex !== pianoRoll.trackIndex) {
                midiModel.setTrackIndex(pianoRoll.trackIndex)
            }
        }
        function onClipIndexChanged() {
            if (midiModel.clipIndex !== pianoRoll.clipIndex) {
                midiModel.setClipIndex(pianoRoll.clipIndex)
            }
        }
    }

    Component.onCompleted: {
        midiModel.setTrackIndex(pianoRoll.trackIndex)
        midiModel.setClipIndex(pianoRoll.clipIndex)
        console.log("PianoView: Initialized midiModel with trackIndex=", midiModel.trackIndex, "clipIndex=", midiModel.clipIndex)
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

                // Связываем вертикальную прокрутку с сеткой
                

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

                property real beatWidth: baseBeatWidth * zoomFactor
                property int countOfBeats: countOfBeats

                onBeatWidthChanged: {
                    contentWidth = countOfBeats * beatWidth
                    console.log("PianoRoll beatWidth updated: beatWidth=", beatWidth, "zoomFactor=", zoomFactor)
                }

                // Синхронизация прокрутки с flickableArea из MainWindow.qml
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

                // Фон сетки с чередованием цветов
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
                    model: midiModel
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
                                console.log("Note selected: noteNumber=", model.noteNumber, "startBeats=", model.startBeats)
                            }

                            onReleased: {
                                let newStartBeats = parent.x / pianoRollFlickable.beatWidth
                                let snappedStart = Math.round(newStartBeats * 16) / 16
                                parent.x = snappedStart * pianoRollFlickable.beatWidth
                                midiModel.updateNote(index, model.noteNumber, snappedStart, model.durationBeats, model.velocity, model.channel)
                                parent.z = 4
                                console.log("Note moved: newStartBeats=", snappedStart)
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
                                console.log("Resizing note at index=", index)
                            }

                            onReleased: {
                                let newDurationBeats = parent.width / pianoRollFlickable.beatWidth
                                let snappedDuration = Math.round(newDurationBeats * 16) / 16
                                parent.width = snappedDuration * pianoRollFlickable.beatWidth
                                midiModel.updateNote(index, model.noteNumber, model.startBeats, snappedDuration, model.velocity, model.channel)
                                parent.z = 4
                                console.log("Note resized: durationBeats=", snappedDuration)
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
                            midiModel.addNote(noteNumber, snappedBeat, 1.0, 100.0, 1)
                            console.log("Added note: noteNumber=", noteNumber, "startBeats=", snappedBeat)
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
    
}