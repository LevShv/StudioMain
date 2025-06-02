import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import "qrc:/FileBrowser"
import Qt.labs.folderlistmodel
import QtQuick.Dialogs
import QtQuick.Shapes 1.15
import "qrc:/"



Window {
    id: mainWindow
    visible: true
    width: 1500
    height: 1080
    title: "StudioMain"
    color: "#2E3440"

    // свойтсва piano rol
    property int selectedTrackIndex: -1
    property int selectedClipIndex: -1
    property bool pianoRollVisible: false
    //

    property Item dragParent: contentItem
    property int countOfTracks: viewModel.trackModel.countOfTracks

    property bool multiSelectMode: false // Режим множественного выделения (удерживать Ctrl)
    property var selectedClips: []
    signal clearSelectedClipsRequested()

    Connections {
        target: mainWindow
        function onClearSelectedClipsRequested() {
            console.log("clearSelectedClipsRequested received, previous selectedClips count=" + mainWindow.selectedClips.length)
            mainWindow.selectedClips = []
            console.log("selectedClips cleared, new count=" + mainWindow.selectedClips.length)
        }
    }

    function clearSelectedClips() {
        console.log("clearSelectedClips called, previous selectedClips count=" + mainWindow.selectedClips.length)
        mainWindow.selectedClips = []
        console.log("selectedClips cleared, new count=" + mainWindow.selectedClips.length)
    }

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
                           ToolButton {
                                text: "Показать/Скрыть Piano Roll"
                                onClicked: {
                                    pianoRollVisible = !pianoRollVisible
                                    console.log("PianoRoll visible:", pianoRollVisible)
                                }
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

                SplitView {
                    orientation: Qt.Vertical
                    
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
                                        height: 52
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
                                property int countOfBeats: 10000
                                property real baseBeatWidth: 40
                                property real zoomLevel: 1.0
                                property real beatWidth: baseBeatWidth * zoomLevel
                                property real widthOfAllArea: countOfBeats * beatWidth
                                property bool needsUpdate: false
                                property real lastContentX: 0
                                property real cachedGroupSize: getGroupSize()

                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                contentWidth: widthOfAllArea
                                contentHeight: timeRuler.height + contentGrid.height
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds
                                flickableDirection: Flickable.HorizontalFlick
                                maximumFlickVelocity: 2000
                                flickDeceleration: 1000

                                Rectangle {
                                    id: selectionRect
                                    color: Qt.rgba(0.5, 0.5, 1, 0.3)
                                    border.color: "blue"
                                    border.width: 1
                                    visible: false
                                    z: 5
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
                                    console.log("Zoom level changed to:", zoomLevel, "contentX:", contentX, "groupSize:", cachedGroupSize)
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
                                    console.log("Updated visible beats: startIndex:", startIndex, "endIndex:", endIndex, "groupSize:", groupSize)
                                }

                                function getGroupSize() {
                                    if (zoomLevel >= 8.0) return 0.0625
                                    if (zoomLevel >= 4.0) return 0.25
                                    if (zoomLevel >= 2.0) return 1
                                    if (zoomLevel >= 0.5) return 4
                                    return 16
                                }

                                ListModel {
                                    id: visibleBeatsModel
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.NoButton
                                    hoverEnabled: true

                                    onWheel: (wheel) => {
                                        var cursorX = Math.max(0, Math.min(wheel.x - flickableArea.contentX, flickableArea.width))
                                        if (isNaN(cursorX)) {
                                            cursorX = flickableArea.width / 2
                                        }
                                        var contentCursorX = cursorX + flickableArea.contentX
                                        var oldBeatWidth = flickableArea.baseBeatWidth * flickableArea.zoomLevel
                                        var currentBeat = oldBeatWidth > 0 ? contentCursorX / oldBeatWidth : 0
                                        var delta = wheel.angleDelta.y / 120
                                        var newZoom = Math.max(0.2, Math.min(10.0, flickableArea.zoomLevel + delta * 0.1))
                                        flickableArea.zoomLevel = newZoom
                                        var newBeatWidth = flickableArea.baseBeatWidth * flickableArea.zoomLevel
                                        flickableArea.contentWidth = flickableArea.countOfBeats * newBeatWidth
                                        flickableArea.contentX = currentBeat * newBeatWidth - cursorX
                                        flickableArea.contentX = Math.max(0, Math.min(flickableArea.contentX, flickableArea.contentWidth - flickableArea.width))
                                        console.log("Wheel zoom: cursorX:", cursorX, "contentCursorX:", contentCursorX, "currentBeat:", currentBeat, "newZoom:", newZoom, "newContentX:", flickableArea.contentX)
                                    }
                                }

                                // Ruler
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
                                                return itemX > -width * 4 && itemX < flickableArea.width + width * 4
                                            }

                                            Rectangle {
                                                anchors.fill: parent
                                                color: {
                                                    var beatIndex = index * flickableArea.cachedGroupSize
                                                    var measure = Math.floor(beatIndex / 4) + 1
                                                    return measure % 2 === 0 ? "#444444" : "#333333"
                                                }
                                                z: 0
                                            }

                                            Rectangle {
                                                height: parent.height
                                                x: 0
                                                color: "#000000"
                                                z: 1
                                                antialiasing: true
                                                width: {
                                                    var beatIndex = index * flickableArea.cachedGroupSize
                                                    return beatIndex % 4 < 0.001 ? 1 : 0.5
                                                }
                                            }

                                            Label {
                                                anchors.left: parent.left
                                                anchors.leftMargin: 2
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: {
                                                    var groupSize = flickableArea.cachedGroupSize
                                                    var beatIndex = index * groupSize
                                                    var measure = Math.floor(beatIndex / 4) + 1
                                                    if (groupSize >= 4) {
                                                        return measure
                                                    }
                                                    var beatInMeasure = Math.floor(beatIndex % 4) + 1
                                                    if (groupSize === 1) {
                                                        return measure + "." + beatInMeasure
                                                    }
                                                    var subBeat = Math.round((beatIndex % 1) / groupSize) + 1
                                                    return measure + "." + beatInMeasure + "." + subBeat
                                                }
                                                color: "#AAAAAA"
                                                font.pixelSize: flickableArea.beatWidth * flickableArea.cachedGroupSize > 20 ? 12 : 8
                                                visible: {
                                                    var groupSize = flickableArea.cachedGroupSize
                                                    var beatIndex = index * groupSize
                                                    return flickableArea.beatWidth * flickableArea.cachedGroupSize > 10 &&
                                                        (beatIndex % 1 < 0.001 || groupSize >= 1)
                                                }
                                                z: 2
                                                background: Rectangle {
                                                    color: "#333333"
                                                    opacity: 0.7
                                                    anchors.fill: parent
                                                    anchors.leftMargin: -2
                                                    anchors.rightMargin: -2
                                                }
                                            }
                                        }
                                    }
                                }

                                // Grid lines

                                // Tracks and clips
                                Item {
                                    id: contentGrid
                                    anchors.top: timeRuler.bottom
                                    width: flickableArea.widthOfAllArea
                                    height: viewModel.trackModel.countOfTracks * 52
                                    z: 2
                                    clip: true

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.LeftButton
                                    onClicked: (mouse) => {
                                        // Map mouse coordinates to contentGrid
                                        var localPos = mapToItem(contentGrid, mouse.x, mouse.y)
                                        var trackIndex = Math.floor(localPos.y / 52)
                                        var groupSize = flickableArea.cachedGroupSize
                                        var snapStep = groupSize * flickableArea.beatWidth // Grid size in pixels
                                        // Calculate position relative to contentGrid's origin
                                        var absoluteX = localPos.x // No need to add contentX here
                                        var position = absoluteX / flickableArea.beatWidth // Convert to beats
                                        // Find nearest grid point
                                        var nearestGridBeat = Math.round(position / groupSize) * groupSize
                                        var nearestGridX = nearestGridBeat * flickableArea.beatWidth // Convert back to pixels
                                        var distanceToGrid = Math.abs(absoluteX - nearestGridX)
                                        // Snap to grid only if distance is strictly less than 8 pixels
                                        if (distanceToGrid < 8) {
                                            position = nearestGridBeat
                                        }
                                        // Round position to 3 decimal places to avoid floating-point issues
                                        position = Math.round(position * 1000) / 1000
                                        if (trackIndex >= 0 && trackIndex < countOfTracks && position >= 0) {
                                            viewModel.addMidiClip(trackIndex, position)
                                            console.log("MIDI clip added: trackIndex:", trackIndex, "position:", position, 
                                                        "absoluteX:", absoluteX, "localPos.x:", localPos.x, 
                                                        "beatWidth:", flickableArea.beatWidth, "zoomLevel:", flickableArea.zoomLevel, 
                                                        "distanceToGrid:", distanceToGrid, "nearestGridBeat:", nearestGridBeat, 
                                                        "contentX:", flickableArea.contentX)
                                        } else {
                                            console.log("Invalid MIDI clip placement: trackIndex:", trackIndex, "position:", position)
                                        }
                                        mouse.accepted = true
                                    }
                                }
                                    Item {
                                        id: tracksAndClipsContainer
                                        anchors.fill: parent

                                        Repeater {
                                            id: trackLinesRepeater
                                            model: visibleBeatsModel
                                            Rectangle {
                                                width: flickableArea.cachedGroupSize * flickableArea.beatWidth
                                                height: contentGrid.height
                                                x: index * width
                                                color: {
                                                    var beatIndex = index * flickableArea.cachedGroupSize
                                                    var measure = Math.floor(beatIndex / 4) + 1
                                                    return measure % 2 === 0 ? "#444444" : "#333333"
                                                }
                                                z: 0
                                            }
                                        }

                                        Repeater {    
                                            model: viewModel.trackModel
                                            delegate: Item {
                                                property int trackIndex: model.trackIndex || 0
                                                width: flickableArea.width
                                                height: 50
                                                x: flickableArea.contentX
                                                y: Math.floor(index * 52)
                                                z: 2

                                                function resetTrackSelections() {
                                                    for (var i = 0; i < clipsRepeater.count; i++) {
                                                        var clip = clipsRepeater.itemAt(i)
                                                        if (clip) {
                                                            clip.isSelected = false
                                                        }
                                                    }
                                                }

                                                Rectangle {
                                                    width: parent.width
                                                    height: 2
                                                    color: "#000000"
                                                    anchors.top: parent.top
                                                    antialiasing: true
                                                    z: 2
                                                    layer.enabled: true
                                                    Component.onCompleted: console.log("Horizontal line drawn at track:", index, "y:", parent.y, "x:", parent.x, "width:", width)
                                                }
                                                Rectangle {
                                                    width: parent.width
                                                    height: 2
                                                    color: "#000000"
                                                    anchors.bottom: parent.bottom
                                                    antialiasing: true
                                                    z: 2
                                                    layer.enabled: true
                                                    visible: index === viewModel.trackModel.countOfTracks - 1
                                                    Component.onCompleted: console.log("Bottom horizontal line drawn at track:", index, "y:", parent.y + height)
                                                }
                                            }
                                        }

                                        Repeater {
                                            model: visibleBeatsModel
                                            Rectangle {
                                                height: contentGrid.height
                                                x: index * flickableArea.cachedGroupSize * flickableArea.beatWidth
                                                color: "#000000"
                                                antialiasing: true
                                                z: 1
                                                width: {
                                                    var beatIndex = index * flickableArea.cachedGroupSize
                                                    return beatIndex % 4 < 0.001 ? 1 : 0.5
                                                }
                                                visible: {
                                                    var itemX = x - flickableArea.contentX
                                                    return itemX > -width * 2 && itemX < flickableArea.width + width * 2
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
                                                property var mainWindowRef: mainWindow // Явная привязка
                                                property var lastCopiedSourceIndex: -1  // Индекс исходного клипа
                                                property real lastCopiedPosition: -1

                                                width: contentGrid.width
                                                height: 50
                                                y: Math.floor(index * 52)
                                                z: 3

                                                function resetTrackSelections() {
                                                    for (var i = 0; i < clipsRepeater.count; i++) {
                                                        var clip = clipsRepeater.itemAt(i)
                                                        if (clip) {
                                                            clip.isSelected = false
                                                        }
                                                    }
                                                }

                                                function findFreePosition(startBeat, duration, sourceIndex) {
                                                var clips = []
                                                for (var i = 0; i < clipsModel.count; i++) {
                                                    var clipData = clipsModel.get(i)
                                                    clips.push({
                                                        start: clipData.startBeats,
                                                        end: clipData.startBeats + clipData.durationBeats
                                                    })
                                                }
                                                
                                                clips.sort((a, b) => a.start - b.start)
                                                
                                                // Если копируем тот же клип - продолжаем с последней позиции
                                                var searchPosition = (lastCopiedSourceIndex === sourceIndex && lastCopiedPosition >= startBeat) 
                                                                ? lastCopiedPosition : startBeat
                                                
                                                while (true) {
                                                    var positionFree = true
                                                    for (var j = 0; j < clips.length; j++) {
                                                        var clip = clips[j]
                                                        if (searchPosition < clip.end && (searchPosition + duration) > clip.start) {
                                                            positionFree = false
                                                            searchPosition = clip.end
                                                            break
                                                        }
                                                    }
                                                    
                                                    if (positionFree) {
                                                        lastCopiedSourceIndex = sourceIndex
                                                        lastCopiedPosition = searchPosition + duration
                                                        return searchPosition
                                                    }
                                                    
                                                    if (searchPosition > 10000) return startBeat + duration
                                                }
                                            }

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

                                                        // Свойство для выделения
                                                        property bool isSelected: false
                                                        property int sourceIndex: index  
                                                        property var mainWindowRef: trackItem.mainWindowRef
                                                        property bool resizingLeft: false
                                                        property bool resizingRight: false
                                                        property real originalX: 0
                                                        property real originalWidth: 0
                                                        property real originalStartBeats: 0
                                                        property real originalDurationBeats: 0
                                                    

                                                        onIsSelectedChanged: {
                                                            if (isSelected) {
                                                                let exists = mainWindow.selectedClips.some(
                                                                    clip => clip.trackIndex === trackIndex && clip.clipIndex === index
                                                                )
                                                                if (!exists) {
                                                                    mainWindow.selectedClips.push({
                                                                        trackIndex: trackIndex,
                                                                        clipIndex: index
                                                                    })
                                                                    console.log(`Clip selected: trackIndex=${trackIndex}, clipIndex=${index}, selectedClips count=${mainWindow.selectedClips.length}`)
                                                                }
                                                            } else {
                                                                mainWindow.selectedClips = mainWindow.selectedClips.filter(
                                                                    clip => clip.trackIndex !== trackIndex || clip.clipIndex !== index
                                                                )
                                                                console.log(`Clip deselected: trackIndex=${trackIndex}, clipIndex=${index}, selectedClips count=${mainWindow.selectedClips.length}`)
                                                            }
                                                        }

                                                        Connections {
                                                            target: flickableArea
                                                            function onBeatWidthChanged() {
                                                                clipItem.width = model.durationBeats * flickableArea.beatWidth
                                                                clipItem.x = model.startBeats * flickableArea.beatWidth
                                                                console.log("Updated clip position and width due to zoom: trackIndex=", trackIndex, "clipIndex=", index, "width=", clipItem.width)
                                                            }
                                                        }

                                                        Rectangle {
                                                            id: clipRectangle
                                                            anchors.fill: parent
                                                            color: model.type === "audio" ? "#FF5722" : "#4CAF50"
                                                            radius: 3
                                                            border.width: isSelected ? 3 : 1 // Жёлтая рамка при выделении
                                                            border.color: isSelected ? "#FFFF00" : Qt.darker(color, 1.2)
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


                                                            MouseArea {
                                                                id: leftResizeArea
                                                                width: Math.min(15, clipItem.width / 4)
                                                                height: parent.height
                                                                anchors.left: parent.left
                                                                cursorShape: Qt.SizeHorCursor
                                                                enabled: isSelected
                                                                hoverEnabled: true
                                                                preventStealing: true
                                                                z: 5

                                                                onPressed: (mouse) => {
                                                                    if (mouse.modifiers & Qt.ControlModifier) {
                                                                        clipItem.resizingLeft = true
                                                                        clipItem.originalX = clipItem.x
                                                                        clipItem.originalWidth = clipItem.width
                                                                        clipItem.originalStartBeats = model.startBeats
                                                                        clipItem.originalDurationBeats = model.durationBeats
                                                                        // Сбрасываем волноформу перед началом изменения
                                                                        if (model.type === "audio") {
                                                                            waveformImage.source = ""
                                                                            console.log("Waveform cleared before left resize: trackIndex=", trackIndex, "clipIndex=", index)
                                                                        }
                                                                        console.log("Started resizing left edge: trackIndex=", trackIndex, "clipIndex=", index, "zoneWidth=", leftResizeArea.width)
                                                                        mouse.accepted = true
                                                                    } else {
                                                                        mouse.accepted = false
                                                                    }
                                                                }

                                                                onPositionChanged: (mouse) => {
                                                                    if (clipItem.resizingLeft) {
                                                                        var newX = clipItem.originalX + mouse.x
                                                                        var newStartBeats = flickableArea.beatWidth > 0 ? newX / flickableArea.beatWidth : 0
                                                                        var newDurationBeats = clipItem.originalDurationBeats + (clipItem.originalStartBeats - newStartBeats)
                                                                        var newWidth = newDurationBeats * flickableArea.beatWidth

                                                                        if (newDurationBeats >= 0.25 && newX >= 0) {
                                                                            clipItem.x = newX
                                                                            clipItem.width = newWidth
                                                                            clipsModel.setData(index, "startBeats", newStartBeats)
                                                                            clipsModel.setData(index, "durationBeats", newDurationBeats)
                                                                            // Обновляем волноформу только для аудиоклипов
                                                                            if (model.type === "audio") {
                                                                                waveformImage.source = "" // Очищаем перед обновлением
                                                                                waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                                                                console.log("Waveform updated during left resize: width=", clipRectangle.width, "source=", waveformImage.source)
                                                                            }
                                                                            console.log("Resizing left: newStartBeats=", newStartBeats, "newDurationBeats=", newDurationBeats)
                                                                        }
                                                                    }
                                                                }

                                                                onReleased: {
                                                                    if (clipItem.resizingLeft) {
                                                                        var groupSize = flickableArea.cachedGroupSize
                                                                        var snapStepBeats = groupSize > 0 ? groupSize : 1
                                                                        var thresholdBeats = 0.1

                                                                        var newStartBeats = flickableArea.beatWidth > 0 ? clipItem.x / flickableArea.beatWidth : 0
                                                                        var newDurationBeats = flickableArea.beatWidth > 0 ? clipItem.width / flickableArea.beatWidth : 0

                                                                        var snappedStartBeats = Math.round(newStartBeats / snapStepBeats) * snapStepBeats
                                                                        var snappedDurationBeats = Math.round(newDurationBeats / snapStepBeats) * snapStepBeats

                                                                        if (Math.abs(newStartBeats - snappedStartBeats) <= thresholdBeats) {
                                                                            newStartBeats = snappedStartBeats
                                                                        }
                                                                        if (Math.abs(newDurationBeats - snappedDurationBeats) <= thresholdBeats) {
                                                                            newDurationBeats = snappedDurationBeats
                                                                        }

                                                                        if (newDurationBeats >= 0.25) {
                                                                            clipItem.x = newStartBeats * flickableArea.beatWidth
                                                                            clipItem.width = newDurationBeats * flickableArea.beatWidth
                                                                            viewModel.moveClip(trackIndex, index, newStartBeats)
                                                                            viewModel.changeClipDuration(trackIndex, index, newDurationBeats)
                                                                            // Очищаем и обновляем волноформу после изменения
                                                                            if (model.type === "audio") {
                                                                                waveformImage.source = "" // Очищаем перед финальным обновлением
                                                                                waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                                                                console.log("Waveform updated after left resize: width=", clipRectangle.width, "source=", waveformImage.source)
                                                                            }
                                                                            console.log("Resized left: trackIndex=", trackIndex, "clipIndex=", index, "newStartBeats=", newStartBeats, "newDurationBeats=", newDurationBeats)
                                                                        } else {
                                                                            console.log("Invalid duration, reverting: newDurationBeats=", newDurationBeats)
                                                                            clipItem.x = model.startBeats * flickableArea.beatWidth
                                                                            clipItem.width = model.durationBeats * flickableArea.beatWidth
                                                                            clipsModel.setData(index, "startBeats", model.startBeats)
                                                                            clipsModel.setData(index, "durationBeats", model.durationBeats)
                                                                            // Восстанавливаем волноформу при откате
                                                                            if (model.type === "audio") {
                                                                                waveformImage.source = "" // Очищаем перед восстановлением
                                                                                waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                                                                console.log("Waveform reverted: width=", clipRectangle.width, "source=", waveformImage.source)
                                                                            }
                                                                        }

                                                                        clipItem.resizingLeft = false
                                                                    }
                                                                }
                                                            }

                                                            // Правый край для изменения длительности (только с Ctrl)
                                                            MouseArea {
                                                                id: rightResizeArea
                                                                width: Math.min(15, clipItem.width / 4)
                                                                height: parent.height
                                                                anchors.right: parent.right
                                                                cursorShape: Qt.SizeHorCursor
                                                                enabled: isSelected
                                                                hoverEnabled: true
                                                                preventStealing: true
                                                                z: 5

                                                                onPressed: (mouse) => {
                                                                    if (mouse.modifiers & Qt.ControlModifier) {
                                                                        clipItem.resizingRight = true
                                                                        clipItem.originalWidth = clipItem.width
                                                                        clipItem.originalDurationBeats = model.durationBeats
                                                                        // Сбрасываем волноформу перед началом изменения
                                                                        if (model.type === "audio") {
                                                                            waveformImage.source = ""
                                                                            console.log("Waveform cleared before right resize: trackIndex=", trackIndex, "clipIndex=", index)
                                                                        }
                                                                        console.log("Started resizing right edge: trackIndex=", trackIndex, "clipIndex=", index, "zoneWidth=", rightResizeArea.width)
                                                                        mouse.accepted = true
                                                                    } else {
                                                                        mouse.accepted = false
                                                                    }
                                                                }

                                                                onPositionChanged: (mouse) => {
                                                                    if (clipItem.resizingRight) {
                                                                        var newWidth = clipItem.originalWidth + mouse.x
                                                                        var newDurationBeats = flickableArea.beatWidth > 0 ? newWidth / flickableArea.beatWidth : 0

                                                                        if (newDurationBeats >= 0.25) {
                                                                            clipItem.width = newWidth
                                                                            clipsModel.setData(index, "durationBeats", newDurationBeats)
                                                                            // Обновляем волноформу
                                                                            if (model.type === "audio") {
                                                                                waveformImage.source = "" // Очищаем перед обновлением
                                                                                waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                                                                console.log("Waveform updated during right resize: width=", clipRectangle.width, "source=", waveformImage.source)
                                                                            }
                                                                            console.log("Resizing right: newDurationBeats=", newDurationBeats)
                                                                        }
                                                                    }
                                                                }

                                                                onReleased: {
                                                                    if (clipItem.resizingRight) {
                                                                        var groupSize = flickableArea.cachedGroupSize
                                                                        var snapStepBeats = groupSize > 0 ? groupSize : 1
                                                                        var thresholdBeats = 0.1

                                                                        var newDurationBeats = flickableArea.beatWidth > 0 ? clipItem.width / flickableArea.beatWidth : 0
                                                                        var snappedDurationBeats = Math.round(newDurationBeats / snapStepBeats) * snapStepBeats

                                                                        if (Math.abs(newDurationBeats - snappedDurationBeats) <= thresholdBeats) {
                                                                            newDurationBeats = snappedDurationBeats
                                                                        }

                                                                        if (newDurationBeats >= 0.25) {
                                                                            clipItem.width = newDurationBeats * flickableArea.beatWidth
                                                                            viewModel.changeClipDuration(trackIndex, index, newDurationBeats)
                                                                            // Очищаем и обновляем волноформу после изменения
                                                                            if (model.type === "audio") {
                                                                                waveformImage.source = "" // Очищаем перед финальным обновлением
                                                                                waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                                                                console.log("Waveform updated after right resize: width=", clipRectangle.width, "source=", waveformImage.source)
                                                                            }
                                                                            console.log("Resized right: trackIndex=", trackIndex, "clipIndex=", index, "newDurationBeats=", newDurationBeats)
                                                                        } else {
                                                                            console.log("Invalid duration, reverting: newDurationBeats=", newDurationBeats)
                                                                            clipItem.width = model.durationBeats * flickableArea.beatWidth
                                                                            clipsModel.setData(index, "durationBeats", model.durationBeats)
                                                                            // Восстанавливаем волноформу при откате
                                                                            if (model.type === "audio") {
                                                                                waveformImage.source = "" // Очищаем перед восстановлением
                                                                                waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                                                                console.log("Waveform reverted: width=", clipRectangle.width, "source=", waveformImage.source)
                                                                            }
                                                                        }

                                                                        clipItem.resizingRight = false
                                                                    }
                                                                }
                                                            }
            
                                                        }

                                                        MouseArea {
                                                            anchors.fill: parent
                                                            acceptedButtons: Qt.LeftButton
                                                            drag.target: clipItem
                                                            drag.axis: Drag.XAxis
                                                            drag.minimumX: 0
                                                            drag.maximumX: Math.max(0, contentGrid.width - clipItem.width)

                                                            onClicked: (mouse) => {
                                                                console.log(`Mouse clicked on clip: trackIndex=${trackIndex}, clipIndex=${index}, ctrlPressed=${mouse.modifiers & Qt.ControlModifier}`)
                                                                if (!mainWindowRef) {
                                                                    console.log("Error: mainWindowRef is undefined in MouseArea")
                                                                    return
                                                                }

                                                                if (mouse.modifiers & Qt.ControlModifier) {
                                                                    // Множественное выделение с Control: переключаем текущий клип
                                                                    clipItem.isSelected = !clipItem.isSelected
                                                                    mainWindow.multiSelectMode = true
                                                                } else {
                                                                    // Одиночное выделение: сбрасываем все и выбираем текущий
                                                                    resetAllClipSelections()
                                                                    clipItem.isSelected = true
                                                                    mainWindow.multiSelectMode = false
                                                                }
                                                                mouse.accepted = true
                                                            }

                                                            onPositionChanged: (mouse) => {
                                                                if (drag.active) {
                                                                    var newX = clipItem.x
                                                                    var newPosition = flickableArea.beatWidth > 0 ? newX / flickableArea.beatWidth : 0
                                                                    clipsModel.setData(index, "startBeats", newPosition)
                                                                    console.log("Dragging: newPosition=", newPosition)
                                                                }
                                                            }

                                                            onReleased: {
                                                                var groupSize = flickableArea.cachedGroupSize
                                                                var snapStep = groupSize * flickableArea.beatWidth
                                                                var threshold = 8 // Порог для привязки к сетке

                                                                // Вычисляем смещение для главного клипа (того, который перетаскивается)
                                                                var deltaX = clipItem.x - (model.startBeats * flickableArea.beatWidth)
                                                                var nearestGridX = Math.round(clipItem.x / snapStep) * snapStep
                                                                var snappedX = Math.abs(clipItem.x - nearestGridX) <= threshold ? nearestGridX : clipItem.x
                                                                var newPosition = flickableArea.beatWidth > 0 ? snappedX / flickableArea.beatWidth : 0
                                                                var snapOffset = snappedX - clipItem.x // Смещение из-за привязки к сетке (если есть)

                                                                // Обновляем главный клип
                                                                viewModel.moveClip(trackIndex, index, newPosition)

                                                                // Для всех выделенных клипов этого трека
                                                                for (var i = 0; i < clipsRepeater.count; i++) {
                                                                    var otherClip = clipsRepeater.itemAt(i)
                                                                    if (otherClip && otherClip.isSelected && otherClip !== clipItem) {
                                                                        // Вычисляем новое положение без привязки к сетке, но с учетом snapOffset главного клипа
                                                                        var newX = otherClip.x + deltaX + snapOffset
                                                                        var newOtherPosition = flickableArea.beatWidth > 0 ? newX / flickableArea.beatWidth : 0
                                                                        viewModel.moveClip(trackIndex, otherClip.sourceIndex, newOtherPosition)
                                                                    }
                                                                }

                                                                clipRectangle.z = 4
                                                            }

                                                            function resetAllClipSelections() {
                                                                for (var t = 0; t < tracksRepeater.count; t++) {
                                                                    var track = tracksRepeater.itemAt(t)
                                                                    if (track && track.resetTrackSelections) {
                                                                        track.resetTrackSelections()
                                                                    }
                                                                }
                                                                mainWindow.selectedClips = []
                                                                console.log("All clip selections cleared, selectedClips count=0")
                                                            }
                                                        }

                                                        function deleteSelectedClips() {
                                                                console.log("Starting deleteSelectedClips, selectedClips count=" + mainWindow.selectedClips.length)
                                                                
                                                                if (!mainWindow.selectedClips || mainWindow.selectedClips.length === 0) {
                                                                    console.log("No clips selected for deletion")
                                                                    mainWindow.selectedClips = []
                                                                    return
                                                                }

                                                                console.log("Preparing to delete " + mainWindow.selectedClips.length + " clips")
                                                                try {
                                                                    viewModel.deleteClips(mainWindow.selectedClips)
                                                                    console.log("Successfully deleted " + mainWindow.selectedClips.length + " clips")
                                                                } catch (error) {
                                                                    console.log("Failed to delete clips, error=" + error)
                                                                    mainWindowRef.clearSelectedClipsRequested()
                                                                }

                                                                mainWindowRef.clearSelectedClipsRequested()
                                                                console.log("Finished deleting clips, selectedClips cleared")
                                                            }                      

                                                        // Обработка клавиш C и D
                                                        Keys.onPressed: (event) => {
                                                            console.log(`Key pressed: key=${event.key}, text=${event.text}, modifiers=${event.modifiers}, clipSelected=${isSelected}, multiSelectMode=${mainWindow.multiSelectMode}`)
                                                            if (isSelected || mainWindow.multiSelectMode) {
                                                                if (event.key === Qt.Key_C && !mainWindow.multiSelectMode) {
                                                                    console.log("Copy key (C) pressed")
                                                                    let duration = model.durationBeats
                                                                    let newPosition = trackItem.findFreePosition(
                                                                        model.startBeats + duration, 
                                                                        duration,
                                                                        sourceIndex
                                                                    )
                                                                    viewModel.addAudioClip(trackIndex, model.file, newPosition)
                                                                    console.log(`Clip copied: trackIndex=${trackIndex}, newPosition=${newPosition}`)
                                                                    event.accepted = true
                                                                } else if (event.key === Qt.Key_B && !mainWindow.multiSelectMode) {
                                                                    console.log("Clone key (B) pressed")
                                                                    let duration = model.durationBeats
                                                                    let newPosition = trackItem.findFreePosition(
                                                                        model.startBeats + duration, 
                                                                        duration,
                                                                        sourceIndex
                                                                    )
                                                                    viewModel.AddCloneClip(trackIndex, sourceIndex, newPosition)
                                                                    console.log(`Clip cloned: trackIndex=${sourceIndex}, newPosition=${newPosition}`)
                                                                    event.accepted = true
                                                                } else if (event.key === Qt.Key_D || event.key === Qt.Key_Delete) {
                                                                    console.log("Delete key (D or Delete) pressed")
                                                                    if (mainWindow.multiSelectMode) {
                                                                        deleteSelectedClips()
                                                                    } else if (isSelected) {
                                                                        console.log(`Deleting single clip: trackIndex=${trackIndex}, clipIndex=${index}`)
                                                                        try {
                                                                            viewModel.deleteClip(trackIndex, index)
                                                                            console.log(`Clip deleted: trackIndex=${trackIndex}, clipIndex=${index}`)
                                                                            clipItem.isSelected = false // Сбрасываем выделение
                                                                        } catch (error) {
                                                                        //  console.log(`Failed to delete single clip: trackIndex=${trackIndex}, clipIndex=${index}, error=${error}`)
                                                                        }
                                                                    }
                                                                    event.accepted = true
                                                                } else if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
                                                                    console.log("Select all key (Ctrl+A) pressed")
                                                                    mainWindow.selectedClips = []
                                                                    for (let t = 0; t < tracksRepeater.count; t++) {
                                                                        let track = tracksRepeater.itemAt(t)
                                                                        if (track && track.clipsRepeater) {
                                                                            for (let i = 0; i < track.clipsRepeater.count; i++) {
                                                                                let clip = track.clipsRepeater.itemAt(i)
                                                                                if (clip) {
                                                                                    clip.isSelected = true
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                    console.log(`All clips selected, selectedClips count=${mainWindow.selectedClips.length}`)
                                                                    event.accepted = true
                                                                } else if (event.key === Qt.Key_Escape) {
                                                                    console.log("Escape key pressed")
                                                                    for (let t = 0; t < tracksRepeater.count; t++) {
                                                                        let track = tracksRepeater.itemAt(t)
                                                                        if (track && track.resetTrackSelections) {
                                                                            track.resetTrackSelections()
                                                                        }
                                                                    }
                                                                    mainWindow.selectedClips = []
                                                                    console.log("All clip selections cleared, selectedClips count=0")
                                                                    event.accepted = true
                                                                } else {
                                                                    console.log(`Unhandled key: key=${event.key}, text=${event.text}`)
                                                                }
                                                            } else {
                                                                console.log("Key ignored: clip not selected and not in multi-select mode")
                                                            }
                                                        }

                                                        // Включаем фокус для обработки клавиш
                                                        focus: isSelected
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

                                Timer {
                                    id: initialUpdateTimer
                                    interval: 1
                                    running: true
                                    repeat: false
                                    onTriggered: {
                                        flickableArea.updateVisibleBeats()
                                        console.log("Forced initial update, width:", flickableArea.width)
                                    }
                                }

                                Component.onCompleted: {
                                    zoomLevel = 1.0
                                    beatWidth = baseBeatWidth * zoomLevel
                                    cachedGroupSize = getGroupSize()
                                    contentWidth = countOfBeats * beatWidth
                                    contentX = 0
                                    console.log("Flickable initialized: width:", width, "zoomLevel:", zoomLevel, "beatWidth:", beatWidth, "cachedGroupSize:", cachedGroupSize, "contentX:", contentX)
                                
                                }
                            }
                        }
                    }

                    PianoView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 300
                        visible: pianoRollVisible
                        trackIndex: selectedTrackIndex
                        clipIndex: selectedClipIndex
                        baseBeatWidth: flickableArea.baseBeatWidth
                        countOfBeats: flickableArea.countOfBeats
                        zoomFactor: flickableArea.zoomLevel
                        // Синхронизация прокрутки
                        Binding {
                            target: pianoRollFlickable
                            property: "contentX"
                            value: flickableArea.contentX
                        }
                        Binding {
                            target: flickableArea
                            property: "contentX"
                            value: pianoRollFlickable.contentX
                        }
                    }   
                }
            }
        }
    }
}