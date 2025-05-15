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
    property int countOfTracks: viewModel.trackModel.countOfTracks

    Component.onCompleted: {
        console.log("MainWindow dragParent:", dragParent)
    }

    Connections {
        target: viewModel
        function onIsPlayingChanged() {
            console.log("Playback state changed, isPlaying:", viewModel.isPlaying)
            if (!viewModel.isPlaying) {
               // greenline.x = 0 // Сбрасываем позицию при остановке
            }
        }

        function onPlayheadPositionChanged(position) {
            if (!greenlineMouseArea.drag.active) {
                greenline.x = position * flickableArea.beatWidth
                // Ограничиваем позицию, чтобы не выходить за границы
                greenline.x = Math.max(0, Math.min(greenline.x, contentGrid.width - greenline.width))
                console.log("Greenline updated to position:", position, "x:", greenline.x)
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

                    Rectangle {
                        id: logo
                        width: 80   // уменьшено с 100 до 80
                        height: 40  // уменьшено с 50 до 40
                        anchors.left: parent.left
                        anchors.leftMargin: 20   // отступ слева
                        anchors.verticalCenter: parent.verticalCenter   // по вертикали по центру
                        color: "#222"   // темный фон (можно выбрать другой темный цвет)
                        radius: 8      // скругление углов
                        border.color: "black"
                        border.width: 2

                        // Контейнер для текста, центрированный внутри logo
                        Item {
                            anchors.fill: parent

                            // Тень (золотой цвет)
                            Text {
                                text: "𝓛𝓮𝓣𝓸"
                                font.pixelSize: Math.min(parent.width, parent.height) * 0.6   // чуть больше размера шрифта
                                font.bold: true
                                color: "gold"   // золотой цвет для тени
                                anchors.centerIn: parent
                                x: 3  // смещение для тени
                                y: 3
                            }

                            // Основной текст поверх тени, черный или светлый для контраста
                            Text {
                                text: "𝓛𝓮𝓣𝓸"
                                font.pixelSize: Math.min(parent.width, parent.height) * 0.6
                                font.bold: true
                                color: "#ffd700"   // белый цвет текста для хорошего контраста на темном фоне
                                anchors.centerIn: parent
                            }
                        }
                    }
                    radius: 8
                    border.color: "white"
                    border.width: 1
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

                    
                    radius: 8
                    border.color: "white"
                    border.width: 1
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
                                model: countOfTracks
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
                        // Внутри Flickable области
                        Flickable {
                            id: flickableArea
                            property int countOfBeats: 100
                            property int beatWidth: 40
                            property int widthOfAllArea: countOfBeats * beatWidth

                            Component.onCompleted: {
                                console.log("Initial widthOfAllArea:", widthOfAllArea)
                                console.log("contentWidth:", contentWidth)
                                console.log("contentGrid.width:", contentGrid.width)
                            }

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            contentWidth: widthOfAllArea
                            contentHeight: timeRuler.height + contentGrid.height
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.HorizontalFlick

                            // Временная линейка
                            Rectangle {
                                id: timeRuler
                                width: flickableArea.widthOfAllArea // Явная ссылка на flickableArea.widthOfAllArea
                                height: 50
                                color: "#1E1E1E"
                                z: 2

                                Row {
                                    anchors.fill: parent
                                    spacing: 0

                                    Repeater {
                                        model: flickableArea.countOfBeats // Используем свойство
                                        Rectangle {
                                            width: flickableArea.beatWidth // Используем свойство
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

                            // Сетка треков
                            Item {
                                id: contentGrid
                                anchors.top: timeRuler.bottom
                                width: flickableArea.widthOfAllArea // Синхронизируем с widthOfAllArea
                                height: 15 * 50

                                Component.onCompleted: {
                                    console.log("contentGrid.width:", width)
                                    console.log("contentGrid.height:", height)
                                }

                                // Клипы и фоновые области треков
                                Item {
                                    id: tracksAndClipsContainer
                                    anchors.fill: parent

                                    // 1. Фоновые прямоугольники треков
                                    Repeater {
                                        model: viewModel.trackModel
                                        delegate: Rectangle {
                                            property int trackIndex: model.trackIndex || 0
                                            width: contentGrid.width
                                            height: 50
                                            y: trackIndex * 50
                                            z: 0
                                            color: "#2D2D2D"
                                            border {
                                                width: 1
                                                color: "#444"
                                            }
                                        }
                                    }

                                    // 2. Вертикальные полосы
                                    Row {
                                        anchors.fill: parent
                                        spacing: 0
                                        z: 1

                                        Repeater {
                                            model: flickableArea.countOfBeats // Синхронизируем с countOfBeats
                                            Rectangle {
                                                width: flickableArea.beatWidth // Используем beatWidth
                                                height: parent.height
                                                color: "transparent"
                                                border.width: 1
                                                border.color: "#444"
                                            }
                                        }
                                    }

                                    // 3. Клипы
                                    Repeater {
                                        id: tracksRepeater
                                        model: viewModel.trackModel

                                        delegate: Item {
                                            id: trackItem
                                            property int trackIndex: model.trackIndex
                                            property var trackData: model.data || {}

                                            width: contentGrid.width
                                            height: 50
                                            y: trackIndex * 50
                                            z: 2

                                            Component.onCompleted: {
                                                console.log("Track index:", trackIndex)
                                                console.log("Track clips:", trackData.clips)
                                            }

                                            Repeater {
                                                model: trackData.clips || []

                                                delegate: Rectangle {
                                                    id: clipRectangle
                                                    property var clipModel: modelData

                                                    x: (clipModel.startBeats || 0) * flickableArea.beatWidth // Используем beatWidth
                                                    width: (clipModel.durationBeats || 1) * flickableArea.beatWidth // Используем beatWidth
                                                    height: 48
                                                    color: clipModel.type === "audio" ? "#FF5722" : "#4CAF50"
                                                    radius: 3
                                                    border.width: 1
                                                    border.color: Qt.darker(color, 1.2)

                                                    Label {
                                                        anchors.fill: parent
                                                        text: clipModel.file ? clipModel.file.split("/").pop() : "MIDI Clip"
                                                        color: "white"
                                                        font.pixelSize: 10
                                                        padding: 5
                                                        elide: Text.ElideRight
                                                        verticalAlignment: Text.AlignVCenter
                                                    }

                                                    MouseArea {
                                                        anchors.fill: parent
                                                        drag.target: clipRectangle
                                                        drag.axis: Drag.XAxis
                                                        drag.minimumX: 0
                                                        drag.maximumX: Math.max(0, contentGrid.width - clipRectangle.width) // Предотвращаем ошибку

                                                        onPressed: {
                                                            console.log("Drag started at:", clipRectangle.x)
                                                            clipRectangle.z = 3
                                                        }

                                                        onReleased: {
                                                            var snappedX = Math.round(clipRectangle.x / flickableArea.beatWidth) * flickableArea.beatWidth
                                                            clipRectangle.x = snappedX
                                                            clipRectangle.z = 2
                                                            console.log("Clip dropped at:", snappedX)

                                                            viewModel.moveClip(
                                                                trackIndex,
                                                                index,
                                                                snappedX / flickableArea.beatWidth
                                                            )
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Линия воспроизведения
                            // Линия воспроизведения
                            // Линия воспроизведения
                            Rectangle {
                                id: greenline
                                width: 2
                                height: contentGrid.height
                                color: "green"
                                z: 10
                                x: viewModel.playheadPosition * flickableArea.beatWidth
                                anchors.top: contentGrid.top

                                MouseArea {
                                    id: greenlineMouseArea
                                    anchors.fill: parent
                                    drag.target: parent
                                    drag.axis: Drag.XAxis
                                    drag.minimumX: 0
                                    drag.maximumX: Math.max(0, contentGrid.width - parent.width)

                                    onPressed: {
                                        console.log("Greenline drag started at:", greenline.x)
                                    }

                                    onReleased: {
                                        var snappedX = Math.round(greenline.x / flickableArea.beatWidth) * flickableArea.beatWidth
                                        greenline.x = snappedX
                                        var newPosition = snappedX / flickableArea.beatWidth
                                        viewModel.setPlayheadPosition(newPosition)
                                        console.log("Greenline dropped at:", snappedX, "position:", newPosition)
                                    }
                                }

                                Connections {
                                    target: viewModel
                                    function onPlayheadPositionChanged(position) {
                                        if (!greenlineMouseArea.drag.active) {
                                            greenline.x = position * flickableArea.beatWidth
                                            greenline.x = Math.max(0, Math.min(greenline.x, contentGrid.width - greenline.width))
                                            console.log("Greenline updated to position:", position, "x:", greenline.x)
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