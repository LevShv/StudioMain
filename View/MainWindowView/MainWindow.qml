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
                            ToolButton { 
                                text: "Открыть" 
                                implicitWidth: 100 
                                onClicked: {
                                    viewModel.OpenProject("C:\\Users\\llvvv\\source\\repos\\Studio\\Result\\Save.json")
                                }
                            }
                            ToolButton { 
                                text: "Сохранить"
                                implicitWidth: 100
                                onClicked: {
                                    viewModel.SaveProject("C:\\Users\\llvvv\\source\\repos\\Studio\\Result\\Save.json")
                                }
                            }

                            ToolButton {
                                text: "Add WAV Track"
                                implicitWidth: 120
                                onClicked: viewModel.addAudioTrack()
                            }
                            ToolButton {
                                text: "Add Sampler Track"
                                implicitWidth: 120
                                onClicked: viewModel.addSamplerTrack()
                            }
                            ToolButton {
                                text: "Add MIDI Track"
                                implicitWidth: 120
                                onClicked: viewModel.addMidiTrack()
                            }
                        }

                        Row {
                            Layout.alignment: Qt.AlignLeft
                            spacing: 5
                            ToolButton { text: "Создать"; implicitWidth: 100 }
                            ToolButton { text: "Копировать"; implicitWidth: 100 }
                            ToolButton {
                                text: "Сохранить"
                                implicitWidth: 100 
                                onClicked: {
                                    viewModel.RenderToWave("C:\\Users\\llvvv\\source\\repos\\Studio\\Result\\mix.wav")
                                    console.log("Render to WAV")
                                }
                            }
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
                                model: viewModel.trackModel
                                Rectangle {
                                    width: 150
                                    height: 50
                                    color: "#2D2D2D"
                                    border.color: "#444"
                                    Label {
                                        anchors.centerIn: parent
                                        text: (index + 1) + " (" + model.trackType + ")"
                                        color: "#CCC"
                                        font.pixelSize: 12
                                    }
                                    Row {
                                        ToolButton {
                                            text: "🎹"
                                            implicitWidth: 30
                                            implicitHeight: 30
                                            onClicked: {
                                                viewModel.openPluginEditor(index, 0) // Открываем первый плагин на дорожке
                                            }
                                        }

                                        ToolButton {
                                            text: "🗑"
                                            implicitWidth: 30
                                            implicitHeight: 30
                                            onClicked: {
                                                viewModel.deleteTrack(index) 
                                            }
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

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            contentWidth: widthOfAllArea
                            contentHeight: timeRuler.height + contentGrid.height
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.HorizontalFlick

                            onZoomLevelChanged: {
                               beatWidth = baseBeatWidth * zoomLevel
                                contentWidth = countOfBeats * beatWidth
                                contentX = Math.max(0, Math.min(contentX, contentWidth - width))
                                //console.log("Flickable: ZoomLevel changed: zoomLevel:", zoomLevel, "beatWidth:", beatWidth, "contentWidth:", contentWidth)
                            }


                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton
                                hoverEnabled: true

                                onWheel: (wheel) => {
                                    // Координата курсора
                                    var cursorX = Math.max(0, Math.min(wheel.x - flickableArea.contentX, flickableArea.width))
                                    if (isNaN(cursorX)) {
                                        cursorX = flickableArea.width / 2
                                        console.log("MouseArea: Invalid cursorX, fallback to center")
                                    }

                                    // Текущий бит под курсором
                                    var contentCursorX = cursorX + flickableArea.contentX
                                    var oldBeatWidth = flickableArea.baseBeatWidth * flickableArea.zoomLevel
                                    var currentBeat = oldBeatWidth > 0 ? contentCursorX / oldBeatWidth : 0

                                    // Изменяем zoomLevel
                                    var delta = wheel.angleDelta.y / 120
                                    var newZoom = Math.max(0.5, Math.min(2.0, flickableArea.zoomLevel + delta * 0.1))
                                    flickableArea.zoomLevel = newZoom

                                    // Пересчитываем beatWidth и contentWidth
                                    var newBeatWidth = flickableArea.baseBeatWidth * flickableArea.zoomLevel
                                    flickableArea.contentWidth = flickableArea.countOfBeats * newBeatWidth

                                    // Пересчитываем contentX
                                    var newContentCursorX = currentBeat * newBeatWidth
                                    flickableArea.contentX = newContentCursorX - cursorX
                                    flickableArea.contentX = Math.max(0, Math.min(flickableArea.contentX, flickableArea.contentWidth - flickableArea.width))

                                    // Проверяем текущий бит
                                    var currentContentX = flickableArea.contentX + cursorX
                                    var newCurrentBeat = newBeatWidth > 0 ? currentContentX / newBeatWidth : 0

                                    // Логируем
                                    console.log("Zoom (wheel): zoomLevel:", flickableArea.zoomLevel,
                                        "contentX:", flickableArea.contentX,
                                        "contentWidth:", flickableArea.contentWidth,
                                        "cursorX:", cursorX,
                                        "wheelX:", wheel.x,
                                        "contentCursorX:", contentCursorX,
                                        "currentBeat:", newCurrentBeat,
                                        "oldBeatWidth:", oldBeatWidth,
                                        "newBeatWidth:", newBeatWidth,
                                        "flickableWidth:", flickableArea.width,
                                        "playheadPosition:", viewModel.playheadPosition,
                                        "greenlineX:", greenline.x,
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
                                        model:  viewModel.trackModel
                                        delegate: Rectangle {
                                            property int trackIndex: model.trackIndex || 0
                                            width: contentGrid.width
                                            height: 50
                                            y: index * 50
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
                                            property var clipsModel: model.clipsModel
                                            width: contentGrid.width
                                            height: 50
                                            y: index * 50
                                            z: 2

                                            Component.onCompleted: {
                                                console.log("Track index:", trackIndex, "clipsModel:", clipsModel);
                                            }

                                            Repeater {
                                                id: clipsRepeater
                                                model: clipsModel
                                                delegate: Rectangle {
                                                    id: clipRectangle
                                                    x: model.startBeats * flickableArea.beatWidth
                                                    width: model.durationBeats * flickableArea.beatWidth
                                                    height: 48
                                                    color: model.type === "audio" ? "#FF5722" : "#4CAF50"
                                                    radius: 3
                                                    border.width: 1
                                                    border.color: Qt.darker(color, 1.2)

                                                    Component.onCompleted: {
                                                        console.log("Clip created at x:", model.startBeats, "type:", model.type, "file:", model.file);
                                                    }

                                                    ToolButton {
                                                        anchors.right: parent.right
                                                        anchors.top: parent.top
                                                        anchors.margins: 2
                                                        z: 10  // Гарантированно выше других элементов
                                                        text: "🗑"
                                                        onClicked: {
                                                            console.log("Deleting clip:", index, "from track:", trackIndex);
                                                            viewModel.deleteClip(trackIndex, index);
                                                        }
                                                    }

                                                    Label {
                                                        anchors.fill: parent
                                                        text: model.file ? model.file.split("/").pop() : "MIDI Clip"
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
                                                            var newPosition = snappedX / flickableArea.beatWidth
                                                            viewModel.moveClip(trackIndex, index, newPosition)
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
                                x: Math.max(0, Math.min(viewModel.playheadPosition * flickableArea.beatWidth, flickableArea.contentWidth - width))
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
                                    drag.maximumX: Math.max(0, flickableArea.contentWidth - greenline.width)

                                    onPressed: {
                                        console.log("Greenline drag started at:", greenline.x)
                                    }

                                    onReleased: {
                                        var snappedX = Math.round(greenline.x / flickableArea.beatWidth) * flickableArea.beatWidth
                                        greenline.x = snappedX
                                        var newPosition = flickableArea.beatWidth > 0 ? snappedX / flickableArea.beatWidth : 0
                                        viewModel.setPlayheadPosition(newPosition)
                                        console.log("Greenline dropped at:", snappedX, "position:", newPosition)
                                    }

                                    Connections {
                                        target: viewModel
                                        function onPlayheadPositionChanged(position) {
                                            if (!greenlineMouseArea.drag.active) {
                                                greenline.x = position * flickableArea.beatWidth
                                                greenline.x = Math.max(0, Math.min(greenline.x, flickableArea.contentWidth - greenline.width))
                                                console.log("Playhead updated: position:", position, "x:", greenline.x, "beat:", greenline.x / flickableArea.beatWidth, "isPlaying:", viewModel.isPlaying)
                                            }
                                        }
                                    }

                                    Connections {
                                        target: flickableArea
                                        function onBeatWidthChanged() {
                                            if (!viewModel.isPlaying && !greenlineMouseArea.drag.active) {
                                                greenline.x = viewModel.playheadPosition * flickableArea.beatWidth
                                                greenline.x = Math.max(0, Math.min(greenline.x, flickableArea.contentWidth - greenline.width))
                                                console.log("Greenline updated on beatWidth change: greenlineX:", greenline.x, "playheadPosition:", viewModel.playheadPosition, "beatWidth:", flickableArea.beatWidth, "isPlaying:", viewModel.isPlaying)
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
}