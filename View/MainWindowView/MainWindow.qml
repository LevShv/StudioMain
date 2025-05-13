import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import "qrc:/FileBrowser"
import Qt.labs.folderlistmodel

Window {
    id: mainWindow
    visible: true
    width: 1500
    height: 1080
    title: "StudioMain"
    color: "#2E3440"

    property Item dragParent: contentItem

    Component.onCompleted: {
        console.log("MainWindow dragParent:", dragParent)
    }

    Connections {
        target: viewModel
        function onIsPlayingChanged() {
            if (viewModel.isPlaying) {
                greenlineAnimator.start()
            } else {
                greenlineAnimator.stop()
            }
        }

        function onPlayheadPositionChanged(position) {
            if (!greenlineMouseArea.drag.active) {
                greenline.x = position * 40
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Верхняя панель инструментов
        Rectangle {
            id: toolbar
            Layout.fillWidth: true
            Layout.preferredHeight: 102
            color: "#2E3440"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    color: "#4C566A"
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    color: "#4C566A"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        Row {
                            Layout.alignment: Qt.AlignLeft
                            spacing: 5

                            ToolButton {
                                text: "Создать"
                                implicitWidth: 100
                            }
                            ToolButton {
                                text: "Копировать"
                                implicitWidth: 100
                            }
                            ToolButton {
                                text: "Сохранить"
                                implicitWidth: 100
                            }
                        }

                        Row {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 5

                            ToolButton {
                                text: viewModel.isPlaying ? "⏸️" : "▶️"
                                implicitWidth: 60
                                onClicked: {
                                    viewModel.togglePlayback()
                                    if (viewModel.isPlaying) {
                                        greenlineAnimator.resume()
                                    } else {
                                        greenlineAnimator.pause()
                                    }
                                }
                            }
                            ToolButton {
                                text: "⏺️"
                                implicitWidth: 60
                            }
                        }

                        Row {
                            Layout.alignment: Qt.AlignRight
                            spacing: 10

                            Slider {
                                width: 150
                                from: 0
                                to: 100
                                value: viewModel.volume
                                onValueChanged: viewModel.setVolume(value)
                            }
                            Slider {
                                width: 150
                                from: 0
                                to: 100
                            }
                        }
                    }
                }
            }
        }

        // Основная рабочая область
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"

            SplitView {
                anchors.fill: parent
                orientation: Qt.Horizontal

                // Левая панель (FileBrowser)
                Rectangle {
                    id: fileBrowserContainer
                    SplitView.minimumWidth: 200
                    SplitView.preferredWidth: 250
                    color: "transparent"

                    Browser {
                        width: parent.width
                        height: parent.height
                        dragParent: mainWindow.dragParent
                        onCurrentFolderChanged: console.log("Folder changed:", currentFolder)

                        onFileDropped: (filePath, globalX, globalY) => {
                            console.log("Received fileDropped, path:", filePath, "global coords:", globalX, globalY)
                            var localPos = contentGrid.mapFromItem(mainWindow.dragParent, globalX, globalY)
                            console.log("Local coords in contentGrid:", localPos.x, localPos.y)
                            if (localPos.x >= 0 && localPos.x <= contentGrid.width &&
                                localPos.y >= 0 && localPos.y <= contentGrid.height) {
                                var trackIndex = Math.floor((localPos.y - timeRuler.height) / 50)
                                var position = Math.floor(localPos.x / 40)
                                console.log("Calculated trackIndex:", trackIndex, "position:", position)
                                if (trackIndex >= 0 && trackIndex < 10 && position >= 0) {
                                    var fileExt = filePath.toLowerCase().split('.').pop();
                                    if (["mp3", "wav", "aiff", "flac"].indexOf(fileExt) !== -1) {
                                        console.log("File dropped in playlist: track", trackIndex, "position", position, "path", filePath)
                                        viewModel.addAudioClip(trackIndex, filePath, position)
                                    } else {
                                        console.log("Invalid file type:", filePath, "Supported types: mp3, wav, aiff, flac")
                                    }
                                } else {
                                    console.log("Invalid track or position: trackIndex", trackIndex, "position", position)
                                }
                            } else {
                                console.log("File dropped outside contentGrid: localPos.x", localPos.x, "localPos.y", localPos.y)
                            }
                        }
                    }
                }

                // Центральная панель (каналы)
                Rectangle {
                    id: channelRack
                    SplitView.maximumWidth: 250
                    SplitView.minimumWidth: 200
                    SplitView.preferredWidth: 250
                    color: "#2E3440"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Label {
                            text: "Треки"
                            color: "white"
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 10
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            GridView {
                                id: channelsGrid
                                anchors.fill: parent
                                cellWidth: 180
                                cellHeight: 100
                                model: 16

                                delegate: Rectangle {
                                    width: channelsGrid.width
                                    height: channelsGrid.cellHeight - 5
                                    color: index % 2 ? "#3B4252" : "#4C566A"
                                    radius: 5

                                    Column {
                                        anchors.centerIn: parent
                                        spacing: 5

                                        Label {
                                            text: "Channel " + (index + 1)
                                            color: "white"
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        Row {
                                            spacing: 10
                                            anchors.horizontalCenter: parent.horizontalCenter

                                            ToolButton {
                                                text: "Mute"
                                                implicitWidth: 60
                                            }
                                            ToolButton {
                                                text: "Solo"
                                                implicitWidth: 60
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Правая панель (плейлист)
                Rectangle {
                    id: playlistPanel
                    SplitView.fillWidth: true
                    color: "#1E1E1E"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Фиксированные заголовки треков
                        Column {
                            id: trackHeaders
                            width: 100
                            Layout.fillHeight: true
                            Rectangle {
                                width: 100
                                height: 50
                                color: "transparent"
                            }

                            Repeater {
                                model: 10
                                Rectangle {
                                    width: 100
                                    height: 50
                                    color: "#2D2D2D"
                                    border.color: "#444"

                                    Label {
                                        anchors.centerIn: parent
                                        text: "Track " + (index + 1)
                                        color: "#CCC"
                                        font.pixelSize: 12
                                    }
                                }
                            }
                        }

                        // Прокручиваемая область
                        Flickable {
                            id: flickableArea
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            contentWidth: 32 * 40
                            contentHeight: 11 * 50
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.HorizontalFlick

                            // Линейка времени
                            Rectangle {
                                id: timeRuler
                                width: contentGrid.width
                                height: 50
                                color: "#1E1E1E"

                                Row {
                                    anchors.fill: parent
                                    Repeater {
                                        model: 32
                                        Rectangle {
                                            width: 40
                                            height: parent.height
                                            color: "transparent"
                                            border.color: "#444"

                                            Label {
                                                anchors.centerIn: parent
                                                text: index % 4 === 0 ? Math.floor(index/4) + 1 : ""
                                                color: "#CCC"
                                                font.pixelSize: 10
                                            }
                                        }
                                    }
                                }
                            }

                            // Сетка
                            Grid {
                                id: contentGrid
                                columns: 32
                                rows: 10
                                anchors.top: timeRuler.bottom
                                width: 32 * 40

                                Component.onCompleted: {
                                    console.log("contentGrid global pos:", mapToItem(mainWindow.dragParent, 0, 0))
                                }

                                Repeater {
                                    model: 32 * 10
                                    delegate: Rectangle {
                                        width: 40
                                        height: 50
                                        color: "transparent"
                                        border.color: "#444"
                                    }
                                }
                            }

                            // Клипы из TrackModel
                            // Замените блок Repeater для клипов
Repeater {
    model: viewModel.trackModel
    delegate: Item {
        property int trackIndex: model.trackIndex || 0 // Значение по умолчанию, если trackIndex undefined
        Repeater {
            model: model.data ? model.data.clips : [] // Проверяем, существует ли data, иначе пустой массив
            delegate: Rectangle {
                id: clipDelegate
                x: model.startTime * 40
                y: trackIndex * 50 + timeRuler.height
                width: model.duration * 40
                height: 48
                color: model.type === "audio" ? "#FF5722" : "#4CAF50"
                radius: 3
                border.width: 1
                border.color: Qt.darker(color, 1.2)

                Label {
                    anchors.fill: parent
                    text: model.file ? model.file.split("/").pop() : "MIDI Clip"
                    color: "white"
                    font.pixelSize: 10
                    padding: 5
                    elide: Text.ElideRight
                }

                MouseArea {
                    anchors.fill: parent
                    drag.target: parent
                    drag.axis: Drag.XAndYAxis
                    drag.minimumX: 0
                    drag.maximumX: contentGrid.width - parent.width
                    drag.minimumY: timeRuler.height
                    drag.maximumY: timeRuler.height + (contentGrid.rows-1) * 50

                    onPressed: clipDelegate.z = 1
                    onReleased: {
                        clipDelegate.z = 0
                        let newX = Math.round(parent.x / 40) * 40
                        let newY = Math.round((parent.y - timeRuler.height) / 50) * 50 + timeRuler.height
                        parent.x = newX
                        parent.y = newY

                        let newTrackIndex = Math.floor((newY - timeRuler.height) / 50)
                        let newStartTime = newX / 40
                        if (newTrackIndex === trackIndex) {
                            viewModel.moveClip(trackIndex, model.index, newStartTime)
                        } else {
                            console.log("Перемещение между треками не реализовано")
                        }
                    }
                }
            }
        }
    }
}

                            // Зеленая линия воспроизведения
                            Rectangle {
                                id: greenline
                                width: 2
                                height: parent.height
                                color: "green"
                                z: 10
                                x: viewModel.playheadPosition * 40

                                MouseArea {
                                    id: greenlineMouseArea
                                    anchors.fill: parent
                                    drag.target: parent
                                    drag.axis: Drag.XAxis
                                    drag.minimumX: 0
                                    drag.maximumX: contentGrid.width - parent.width

                                    onReleased: {
                                        parent.x = Math.round(parent.x / 40) * 40
                                        viewModel.setPlayheadPosition(parent.x / 40)
                                    }
                                }

                                PropertyAnimation {
                                    id: greenlineAnimator
                                    target: greenline
                                    property: "x"
                                    from: 0
                                    to: contentGrid.width
                                    duration: 50000
                                    loops: Animation.Infinite
                                    running: viewModel.isPlaying
                                }

                                Connections {
                                    target: viewModel
                                    function onPlayheadPositionChanged(position) {
                                        if (!greenlineMouseArea.drag.active) {
                                            greenline.x = position * 40
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}