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
                            console.log("File dropped: filePath:", filePath, "localPos.x:", localPos.x, "localPos.y:", localPos.y)
                            if (localPos.x >= 0 && localPos.x <= contentGrid.width &&
                                localPos.y >= 0 && localPos.y <= contentGrid.height) {
                                var trackIndex = Math.floor(localPos.y / 50) // Adjusted calculation
                                var position = Math.floor(localPos.x / flickableArea.beatWidth)
                                console.log("Calculated trackIndex:", trackIndex, "position:", position, "countOfTracks:", countOfTracks)
                                if (trackIndex >= 0 && trackIndex < countOfTracks && position >= 0) {
                                    var fileExt = filePath.toLowerCase().split('.').pop()
                                    if (["mp3", "wav", "aiff", "flac"].indexOf(fileExt) !== -1) {
                                        viewModel.addAudioClip(trackIndex, filePath, position)
                                        console.log("Added audio clip: trackIndex:", trackIndex, "filePath:", filePath, "position:", position)
                                    } else if (["dll", "vst3"].indexOf(fileExt) !== -1) {
                                        viewModel.addPlugin(trackIndex, filePath)
                                        console.log("Added plugin: trackIndex:", trackIndex, "filePath:", filePath)
                                    } else {
                                        console.log("Invalid file type:", filePath)
                                    }
                                } else {
                                    console.log("Invalid drop: trackIndex:", trackIndex, "position:", position, "countOfTracks:", countOfTracks)
                                }
                            } else {
                                console.log("Drop outside contentGrid: localPos.x:", localPos.x, "localPos.y:", localPos.y, "contentGrid.width:", contentGrid.width, "contentGrid.height:", contentGrid.height)
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
                        // Прокручиваемая область
Flickable {
    id: flickableArea
    property int countOfBeats: 1000
    property real baseBeatWidth: 40
    property real zoomLevel: 1.0
    property real beatWidth: baseBeatWidth * zoomLevel
    property real widthOfAllArea: countOfBeats * beatWidth
    property bool needsUpdate: false
    property real lastContentX: 0
    property real cachedGroupSize: getGroupSize()
    property bool isZooming: false // Флаг для зума

    Layout.fillWidth: true
    Layout.fillHeight: true
    contentWidth: widthOfAllArea
    contentHeight: timeRuler.height + contentGrid.height
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.HorizontalFlick
    maximumFlickVelocity: 2000
    flickDeceleration: 1000

    // Сглаживание прокрутки, но не во время зума
    Behavior on contentX {
        enabled: !flickableArea.isZooming
        SmoothedAnimation { duration: 150; velocity: 1000 }
    }

    Timer {
        id: updateDebounceTimer
        interval: 50
        running: flickableArea.needsUpdate
        onTriggered: {
            flickableArea.updateVisibleBeats()
            flickableArea.needsUpdate = false
        }
    }

    onZoomLevelChanged: {
        beatWidth = baseBeatWidth * zoomLevel
        contentWidth = countOfBeats * beatWidth
        contentX = Math.max(0, Math.min(contentX, contentWidth - width))
        cachedGroupSize = getGroupSize()
        needsUpdate = true
        isZooming = false // Сбрасываем флаг после зума
        console.log("Zoom level changed to:", zoomLevel, "contentX:", contentX)
    }

    onContentXChanged: {
        if (Math.abs(contentX - lastContentX) > 100) {
            needsUpdate = true
            lastContentX = contentX
            console.log("ContentX changed, updating visible beats at:", contentX)
        }
    }

    function updateVisibleBeats() {
        var startBeat = Math.floor(contentX / beatWidth)
        var endBeat = Math.ceil((contentX + width) / beatWidth)
        var groupSize = cachedGroupSize
        var startIndex = Math.floor(startBeat / groupSize) - 2
        var endIndex = Math.ceil(endBeat / groupSize) + 2
        startIndex = Math.max(0, startIndex)
        endIndex = Math.min(Math.ceil(countOfBeats / groupSize), endIndex)
        visibleBeatsModel.clear()
        for (var i = startIndex; i < endIndex; i++) {
            visibleBeatsModel.append({"index": i})
        }
    }

    function getGroupSize() {
        if (beatWidth > 80) return 0.5
        if (beatWidth > 60) return 0.75
        if (beatWidth > 40) return 1
        if (beatWidth > 20) return 2
        if (beatWidth > 10) return 4
        return 8
    }

    ListModel {
        id: visibleBeatsModel
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true

        onWheel: (wheel) => {
            flickableArea.isZooming = true // Устанавливаем флаг зума
            // Вычисляем позицию курсора относительно видимой области
            var cursorX = wheel.x
            if (isNaN(cursorX) || cursorX < 0 || cursorX > flickableArea.width) {
                cursorX = flickableArea.width / 2
            }
            // Текущая позиция курсора в контенте
            var contentCursorX = cursorX + flickableArea.contentX
            // Текущий бит под курсором
            var currentBeat = contentCursorX / flickableArea.beatWidth
            // Новый уровень зума
            var delta = wheel.angleDelta.y / 120
            var newZoom = Math.max(0.2, Math.min(10.0, flickableArea.zoomLevel + delta * 0.1))
            // Устанавливаем новый зум
            flickableArea.zoomLevel = newZoom
            // Новый beatWidth
            var newBeatWidth = flickableArea.baseBeatWidth * flickableArea.zoomLevel
            // Пересчитываем contentX, чтобы курсор остался на месте
            flickableArea.contentX = currentBeat * newBeatWidth - cursorX
            flickableArea.contentX = Math.max(0, Math.min(flickableArea.contentX, flickableArea.contentWidth - flickableArea.width))
            console.log("Wheel zoom: cursorX:", cursorX, "contentCursorX:", contentCursorX, "currentBeat:", currentBeat, "newZoom:", newZoom, "newContentX:", flickableArea.contentX)
        }
    }

    // Ruler (без изменений)
    Rectangle {
        id: timeRuler
        width: flickableArea.widthOfAllArea
        height: 50
        color: "#333333"
        z: 3

        Repeater {
            model: visibleBeatsModel
            Item {
                width: flickableArea.cachedGroupSize * flickableArea.beatWidth
                height: 50
                x: index * width
                visible: {
                    var itemX = x - flickableArea.contentX
                    return itemX > -width * 2 && itemX < flickableArea.width + width * 2
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    x: 0
                    color: "#000000"
                    z: 1
                    antialiasing: true
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: width / 2
                    anchors.verticalCenter: parent.verticalCenter
                    text: {
                        var groupSize = flickableArea.cachedGroupSize
                        var beatIndex = index * groupSize + groupSize
                        if (flickableArea.beatWidth < flickableArea.baseBeatWidth) {
                            return beatIndex.toFixed(1)
                        }
                        var seconds = beatIndex * (60 / viewModel.bpm)
                        if (seconds >= 60) {
                            var minutes = Math.floor(seconds / 60)
                            seconds = Math.round(seconds % 60)
                            return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
                        }
                        return Math.round(beatIndex)
                    }
                    color: "#FFFFFF"
                    font.pixelSize: flickableArea.beatWidth > 10 ? 12 : 8
                    visible: flickableArea.beatWidth > 5
                    z: 2
                }
            }
        }
    }

    // Grid lines (без изменений)
    Rectangle {
        id: timelineGrid
        width: flickableArea.widthOfAllArea
        height: contentGrid.height
        anchors.top: timeRuler.bottom
        color: "#333333"
        z: 1
        clip: true

        Repeater {
            model: visibleBeatsModel
            Rectangle {
                width: flickableArea.cachedGroupSize * flickableArea.beatWidth
                height: timelineGrid.height
                x: index * width
                color: "transparent"
                Rectangle {
                    width: 1
                    height: parent.height
                    color: "#000000"
                    antialiasing: true
                }
                visible: {
                    var itemX = x - flickableArea.contentX
                    return itemX > -width * 2 && itemX < flickableArea.width + width * 2
                }
            }
        }
    }

    // Tracks and clips
    Item {
        id: contentGrid
        anchors.top: timeRuler.bottom
        width: flickableArea.widthOfAllArea
        height: viewModel.trackModel.countOfTracks * 52
        z: 2
        clip: true

        Item {
            id: tracksAndClipsContainer
            anchors.fill: parent

            // Статические горизонтальные линии с ограниченной шириной
            Repeater {
                model: viewModel.trackModel
                delegate: Item {
                    property int trackIndex: model.trackIndex || 0
                    width: flickableArea.width // Ограничено видимой областью
                    height: 50
                    x: flickableArea.contentX // Синхронизация с прокруткой
                    y: Math.floor(index * 52)
                    z: 2

                    Rectangle {
                        width: parent.width
                        height: 2
                        color: "#000000"
                        anchors.top: parent.top
                        antialiasing: true
                        z: 2
                        layer.enabled: true // Включено обратно
                        Component.onCompleted: console.log("Horizontal line drawn at track:", index, "y:", parent.y, "x:", parent.x, "width:", width)
                    }
                    Rectangle {
                        width: parent.width
                        height: 2
                        color: "#000000"
                        anchors.bottom: parent.bottom
                        antialiasing: true
                        z: 2
                        layer.enabled: true // Включено обратно
                        visible: index === viewModel.trackModel.countOfTracks - 1
                        Component.onCompleted: console.log("Bottom horizontal line drawn at track:", index, "y:", parent.y + height)
                    }
                }
            }

            Repeater {
                id: tracksRepeater
                model: viewModel.trackModel
                delegate: Item {
                    id: trackItem
                    property int trackIndex: model.trackIndex
                    property var clipsModel: model.clipsModel
                    width: contentGrid.width
                    height: 50
                    y: Math.floor(index * 52)
                    z: 3

                    Connections {
                        target: viewModel.trackModel
                        function onDataChanged(topLeft, bottomRight, roles) {
                            if (index >= topLeft.row && index <= bottomRight.row && roles.includes(viewModel.trackModel.TrackIndexRole)) {
                                trackIndex = model.trackIndex
                            }
                        }
                    }

                    Repeater {
                        id: clipsRepeater
                        model: clipsModel
                        delegate: Item {
                            id: clipItem
                            x: model.startBeats * flickableArea.beatWidth
                            width: model.durationBeats * flickableArea.beatWidth
                            height: 48
                            anchors.top: parent.top
                            anchors.topMargin: 1
                            visible: {
                                var clipX = x - flickableArea.contentX
                                return clipX > -width * 3 && clipX < flickableArea.width + width * 3
                            }

                            Rectangle {
                                id: clipRectangle
                                anchors.fill: parent
                                color: model.type === "audio" ? "#FF5722" : "#4CAF50"
                                radius: 3
                                border.width: 1
                                border.color: Qt.darker(color, 1.2)
                                z: 4

                                Image {
                                    id: waveformImage
                                    anchors.fill: parent
                                    source: ""
                                    asynchronous: true
                                    cache: false
                                    visible: model.type === "audio" && source != ""
                                }

                                Timer {
                                    id: imageUpdateTimer
                                    interval: 1000
                                    running: clipItem.visible && model.type === "audio" && waveformImage.source == "" && !flickableArea.moving
                                    onTriggered: {
                                        if (clipItem.visible) {
                                            waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                            console.log("Waveform updated via timer for clip:", index, "width:", clipRectangle.width, "source:", waveformImage.source)
                                        }
                                    }
                                }

                                Connections {
                                    target: clipItem
                                    function onWidthChanged() {
                                        if (clipItem.visible && model.type === "audio") {
                                            imageUpdateTimer.restart()
                                            waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                            console.log("Clip width changed, updated waveform:", clipRectangle.width, "source:", waveformImage.source)
                                        }
                                    }
                                }

                                Connections {
                                    target: flickableArea
                                    function onZoomLevelChanged() {
                                        if (clipItem.visible && model.type === "audio") {
                                            imageUpdateTimer.restart()
                                            waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                            console.log("Zoom level changed, updated waveform:", clipRectangle.width, "source:", waveformImage.source)
                                        }
                                    }
                                }

                                Connections {
                                    target: flickableArea
                                    function onContentXChanged() {
                                        if (clipItem.visible && model.type === "audio" && waveformImage.source == "" && !flickableArea.moving) {
                                            imageUpdateTimer.restart()
                                            waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                            console.log("ContentX changed, updated waveform:", clipRectangle.width, "source:", waveformImage.source)
                                        }
                                    }
                                }

                                ToolButton {
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 2
                                    z: 5
                                    text: "🗑"
                                    onClicked: viewModel.deleteClip(trackIndex, index)
                                }

                                Label {
                                    anchors.fill: parent
                                    text: model.file ? model.file.split("/").pop() : "MIDI Clip"
                                    color: "white"
                                    font.pixelSize: 10
                                    padding: 5
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                    opacity: model.type === "audio" ? 0.5 : 1.0
                                    visible: !waveformImage.visible
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                drag.target: clipItem
                                drag.axis: Drag.XAxis
                                drag.minimumX: 0
                                drag.maximumX: Math.max(0, contentGrid.width - clipItem.width)

                                onPressed: {
                                    clipRectangle.z = 6
                                }

                                onReleased: {
                                    var snappedX = Math.round(clipItem.x / flickableArea.beatWidth) * flickableArea.beatWidth
                                    var newPosition = flickableArea.beatWidth > 0 ? snappedX / flickableArea.beatWidth : 0
                                    clipRectangle.z = 4
                                    viewModel.moveClip(trackIndex, index, newPosition)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: greenline
        width: 2
        height: contentGrid.height
        color: "green"
        z: 7
        x: Math.max(0, Math.min(viewModel.playheadPosition * flickableArea.beatWidth, flickableArea.contentWidth - width))
        anchors.top: timeRuler.bottom

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

            onReleased: {
                var snappedX = Math.round(greenline.x / flickableArea.beatWidth) * flickableArea.beatWidth
                greenline.x = snappedX
                var newPosition = flickableArea.beatWidth > 0 ? snappedX / flickableArea.beatWidth : 0
                viewModel.setPlayheadPosition(newPosition)
            }

            Connections {
                target: viewModel
                function onPlayheadPositionChanged(position) {
                    if (!greenlineMouseArea.drag.active) {
                        greenline.x = position * flickableArea.beatWidth
                        greenline.x = Math.max(0, Math.min(greenline.x, flickableArea.contentWidth - greenline.width))
                    }
                }
            }

            Connections {
                target: flickableArea
                function onBeatWidthChanged() {
                    if (!viewModel.isPlaying && !greenlineMouseArea.drag.active) {
                        greenline.x = viewModel.playheadPosition * flickableArea.beatWidth
                        greenline.x = Math.max(0, Math.min(greenline.x, flickableArea.contentWidth - greenline.width))
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        flickableArea.updateVisibleBeats()
    }
}
                    }
                }
            }
        }
    }
}