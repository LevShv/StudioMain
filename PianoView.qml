
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Shapes 1.15


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
                midiModel.setTrackIndex(pianoRoll.trackIndex);
            }
        }
        function onClipIndexChanged() {
            if (midiModel.clipIndex !== pianoRoll.clipIndex) {
                midiModel.setClipIndex(pianoRoll.clipIndex);
            }
        }
    }

    Component.onCompleted: {
        midiModel.setTrackIndex(pianoRoll.trackIndex);
        midiModel.setClipIndex(pianoRoll.clipIndex);
        console.log("PianoView: midiModel initialized, trackIndex=", midiModel.trackIndex, "clipIndex=", midiModel.clipIndex);
    }

    // Основной контейнер
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Панель инструментов
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
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
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Клавиши
            Rectangle {
                id: keysPanel
                Layout.preferredWidth: 50
                Layout.fillHeight: true
                color: "#3B4252"

                ListView {
                    id: keysView
                    anchors.fill: parent
                    model: 128 // MIDI-ноты 0–127
                    interactive: false
                    clip: true
                    verticalLayoutDirection: ListView.BottomToTop

                    delegate: Rectangle {
                        width: keysPanel.width
                        height: 20
                        color: (index % 12 === 1 || index % 12 === 3 || index % 12 === 6 || index % 12 === 8 || index % 12 === 10) ? "#2E3440" : "#ECEFF4"
                        border.color: "#4C566A"
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: {
                                var noteNames = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
                                var octave = Math.floor(index / 12) - 1;
                                var note = index % 12;
                                return noteNames[note] + octave;
                            }
                            color: (index % 12 === 1 || index % 12 === 3 || index % 12 === 6 || index % 12 === 8 || index % 12 === 10) ? "#ECEFF4" : "#2E3440"
                            font.pixelSize: 10
                        }
                    }
                }
            }

            // Сетка и ноты
            Flickable {
                id: pianoRollFlickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: countOfBeats * beatWidth
                contentHeight: 128 * 20 // 128 нот по 20 пикселей
                clip: true
                property real beatWidth: baseBeatWidth * zoomFactor
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalAndVerticalFlick

                onBeatWidthChanged: {
                    contentWidth = countOfBeats * beatWidth
                    console.log("PianoRoll beatWidth updated: beatWidth=", beatWidth, "zoomFactor=", zoomFactor)
                }

                // Синхронизация прокрутки с flickableArea из MainWindow.qml
                Binding {
                    target: pianoRollFlickable
                    property: "contentX"
                    value: flickableArea.contentX
                    when: !pianoRollFlickable.moving
                }
                Binding {
                    target: flickableArea
                    property: "contentX"
                    value: pianoRollFlickable.contentX
                    when: !flickableArea.moving
                }

                // Сетка (вертикальные линии)
                Repeater {
                    model: countOfBeats * 4 // 1/4 ноты
                    Rectangle {
                        x: index * (pianoRollFlickable.beatWidth / 4)
                        y: 0
                        width: index % 4 === 0 ? 2 : 1
                        height: pianoRollFlickable.contentHeight
                        color: index % 4 === 0 ? "#4C566A" : "#3B4252"
                        visible: {
                            var itemX = x - pianoRollFlickable.contentX
                            return itemX > -width && itemX < pianoRollFlickable.width + width
                        }
                    }
                }

                // Горизонтальные линии (ноты)
                Repeater {
                    model: 128
                    Rectangle {
                        x: 0
                        y: index * 20
                        width: pianoRollFlickable.contentWidth
                        height: 1
                        color: (index % 12 === 1 || index % 12 === 3 || index % 12 === 6 || index % 12 === 8 || index % 12 === 10) ? "#4C566A" : "#3B4252"
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
                                var newStartBeats = parent.x / pianoRollFlickable.beatWidth
                                var snappedStart = Math.round(newStartBeats * 16) / 16 // Привязка к 1/16
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
                            }

                            onReleased: {
                                var newDurationBeats = parent.width / pianoRollFlickable.beatWidth
                                var snappedDuration = Math.round(newDurationBeats * 16) / 16
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
                        var beat = mouse.x / pianoRollFlickable.beatWidth
                        var snappedBeat = Math.round(beat * 16) / 16
                        var noteNumber = 127 - Math.floor(mouse.y / 20)
                        if (noteNumber >= 0 && noteNumber < 128 && snappedBeat >= 0) {
                            midiModel.addNote(noteNumber, snappedBeat, 1.0, 100.0, 1) // Длительность 1 бит, velocity 100, канал 1
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