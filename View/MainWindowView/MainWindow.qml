import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import "qrc:/FileBrowser"
import Qt.labs.folderlistmodel
import QtQuick.Dialogs

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
            console.log("Playback state changed, isPlaying:", viewModel.isPlaying, "position:", viewModel.playheadPosition)
        }
        function onPluginAdded(trackIndex) {
            console.log("Plugin added to track:", trackIndex)
        }
        function onPluginEditorOpened(trackIndex, pluginIndex, window) {
            // Создаем динамическое окно для плагина
            var component = Qt.createComponent("PluginEditorWindow.qml")
            if (component.status === Component.Ready) {
                var pluginWindow = component.createObject(mainWindow, {
                    "trackIndex": trackIndex,
                    "pluginIndex": pluginIndex,
                    "nativeWindow": window
                })
                pluginWindow.show()
            } else {
                console.error("Failed to create PluginEditorWindow:", component.errorString())
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
                    // ... (Логотип без изменений)
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
                            ToolButton { text: "Создать"; implicitWidth: 100 }
                            ToolButton { text: "Копировать"; implicitWidth: 100 }
                            ToolButton { text: "Сохранить"; implicitWidth: 100 }
                        }

                        Row {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 5
                            ToolButton {
                                text: viewModel.isPlaying ? "⏸️" : "▶️"
                                implicitWidth: 60
                                onClicked: viewModel.togglePlayback()
                            }
                            ToolButton { text: "⏺️"; implicitWidth: 60 }
                            ToolButton {
                                text: "⏮️"
                                implicitWidth: 60
                                onClicked: {
                                    viewModel.setPlayheadPosition(0)
                                    console.log("Reset playhead to start")
                                }
                            }
                        }

                        Row {
                            Layout.alignment: Qt.AlignRight
                            spacing: 10
                            Label { text: "Zoom:"; color: "white"; anchors.verticalCenter: parent.verticalCenter }
                            Slider {
                                id: zoomSlider
                                width: 150
                                from: 0.5
                                to: 2.0
                                value: 1.0
                                onValueChanged: {
                                    // Сохраняем пропорциональную прокрутку
                                    var oldContentWidth = flickableArea.contentWidth
                                    var oldContentX = flickableArea.contentX
                                    var ratio = (oldContentX + flickableArea.width / 2) / oldContentWidth
                                    flickableArea.zoomLevel = value
                                    flickableArea.contentX = ratio * flickableArea.contentWidth - flickableArea.width / 2
                                    flickableArea.contentX = Math.max(0, Math.min(flickableArea.contentX, flickableArea.contentWidth - flickableArea.width))
                                    
                                }
                            }
                            Label { text: "BPM:"; color: "white"; anchors.verticalCenter: parent.verticalCenter }
                            Slider {
                                width: 150
                                from: 60
                                to: 200
                                value: viewModel.bpm
                                onValueChanged: viewModel.setBpm(value)
                            }
                            Slider {
                                width: 150
                                from: 0
                                to: 100
                                value: viewModel.volume
                                onValueChanged: viewModel.setVolume(value)
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
                            var localPos = contentGrid.mapFromItem(mainWindow.dragParent, globalX, globalY)
                            if (localPos.x >= 0 && localPos.x <= contentGrid.width &&
                                localPos.y >= 0 && localPos.y <= contentGrid.height) {
                                var trackIndex = Math.floor((localPos.y - timeRuler.height) / 50)
                                var position = Math.floor(localPos.x / flickableArea.beatWidth)
                                if (trackIndex >= 0 && trackIndex < countOfTracks && position >= 0) {
                                    var fileExt = filePath.toLowerCase().split('.').pop();
                                    if (["mp3", "wav", "aiff", "flac"].indexOf(fileExt) !== -1) {
                                        viewModel.addAudioClip(trackIndex, filePath, position)
                                    } else if (["dll", "vst3"].indexOf(fileExt) !== -1) {
                                        viewModel.addPlugin(trackIndex, filePath)
                                    } else {
                                        console.log("Invalid file type:", filePath)
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
                            width: 150
                            Layout.fillHeight: true
                            Rectangle { width: 100; height: 50; color: "transparent" }
                            Repeater {
                                model: countOfTracks
                                Rectangle {
                                    width: 150
                                    height: 50
                                    color: "#2D2D2D"
                                    border.color: "#444"
                                    Label {
                                        anchors.centerIn: parent
                                        text: "Track " + (index + 1)
                                        color: "#CCC"
                                        font.pixelSize: 12
                                    }
                                    ToolButton {
                                        text: "🎹"
                                        implicitWidth: 30
                                        implicitHeight: 30
                                        onClicked: {
                                            viewModel.openPluginEditor(index, 0) // Открываем первый плагин на дорожке
                                        }
                                    }
                                }
                            }
                        }

                        // Прокручиваемая область
                        Flickable {
                            id: flickableArea
                            property int countOfBeats: 100
                            property real baseBeatWidth: 40
                            property real zoomLevel: 1.0
                            property real beatWidth: baseBeatWidth * zoomLevel
                            property real widthOfAllArea: countOfBeats * beatWidth

                            Component.onCompleted: {
                                console.log("Initial widthOfAllArea:", widthOfAllArea)
                            }

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            contentWidth: widthOfAllArea
                            contentHeight: timeRuler.height + contentGrid.height
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.HorizontalFlick

                            // Обработка колеса мыши для зума с фокусировкой на курсоре
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton
                                onWheel: (wheel) => {
                                    // Позиция курсора
                                    var cursorX = wheel.x
                                    var contentCursorX = cursorX + flickableArea.contentX
                                    var cursorBeat = contentCursorX / flickableArea.beatWidth

                                    // Изменяем zoomLevel
                                    var delta = wheel.angleDelta.y / 120
                                    var newZoom = Math.max(0.5, Math.min(2.0, flickableArea.zoomLevel + delta * 0.1))
                                    flickableArea.zoomLevel = newZoom
                                    zoomSlider.value = newZoom

                                    // Корректируем contentX для фокусировки на курсоре
                                    var newContentCursorX = cursorBeat * flickableArea.beatWidth
                                    flickableArea.contentX = newContentCursorX - cursorX
                                    flickableArea.contentX = Math.max(0, Math.min(flickableArea.contentX, flickableArea.contentWidth - flickableArea.width))

                                    console.log("Zoom (wheel) changed to:", flickableArea.zoomLevel, 
                                               "contentX:", flickableArea.contentX, 
                                               "greenline.x:", greenline.x, 
                                               "playheadPosition:", viewModel.playheadPosition, 
                                               "isPlaying:", viewModel.isPlaying)
                                }
                            }

                            // Временная линейка
                            Rectangle {
                                id: timeRuler
                                width: flickableArea.widthOfAllArea
                                height: 50
                                color: "#1E1E1E"
                                z: 2

                                Row {
                                    anchors.fill: parent
                                    spacing: 0
                                    Repeater {
                                        model: flickableArea.countOfBeats
                                        Rectangle {
                                            width: flickableArea.beatWidth
                                            height: parent.height
                                            color: "transparent"
                                            border.color: "#444"
                                            Label {
                                                anchors.centerIn: parent
                                                //  text: index % 4 === 0 ? Math.floor(index/4) + 1 : ""
                                                text: index 
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
                                width: flickableArea.widthOfAllArea
                                height: 15 * 50 // Высота не зависит от зума

                                Item {
                                    id: tracksAndClipsContainer
                                    anchors.fill: parent

                                    // Фоновые прямоугольники треков
                                    Repeater {
                                        model: viewModel.trackModel
                                        delegate: Rectangle {
                                            property int trackIndex: model.trackIndex || 0
                                            width: contentGrid.width
                                            height: 50
                                            y: trackIndex * 50
                                            z: 0
                                            color: "#2D2D2D"
                                            border { width: 1; color: "#444" }
                                        }
                                    }

                                    // Вертикальные полосы
                                    Row {
                                        anchors.fill: parent
                                        spacing: 0
                                        z: 1
                                        Repeater {
                                            model: flickableArea.countOfBeats
                                            Rectangle {
                                                width: flickableArea.beatWidth
                                                height: parent.height
                                                color: "transparent"
                                                border.width: 1
                                                border.color: "#444"
                                            }
                                        }
                                    }

                                    // Клипы
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
                                            }

                                            Repeater {
                                                model: trackData.clips || []
                                                delegate: Rectangle {
                                                    id: clipRectangle
                                                    property var clipModel: modelData
                                                    x: (clipModel.startBeats || 0) * flickableArea.beatWidth
                                                    width: (clipModel.durationBeats || 1) * flickableArea.beatWidth
                                                    height: 48
                                                    color: clipModel.type === "audio" ? "#FF5722" : "#4CAF50"
                                                    radius: 3
                                                    border.width: 1
                                                    border.color: Qt.darker(color, 1.2)

                                                    Component.onCompleted: {
                                                        console.log("Clip created at x:" + clipModel.startBeats)
                                                    }

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
                                                        drag.maximumX: Math.max(0, contentGrid.width - clipRectangle.width)

                                                        onPressed: {
                                                            console.log("Drag started at:", clipRectangle.x)
                                                            clipRectangle.z = 3
                                                        }

                                                        onReleased: {
                                                            var snappedX = Math.round(clipRectangle.x / flickableArea.beatWidth) * flickableArea.beatWidth
                                                            clipRectangle.x = snappedX
                                                            clipRectangle.z = 2
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
                            Rectangle {
                                id: greenline
                                width: 2
                                height: contentGrid.height
                                color: "green"
                                z: 10
                                x: viewModel.playheadPosition * flickableArea.beatWidth
                                anchors.top: contentGrid.top

                                // Шлейф (основной слой)
                                Rectangle {
                                    id: trail
                                    width: 10
                                    height: parent.height
                                    color: Qt.rgba(0, 1, 0, 0.5)
                                    anchors.right: parent.left
                                    visible: viewModel.isPlaying
                                    opacity: 0

                                    SequentialAnimation {
                                        running: viewModel.isPlaying
                                        loops: Animation.Infinite
                                        NumberAnimation {
                                            target: trail
                                            property: "opacity"
                                            from: 0.5
                                            to: 0
                                            duration: 600
                                        }
                                        PauseAnimation { duration: 200 }
                                    }
                                }

                                // Вторичный шлейф
                                Rectangle {
                                    id: trailSecondary
                                    width: 20
                                    height: parent.height
                                    color: Qt.rgba(0, 1, 0, 0.3)
                                    anchors.right: parent.left
                                    visible: viewModel.isPlaying
                                    opacity: 0

                                    SequentialAnimation {
                                        running: viewModel.isPlaying
                                        loops: Animation.Infinite
                                        NumberAnimation {
                                            target: trailSecondary
                                            property: "opacity"
                                            from: 0.3
                                            to: 0
                                            duration: 800
                                        }
                                        PauseAnimation { duration: 300 }
                                    }
                                }

                                MouseArea {
                                    id: greenlineMouseArea
                                    anchors.fill: parent
                                    anchors.leftMargin: -14
                                    anchors.rightMargin: -14
                                    width: 30
                                    drag.target: greenline
                                    drag.axis: Drag.XAxis
                                    drag.minimumX: 0
                                    drag.maximumX: Math.max(0, contentGrid.width - greenline.width)

                                    onPressed: {
                                        console.log("Greenline drag started at:", greenline.x)
                                    }

                                    onReleased: {
                                        var snappedX = Math.round(greenline.x / flickableArea.beatWidth) * flickableArea.beatWidth
                                        greenline.x = snappedX
                                        var newPosition = snappedX / flickableArea.beatWidth

                                    //    var newPosition = greenline.x
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
                                           // console.log("Greenline updated to position:", position, "x:", greenline.x)
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