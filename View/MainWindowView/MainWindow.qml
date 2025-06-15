import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import Qt.labs.folderlistmodel
import QtQuick.Dialogs
import QtQuick.Shapes 1.15
import "qrc:/FileBrowser"
import "qrc:/Piano"
import "qrc:/Separator"
import "qrc:/View/Components"

Window {
    id: mainWindow
    visible: true
    width: 1500
    height: 1080
    minimumWidth: 800  // Минимальная ширина
    minimumHeight: 600 // Минимальная высота
    title: viewModel.currentProjectPath === "" ? "StudioMain" : "StudioMain: " + getFileName(viewModel.currentProjectPath)
    color: "#2E3440"
    function getFileName(path) {
        var parts = path.split(/[\\/]/); // Разделяем путь по слешам
        var fileName = parts[parts.length - 1]; // Берем последнюю часть (имя файла)
        return fileName.split('.')[0]; // Убираем расширение, возвращаем часть до первой точки
    }
    // свойтсва piano rol
    property int selectedTrackIndex: -1
    property int selectedClipIndex: -1
    property int displayedClipIndex: -1 // Новое свойство для label
    property bool pianoRollVisible: false
    //
    property string imagesPath: "file:///" + viewModel.applicationHomeFolder() + "/images/"

    property Item dragParent: contentItem
    property int countOfTracks: viewModel.trackModel.countOfTracks

    property bool multiSelectMode: false // Режим множественного выделения (удерживать Ctrl)
    property var selectedClips: []

    property bool separatorVisible: false
    signal clearSelectedClipsRequested()
    property bool isExiting: false
    property bool pianoRollAutoOpen: true
    

    // Save dialog moved from toolbar
    FileDialog {
        id: saveDialog
        title: "Сохранить проект"
        nameFilters: ["ltproj файлы (*.ltproj)"]
        fileMode: FileDialog.SaveFile
        currentFolder: "file:///" + viewModel.applicationHomeFolder() + "/Saves"
        defaultSuffix: "ltproj"
        onAccepted: {
            var filePath = saveDialog.selectedFile.toString()
            if (filePath.startsWith("file:///")) {
                filePath = filePath.substring(8)
            }
            console.log("Сохранение проекта в:", filePath)
            try {
                viewModel.SaveProject(filePath)
                viewModel.setCurrentProjectPath(filePath)
                console.log("Проект успешно сохранен, устанавливаем isExiting")
                isExiting = true
                viewModel.prepareForExit()
                // Даем время на завершение всех операций
                Qt.callLater(function() {
                    console.log("Вызываем Qt.quit()")
                    Qt.quit()
                })
            } catch (error) {
                console.error("Ошибка при сохранении проекта:", error)
                isExiting = false // Сбрасываем флаг, чтобы можно было повторить попытку
                exitDialog.open() // Открываем диалог для повторной попытки
            }
        }
        onRejected: {
            console.log("Диалог сохранения отменен")
            isExiting = false // Сбрасываем флаг, если пользователь отменил
        }
    }

    // Exit confirmation dialog
    Dialog {
        id: exitDialog
        title: "Сохранить проект перед выходом?"
        modal: true
        standardButtons: Dialog.NoButton
        anchors.centerIn: parent
        y: 350
        closePolicy: Popup.CloseOnEscape

        ColumnLayout {
            spacing: 10
            Label {
                text: "Вы хотите сохранить изменения в проекте перед выходом?"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                Button {
                    text: "Сохранить"
                    onClicked: {
                        exitDialog.accept()
                        viewModel.prepareForExit()
                        if (viewModel.currentProjectPath === "") {
                            saveDialog.open()
                            saveDialog.onAccepted.connect(function() {
                                Qt.quit()
                            })
                        } else {
                            viewModel.SaveProject(viewModel.currentProjectPath)
                            Qt.quit()
                        }
                    }
                }
                Button {
                    text: "Не сохранять"
                    onClicked: {
                        exitDialog.accept()
                        viewModel.prepareForExit()
                        Qt.quit()
                    }
                }
                Button {
                    text: "Отмена"
                    onClicked: {
                        exitDialog.reject()
                    }
                }
            }
        }
    }

    // Modified closing handler
    onClosing: (close) => {
        if (!exitDialog.visible) {
            close.accepted = false
            exitDialog.open()
        }
    }
    
    Shortcut {
        sequence: "F11"
        onActivated: {
            if (mainWindow.visibility === Window.FullScreen) {
                mainWindow.showNormal()
            } else {
                mainWindow.showFullScreen()
            }
        }
    }

    signal clipMoved(int trackIndex, int clipIndex, double newStartBeats)

    Component.onCompleted: {
        console.log("viewModel object:", viewModel)
        console.log("viewModel.pluginModel object:", viewModel.pluginModel)
        if (viewModel) {
            console.log("viewModel signals:", Object.keys(viewModel))
        }
    }

    Connections {
        target: mainWindow
        function onClearSelectedClipsRequested() {
            console.log("clearSelectedClipsRequested received, previous selectedClips count=" + mainWindow.selectedClips.length)
            mainWindow.selectedClips = []
            console.log("selectedClips cleared, new count=" + mainWindow.selectedClips.length)
        }
        function onClipMoved(trackIndex, clipIndex, newStartBeats) {
            console.log("MainWindow: Handling clipMoved signal: trackIndex=", trackIndex,
                        "clipIndex=", clipIndex, "newStartBeats=", newStartBeats)
            if (trackIndex === viewModel.midiModel.trackIndex && clipIndex === viewModel.midiModel.clipIndex) {
                viewModel.midiModel.setRedlineStartime()
            }
        }

    }

    Connections {
        target: viewModel
        onPluginAdded: function(trackIndex) {
            console.log("Plugin added to track:", trackIndex, "Current selectedTrackIndex:", selectedTrackIndex, "separatorVisible:", separatorVisible)
            if (!viewModel || !viewModel.pluginModel) {
                console.error("viewModel or pluginModel is null!")
                return;
            }
            viewModel.pluginModel.setTrackIndex(trackIndex);
            console.log("Updated PluginModel for track:", trackIndex)
            if (selectedTrackIndex !== trackIndex) {
                selectedTrackIndex = trackIndex;
                console.log("Synchronized selectedTrackIndex to:", trackIndex)
            }
            separatorVisible = true;
            console.log("Set separatorVisible to true")
        }

        onPluginEditorOpened: function(trackIndex, pluginIndex, window) {
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

        onClipMoved: function(trackIndex, clipIndex, newStartTime) {
            if (trackIndex === selectedTrackIndex) {
                viewModel.pluginModel.setTrackIndex(trackIndex);
            }
        }

        onClipAdded: function(trackIndex) {
            if (trackIndex === selectedTrackIndex) {
                viewModel.pluginModel.setTrackIndex(trackIndex);
            }
        }

        onClipDeleted: function(trackIndex, clipIndex) {
            if (trackIndex === selectedTrackIndex) {
                viewModel.pluginModel.setTrackIndex(trackIndex);
            }
        }

        onIsPlayingChanged: function() {
            console.log("Playback state changed, isPlaying:", viewModel.isPlaying, "position:", viewModel.playheadPosition)
        }

        onSelectedTrackIndexChanged: function() {
            if (selectedTrackIndex >= 0) {
                viewModel.pluginModel.setTrackIndex(selectedTrackIndex);
                console.log("Selected track changed to:", selectedTrackIndex);
                separatorVisible = true;
            } else {
                separatorVisible = false;
                console.log("No track selected, separatorVisible set to false");
            }
        }
    }

    function updateSelectedTrackFromClip() {
        if (selectedClips.length > 0) {
            // Берем первый выбранный клип (можно адаптировать для множественного выбора)
            var clip = selectedClips[0];
            selectedTrackIndex = clip.trackIndex;
            console.log("Updated selectedTrackIndex to:", selectedTrackIndex, "from clip:", clip)
        } else {
            selectedTrackIndex = -1;
            console.log("No clips selected, setting selectedTrackIndex to:", selectedTrackIndex)
        }
        // Обновляем видимость separatorPanel
       // separatorVisible = selectedTrackIndex >= 0;
    }

    function resetMainWindowParameters() {
        selectedTrackIndex = -1
        selectedClipIndex = -1
        pianoRollVisible = false
        clearSelectedClips()
        separatorVisible: false
    }

    // Подключение к сигналу изменения selectedClips
    onSelectedClipsChanged: {
        updateSelectedTrackFromClip();
    }

    function clearSelectedClips() {
        console.log("clearSelectedClips called, previous selectedClips count=" + mainWindow.selectedClips.length)
        mainWindow.selectedClips = []
        console.log("selectedClips cleared, new count=" + mainWindow.selectedClips.length)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Верхняя панель инструментов
        TopToolBar {
            id: topToolbar
            Layout.fillWidth: true

            onToggleSeparator: {
                mainWindow.separatorVisible = !mainWindow.separatorVisible
                console.log("Separator toggled, separatorVisible:", mainWindow.separatorVisible)
            }

            onTogglePianoRoll: {
                mainWindow.pianoRollVisible = !mainWindow.pianoRollVisible
                console.log("Piano Roll toggled, pianoRollVisible:", mainWindow.pianoRollVisible)
            }
        }

        // Основная рабочая область
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            DropArea {
                id: appDropArea
                anchors.fill: parent
                onDropped: (drop) => {
                    if (drop.hasUrls) {
                        var filePath = drop.urls[0].toString()
                        var globalPos = mapToItem(mainWindow.contentItem, drop.x, drop.y)
                        console.log("File dropped in app: filePath=", filePath, "x=", globalPos.x, "y=", globalPos.y)
                        // Передаем событие в обработчик браузера
                        fileBrowserContainer.children[0].fileDropped(filePath, globalPos.x, globalPos.y)
                        drop.acceptProposedAction()
                    }
                }
            }

            SplitView {
                anchors.fill: parent
                orientation: Qt.Horizontal

                // Левая панель (FileBrowser)
                // Левая часть (браузер и separator panel)
                Rectangle {
                    id: leftPanel
                    SplitView.minimumWidth: 150
                    SplitView.preferredWidth: 200
                    SplitView.maximumWidth: 250
                    color: "transparent"

                    SplitView {
                        anchors.fill: parent
                        orientation: Qt.Vertical

                        // Браузер
                        Rectangle {
                            id: fileBrowserContainer
                            SplitView.fillWidth: true
                            SplitView.preferredHeight: parent.height * 0.4 // 40% для браузера
                            color: "transparent"

                            Browser {
                                width: parent.width
                                height: parent.height
                                dragParent: mainWindow.dragParent
                                onCurrentFolderChanged: console.log("Folder changed:", currentFolder)

                                onFileDropped: (filePath, globalX, globalY) => {
                                    // Декодируем URL-encoded символы в пути
                                    var decodedPath = decodeURIComponent(filePath);
                                    // Удаляем префикс "file:///" если он есть
                                    if (decodedPath.startsWith("file:///")) {
                                        decodedPath = decodedPath.substring(8);
                                    }
                                    // Заменяем все обратные слеши на прямые
                                    decodedPath = decodedPath.replace(/\\/g, "/");
                                    var localPos = contentGrid.mapFromItem(mainWindow.dragParent, globalX, globalY)
                                    console.log("File dropped: filePath:", filePath, "localPos.x:", localPos.x, "localPos.y:", localPos.y)
                                    if (localPos.x >= 0 && localPos.x <= contentGrid.width &&
                                        localPos.y >= 0 && localPos.y <= contentGrid.height) {
                                        var trackIndex = Math.floor(localPos.y / 50)
                                        var position = Math.floor(localPos.x / flickableArea.beatWidth)
                                        console.log("Calculated trackIndex:", trackIndex, "position:", position, "countOfTracks:", countOfTracks)
                                        if (trackIndex >= 0 && trackIndex < countOfTracks && position >= 0) {
                                            var fileExt = filePath.toLowerCase().split('.').pop()
                                            if (["mp3", "wav", "aiff", "flac"].indexOf(fileExt) !== -1) {
                                                viewModel.addAudioClip(trackIndex, decodedPath, position)
                                                console.log("Added audio clip: trackIndex:", trackIndex, "filePath:", filePath, "position:", position)
                                            } else if (["dll", "vst3"].indexOf(fileExt) !== -1) {
                                                mainWindow.selectedTrackIndex = trackIndex
                                                viewModel.pluginModel.setTrackIndex(trackIndex);
                                                viewModel.pluginModel.addPlugin(trackIndex, decodedPath)
                                                mainWindow.separatorVisible = true // Показываем SeparatorPanel
                                                console.log("Added plugin: trackIndex:", trackIndex, "filePath:", filePath)
                                            } else {
                                                console.log("Invalid file type:", decodedPath)
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

                        // Separator Panel
                        Separator {
                            id: separatorPanel
                            SplitView.fillWidth: true
                            SplitView.fillHeight: true
                            panelVisible: separatorVisible
                            pluginModel: viewModel.pluginModel
                            imagesPath: mainWindow.imagesPath
                            selectedTrackIndex: mainWindow.selectedTrackIndex
                            clipIndex: selectedClipIndex
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0
                    
                    Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                        SplitView {
                            anchors.fill: parent
                            orientation: Qt.Vertical
                                // Правая панель (плейлист)
                            Rectangle {
                                id: playlistPanel
                                SplitView.fillWidth: true
                                SplitView.preferredHeight: parent.height * 0.4 // Начальная высота - 40%
                                color: "#1E1E1E"

                                Flickable {
                                    id: verticalFlickable
                                    anchors.fill: parent
                                    contentHeight: trackHeaders.height // Высота контента зависит от RowLayout
                                    contentWidth: rowLayout.width
                                    flickableDirection: Flickable.VerticalFlick // Только вертикальная прокрутка
                                    clip: true

                                    ScrollBar.vertical: ScrollBar {
                                        id: verticalScrollBar
                                        active: verticalFlickable.moving || true // Всегда активен
                                        policy: ScrollBar.AsNeeded // Показывать, если контент больше области
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        spacing: 0

                                        // Фиксированные заголовки треков
                                        // Фиксированные заголовки треков
                                        Column {
                                            id: trackHeaders
                                            width: 150
                                            Layout.fillHeight: true                                
                                            Rectangle {
                                                width: 150
                                                height: 50
                                                color: "#2D2D2D"

                                                Row {
                                                    anchors.centerIn: parent
                                                    spacing: 20

                                                    // Регулятор громкости (Dial)
                                                    Dial {
                                                        id: masterGainDial
                                                        width: 35
                                                        height: 35
                                                        from: 0
                                                        to: 200
                                                        value: viewModel.masterGain * 100// Начальное значение (как в TopToolBar)
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        onValueChanged: {
                                                            viewModel.setMasterGain(value / 100)
                                                            console.log("MasterGain volume set to:", value)
                                                        }

                                                        handle: null

                                                        // Определяем углы для фиксированного закрашивания
                                                        readonly property real fixedStartAngle: 130
                                                        readonly property real fixedEndAngle: 270

                                                        // Обработка колесика мыши
                                                        MouseArea {
                                                            anchors.fill: parent
                                                            hoverEnabled: true
                                                            onWheel: {
                                                                if (wheel.angleDelta.y > 0) {
                                                                    masterGainDial.value = Math.min(masterGainDial.to, masterGainDial.value + 5)
                                                                } else {
                                                                    masterGainDial.value = Math.max(masterGainDial.from, masterGainDial.value - 5)
                                                                }
                                                                wheel.accepted = true
                                                            }
                                                        }

                                                        background: Rectangle {
                                                            color: "transparent"
                                                            border.color: "white"
                                                            border.width: 2
                                                            radius: width / 2

                                                            Rectangle {
                                                                id: customHandle
                                                                width: 2
                                                                height: parent.width * 0.4
                                                                color: "white"
                                                                antialiasing: true
                                                                x: parent.width / 2 - width / 2
                                                                y: parent.height / 2 - height
                                                                rotation: masterGainDial.angle
                                                                transformOrigin: Item.Bottom
                                                            }
                                                        }

                                                        Shape {
                                                            anchors.fill: parent

                                                            ShapePath {
                                                                fillColor: "transparent"
                                                                strokeColor: "#8690fa"
                                                                strokeWidth: 2
                                                                capStyle: ShapePath.RoundCap

                                                                PathAngleArc {
                                                                    centerX: masterGainDial.width / 2
                                                                    centerY: masterGainDial.height / 2
                                                                    radiusX: masterGainDial.width / 2 - 1
                                                                    radiusY: masterGainDial.height / 2 - 1
                                                                    startAngle: masterGainDial.fixedStartAngle
                                                                    sweepAngle: masterGainDial.fixedEndAngle - masterGainDial.fixedStartAngle + masterGainDial.angle
                                                                }
                                                            }
                                                        }
                                                    }

                                                    // Надпись "Master" по вертикали
                                                    Label {
                                                        text: "Master"
                                                        color: "#CCC" // Как в заголовках треков
                                                        font.pixelSize: 15 // Как в заголовках треков
                                                        anchors.verticalCenter: parent.verticalCenter                                                        
                                                        verticalAlignment: Text.AlignVCenter
                                                    }
                                                }
                                            }
                                            

                                            Repeater {
                                                model: viewModel.trackModel
                                                Rectangle {
                                                    id: trackControl
                                                    width: 150
                                                    height: 52
                                                    color: {
                                                        if (model.trackType === "Audio") return "#4A2C2C" // Темно-красный
                                                        if (model.trackType === "Midi") return "#2C4A2C"  // Темно-зеленый
                                                        if (model.trackType === "Sampler") return "#2C3C4A" // Темно-голубой
                                                        return "#2D2D2D" // Цвет по умолчанию
                                                    }
                                                    border.color: "#444"
                                                    property real lastVolume: model.gain !== undefined ? model.gain : 30

                                                    MouseArea {
                                                        anchors.fill: parent
                                                        acceptedButtons: Qt.RightButton
                                                        onClicked: {
                                                            if (mouse.button === Qt.RightButton) {
                                                                contextMenu.popup()
                                                            }
                                                        }
                                                    }

                                                    Menu {
                                                        id: contextMenu
                                                        width: 130
                                                        topPadding: 2
                                                        bottomPadding: 2
                                                        delegate: MenuItem {
                                                            id: menuItem
                                                            implicitHeight: 10
                                                            padding: 4
                                                            contentItem: Text {
                                                                text: parent.text
                                                                color: "#EEE"
                                                                font.pixelSize: 11
                                                                horizontalAlignment: Text.AlignLeft
                                                                verticalAlignment: Text.AlignVCenter
                                                            }
                                                            background: Rectangle {
                                                                color: parent.highlighted ? "#555" : "transparent"
                                                                radius: 2
                                                            }
                                                        }
                                                        MenuItem {
                                                            text: "Удалить дорожку"
                                                            onTriggered: viewModel.deleteTrack(index)
                                                        }
                                                    }

                                                    Dial {
                                                        id: trackVolumeDial
                                                        width: 35
                                                        height: 35
                                                        from: 0
                                                        to: 200
                                                        value: model.gain * 100 !== undefined ? model.gain * 100 : 30 // Начальное значение
                                                        anchors.left: parent.left
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        anchors.leftMargin: 25
                                                        onValueChanged: {
                                                            if (Math.abs(value - (model.gain || 30)) > 0.001) { // Защита от цикла
                                                                if (value > 0) {
                                                                    trackControl.lastVolume = value
                                                                }
                                                                viewModel.setTrackGain(index, value / 100)
                                                                if (value > 0 && model.muted) {
                                                                    viewModel.setTrackMute(index, false)
                                                                }
                                                                console.log("Track " + index + " gain set to " + value)
                                                            }
                                                        }
                                                        handle: null
                                                        readonly property real fixedStartAngle: 130
                                                        readonly property real fixedEndAngle: 270

                                                        MouseArea {
                                                            anchors.fill: parent
                                                            hoverEnabled: true
                                                            onWheel: {
                                                                if (wheel.angleDelta.y > 0) {
                                                                    trackVolumeDial.value = Math.min(trackVolumeDial.to, trackVolumeDial.value + 5)
                                                                } else {
                                                                    trackVolumeDial.value = Math.max(trackVolumeDial.from, trackVolumeDial.value - 5)
                                                                }
                                                                wheel.accepted = true
                                                            }
                                                        }

                                                        background: Rectangle {
                                                            color: "transparent"
                                                            border.color: "white"
                                                            border.width: 2
                                                            radius: width / 2
                                                            Rectangle {
                                                                width: 2
                                                                height: parent.width * 0.4
                                                                color: "white"
                                                                antialiasing: true
                                                                x: parent.width / 2 - width / 2
                                                                y: parent.height / 2 - height
                                                                rotation: trackVolumeDial.angle
                                                                transformOrigin: Item.Bottom
                                                            }
                                                        }

                                                        Shape {
                                                            anchors.fill: parent
                                                            ShapePath {
                                                                fillColor: "transparent"
                                                                strokeColor: "#8690fa"
                                                                strokeWidth: 2
                                                                capStyle: ShapePath.RoundCap
                                                                PathAngleArc {
                                                                    centerX: trackVolumeDial.width / 2
                                                                    centerY: trackVolumeDial.height / 2
                                                                    radiusX: trackVolumeDial.width / 2 - 1
                                                                    radiusY: trackVolumeDial.height / 2 - 1
                                                                    startAngle: trackVolumeDial.fixedStartAngle
                                                                    sweepAngle: trackVolumeDial.fixedEndAngle - trackVolumeDial.fixedStartAngle + trackVolumeDial.angle
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Column {
                                                        anchors.left: trackVolumeDial.right
                                                        anchors.right: parent.right
                                                        anchors.top: parent.top
                                                        anchors.bottom: parent.bottom
                                                        anchors.leftMargin: 4
                                                        anchors.topMargin: 2
                                                        spacing: 1 // Добавляем небольшой отступ между элементами

                                                        Rectangle {
                                                            id: trackNameControl
                                                            width: parent.width
                                                            height: 16
                                                            color: "transparent"
                                                            border.color: trackNameInput.activeFocus ? "#8690fa" : "transparent"
                                                            border.width: 0.5
                                                            radius: 2

                                                            property string trackName: model.name || ""

                                                            TextInput {
                                                                id: trackNameInput
                                                                anchors.fill: parent
                                                                horizontalAlignment: TextInput.AlignHCenter
                                                                verticalAlignment: TextInput.AlignVCenter
                                                                color: "white"
                                                                font.pixelSize: 11
                                                                text: trackNameControl.trackName
                                                                visible: false
                                                                selectByMouse: true
                                                                activeFocusOnPress: true
                                                                maximumLength: 20
                                                                validator: RegularExpressionValidator { regularExpression: /.{0,20}/ }

                                                                Keys.onPressed: (event) => {
                                                                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                                                        event.accepted = true
                                                                        handleInput()
                                                                    }
                                                                }

                                                                onActiveFocusChanged: {
                                                                    if (!activeFocus && visible) {
                                                                        handleInput()
                                                                    }
                                                                }

                                                                function handleInput() {
                                                                    viewModel.setName(index, text)
                                                                    trackNameControl.trackName = text
                                                                    console.log("Track " + index + " name set to: " + text)
                                                                    hideInput()
                                                                }

                                                                function hideInput() {
                                                                    visible = false
                                                                    trackNameText.visible = true
                                                                    focus = false // Сбрасываем фокус для убирания обводки
                                                                }
                                                            }

                                                            Text {
                                                                id: trackNameText
                                                                anchors.centerIn: parent
                                                                text: trackNameControl.trackName || "Track " + (index + 1) // Изменено на "track" с маленькой буквы
                                                                color: "white"
                                                                font.pixelSize: 11
                                                                opacity: trackNameControl.trackName ? 1.0 : 0.5
                                                            }

                                                            MouseArea {
                                                                anchors.fill: parent
                                                                hoverEnabled: true
                                                                cursorShape: Qt.PointingHandCursor
                                                                acceptedButtons: Qt.LeftButton

                                                                onDoubleClicked: {
                                                                    trackNameInput.text = trackNameControl.trackName
                                                                    trackNameText.visible = false
                                                                    trackNameInput.visible = true
                                                                    trackNameInput.forceActiveFocus()
                                                                    trackNameInput.selectAll()
                                                                }

                                                                onClicked: {
                                                                    if (!trackNameInput.activeFocus) {
                                                                        trackNameInput.text = trackNameControl.trackName
                                                                        trackNameText.visible = false
                                                                        trackNameInput.visible = true
                                                                        trackNameInput.forceActiveFocus()
                                                                    }
                                                                }
                                                            }
                                                        }

/*                                                         Label {
                                                            width: parent.width
                                                            horizontalAlignment: Text.AlignHCenter
                                                            text: "(" + model.trackType + ")"
                                                            color: "#CCC"
                                                            font.pixelSize: 9 // Уменьшаем размер шрифта для компактности
                                                            elide: Text.ElideRight
                                                        } */

                                                        Row {
                                                            anchors.horizontalCenter: parent.horizontalCenter
                                                            spacing: 2

                                                            RoundButton {
                                                                id: muteButton
                                                                width: 30
                                                                height: 30
                                                                radius: width / 2
                                                                ToolTip.visible: hovered
                                                                ToolTip.delay: 500
                                                                ToolTip.text: model.muted ? "Unmute" : "Mute"

                                                                background: Rectangle {
                                                                    radius: parent.radius
                                                                    color: muteButton.hovered ? "#d0d0d0" : "transparent"
                                                                    border.color: muteButton.hovered ? "#a0a0a0" : "transparent"
                                                                    border.width: 1
                                                                    Behavior on color { ColorAnimation { duration: 100 } }
                                                                    Behavior on border.color { ColorAnimation { duration: 100 } }
                                                                }

                                                                Image {
                                                                    id: rbmute
                                                                    anchors.centerIn: parent
                                                                    width: 18
                                                                    height: 18
                                                                    source: model.muted ? imagesPath + "muted.png" : imagesPath + "unmuted.png"
                                                                    sourceSize.width: 18
                                                                    sourceSize.height: 18
                                                                    opacity: muteButton.down ? 0.7 : 1.0
                                                                    fillMode: Image.PreserveAspectFit
                                                                    Behavior on opacity { NumberAnimation { duration: 100 } }
                                                                }

                                                                onClicked: {
                                                                    muteButton.scale = 0.95
                                                                    viewModel.setTrackMute(index, !model.muted)
                                                                    console.log("Track " + index + (model.muted ? " unmuted" : " muted"))
                                                                    rbmute.source = model.muted ? imagesPath + "muted.png" : imagesPath + "unmuted.png"
                                                                }

                                                                Behavior on scale {
                                                                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                                                                }
                                                            }

                                                            /* RoundButton {
                                                                id: soloButton
                                                                width: 30
                                                                height: 30
                                                                radius: width / 2
                                                                ToolTip.visible: hovered
                                                                ToolTip.delay: 500
                                                                ToolTip.text: "Solo"

                                                                background: Rectangle {
                                                                    radius: parent.radius
                                                                    color: soloButton.hovered ? "#d0d0d0" : "transparent"
                                                                    border.color: soloButton.hovered ? "#a0a0a0" : "transparent"
                                                                    border.width: 1
                                                                    Behavior on color { ColorAnimation { duration: 100 } }
                                                                    Behavior on border.color { ColorAnimation { duration: 100 } }
                                                                }

                                                                Image {
                                                                    anchors.centerIn: parent
                                                                    width: 18
                                                                    height: 18
                                                                    source: imagesPath + "solo.png"
                                                                    sourceSize.width: 18
                                                                    sourceSize.height: 18
                                                                    opacity: soloButton.down ? 0.7 : 1.0
                                                                    fillMode: Image.PreserveAspectFit
                                                                    Behavior on opacity { NumberAnimation { duration: 100 } }
                                                                }

                                                                onClicked: {
                                                                    soloButton.scale = 0.95
                                                                    viewModel.toggleSolo(index)
                                                                    console.log("Track " + index + " solo toggled")
                                                                }

                                                                Behavior on scale {
                                                                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                                                                }
                                                            } */
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
                                                                        color: model.color !== undefined ? model.color : "#808080"
                                                                        opacity: 0.6
                                                                        radius: 3
                                                                        border.width: isSelected ? 3 : 1 // Жёлтая рамка при выделении
                                                                        border.color: isSelected ? "#FFFF00" : Qt.darker(color, 1.2)
                                                                        z: 4

                                                                        property bool isLabelVisible: {
                                                                            // Показываем лейбл только если ширина клипа больше минимально допустимой
                                                                            var minWidthRequired = clipLabel.implicitWidth + 10 // 10 - это отступы и небольшой запас
                                                                            return width > minWidthRequired
                                                                        }
                                                                        // Верхняя зона (20%) для лейбла
                                                                        Item {
                                                                            id: topZone
                                                                            anchors.top: parent.top
                                                                            anchors.left: parent.left
                                                                            anchors.right: parent.right
                                                                            height: parent.height * 0.2

                                                                        Label {
                                                                                id: clipLabel
                                                                                anchors.left: parent.left
                                                                                anchors.top: parent.top  // Фиксируем сверху вместо verticalCenter
                                                                                topPadding: 3  // Добавляем отступ сверху
                                                                                leftPadding: 5
                                                                                text: {
                                                                                    if (model.type === "midi") {
                                                                                        return "Midi";
                                                                                    } else if (model.type === "sampler") {
                                                                                        return "Sampler";
                                                                                    } else if (model.file) {
                                                                                        var parts = model.file.split(/[\\/]/);
                                                                                        return parts[parts.length - 1];
                                                                                    }
                                                                                    return "";
                                                                                }
                                                                                color: "#FFFFFF"
                                                                                font.pixelSize: 10 // Устанавливаем фиксированный размер шрифта в пикселях
                                                                                font.letterSpacing: 0.8
                                                                                elide: Text.ElideRight
                                                                                maximumLineCount: 1
                                                                                opacity: 1
                                                                                style: Text.Outline
                                                                                styleColor: "#000000"
                                                                                renderType: Text.NativeRendering
                                                                                smooth: true
                                                                                visible: clipRectangle.isLabelVisible
                                                                            }
                                                                        }

                                                                        // Нижняя зона (80%) для остального содержимого
                                                                        Item {
                                                                            id: bottomZone
                                                                            anchors.top: topZone.bottom
                                                                            anchors.left: parent.left
                                                                            anchors.right: parent.right
                                                                            anchors.bottom: parent.bottom

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
                                                                                        // Обновляем clipDuration в midiModel для MIDI-клипов
                                                                                        if (model.type === "midi" && trackIndex === mainWindow.selectedTrackIndex && index === mainWindow.selectedClipIndex) {
                                                                                            viewModel.midiModel.setClipDuration(newDurationBeats)
                                                                                            // Принудительно обновляем clipIndex, чтобы обновить PianoView
                                                                                            mainWindow.selectedClipIndex = -1
                                                                                            mainWindow.selectedClipIndex = index
                                                                                            console.log("Updated midiModel.clipDuration to", newDurationBeats, "for trackIndex=", trackIndex, "clipIndex=", index)
                                                                                        }
                                                                                        // Очищаем и обновляем волноформу после изменения
                                                                                        if (model.type === "audio") {
                                                                                            waveformImage.source = ""
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
                                                                                        if (model.type === "audio") {
                                                                                            waveformImage.source = ""
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
                                                                                    clipsModel.setData(clipsModel.index(index, 0), newDurationBeats, 258) // 258 = Qt::UserRole + 2, если DurationBeatsRole = 258
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
                                                                                        // Обновляем clipDuration в midiModel для MIDI-клипов
                                                                                        if (model.type === "midi" && trackIndex === mainWindow.selectedTrackIndex && index === mainWindow.selectedClipIndex) {
                                                                                            // Принудительно обновляем clipIndex, чтобы обновить PianoView
                                                                                            mainWindow.selectedClipIndex = -1
                                                                                            mainWindow.selectedClipIndex = index
                                                                                            viewModel.midiModel.setClipDuration(newDurationBeats)
                                                                                            console.log("Updated midiModel.clipDuration to", newDurationBeats, "for trackIndex=", trackIndex, "clipIndex=", index)
                                                                                        }
                                                                                        // Очищаем и обновляем волноформу после изменения
                                                                                        if (model.type === "audio") {
                                                                                            waveformImage.source = ""
                                                                                            waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                                                                            console.log("Waveform updated after right resize: width=", clipRectangle.width, "source=", waveformImage.source)
                                                                                        }
                                                                                        console.log("Resized right: trackIndex=", trackIndex, "clipIndex=", index, "newDurationBeats=", newDurationBeats)
                                                                                    } else {
                                                                                        console.log("Invalid duration, reverting: newDurationBeats=", newDurationBeats)
                                                                                        clipItem.width = model.durationBeats * flickableArea.beatWidth
                                                                                        clipsModel.setData(index, "durationBeats", model.durationBeats)
                                                                                        if (model.type === "audio") {
                                                                                            waveformImage.source = ""
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
                                                                                mainWindow.selectedTrackIndex = trackIndex
                                                                                // Сохраняем индекс клона для label
                                                                                mainWindow.displayedClipIndex = index
                                                                                // Если это клон, выбираем мастер-клип
                                                                                let targetClipIndex = model.masterClipIndex >= 0 ? model.masterClipIndex : index
                                                                                mainWindow.selectedClipIndex = targetClipIndex
                                                                                viewModel.pluginModel.setTrackIndex(trackIndex); // Синхронизируем pluginModel
                                                                                mainWindow.separatorVisible = true; // Показываем SeparatorPanel
                                                                                if ((model.type === "midi" || model.type === "sampler" )&& mainWindow.pianoRollAutoOpen) {                                                                                    
                                                                                    mainWindow.pianoRollVisible = true
                                                                                    console.log(`Opening Piano Roll: trackIndex=${trackIndex}, clipIndex=${index}`)
                                                                                } else {
                                                                                    mainWindow.pianoRollVisible = false
                                                                                }
                                                                            }
                                                                            mouse.accepted = true
                                                                        }

                                                                        onPositionChanged: (mouse) => {
                                                                            if (drag.active) {
                                                                                var newX = clipItem.x
                                                                                var newPosition = flickableArea.beatWidth > 0 ? newX / flickableArea.beatWidth : 0
                                                                                console.log("Dragging: newPosition=", newPosition)
                                                                            }
                                                                        }

                                                                        onReleased: {
                                                                            var groupSize = flickableArea.cachedGroupSize
                                                                            var snapStep = groupSize * flickableArea.beatWidth // Шаг сетки в пикселях
                                                                            var nearestGridX = Math.round(clipItem.x / snapStep) * snapStep
                                                                            var distanceToGrid = Math.abs(clipItem.x - nearestGridX)
                                                                            var threshold = 8 // Порог привязки в пикселях (8 пикселей, как в кликах по contentGrid)

                                                                            // Привязываем к сетке, если расстояние до ближайшей точки меньше или равно порогу
                                                                            var snappedX = distanceToGrid <= threshold ? nearestGridX : clipItem.x
                                                                            snappedX = Math.max(0, Math.min(snappedX, contentGrid.width - clipItem.width))
                                                                            var deltaX = snappedX - (model.startBeats * flickableArea.beatWidth) // Смещение для главного клипа
                                                                            var newPosition = flickableArea.beatWidth > 0 ? snappedX / flickableArea.beatWidth : 0
                                                                            newPosition = Math.round(newPosition * 1000) / 1000 // Округление до 3 десятичных знаков

                                                                            // Обновляем главный клип
                                                                            clipItem.x = snappedX
                                                                            viewModel.moveClip(trackIndex, index, newPosition)

                                                                            mainWindow.clipMoved(trackIndex, index, newPosition)

                                                                            // Для всех выделенных клипов этого трека
                                                                            for (var i = 0; i < clipsRepeater.count; i++) {
                                                                                var otherClip = clipsRepeater.itemAt(i)
                                                                                if (otherClip && otherClip.isSelected && otherClip !== clipItem) {
                                                                                    // Вычисляем новое положение с учетом смещения главного клипа
                                                                                    var otherX = otherClip.x + deltaX
                                                                                    // Привязываем к сетке с тем же порогом
                                                                                    var otherNearestGridX = Math.round(otherX / snapStep) * snapStep
                                                                                    var otherDistanceToGrid = Math.abs(otherX - otherNearestGridX)
                                                                                    var otherSnappedX = otherDistanceToGrid <= threshold ? otherNearestGridX : otherX
                                                                                    otherSnappedX = Math.max(0, Math.min(otherSnappedX, contentGrid.width - otherClip.width))
                                                                                    var otherNewPosition = flickableArea.beatWidth > 0 ? otherSnappedX / flickableArea.beatWidth : 0
                                                                                    otherNewPosition = Math.round(otherNewPosition * 1000) / 1000

                                                                                    otherClip.x = otherSnappedX
                                                                                    viewModel.moveClip(trackIndex, otherClip.sourceIndex, otherNewPosition)
                                                                                }
                                                                            }

                                                                            clipRectangle.z = 4
                                                                            console.log("Clip snapped: trackIndex=", trackIndex, "clipIndex=", index, "x=", clipItem.x, 
                                                                                        "newPosition=", newPosition, "distanceToGrid=", distanceToGrid, 
                                                                                        "nearestGridX=", nearestGridX, "snapStep=", snapStep, 
                                                                                        "zoomLevel=", flickableArea.zoomLevel)
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
                                                                                if (model.type === "midi") {
                                                                                    viewModel.copyMidiClip(trackIndex, sourceIndex, newPosition / 2)
                                                                                    console.log(`MIDI clip copied: trackIndex=${trackIndex}, newPosition=${newPosition}`)
                                                                                } else if (model.type === "audio") {
                                                                                    viewModel.addAudioClip(trackIndex, model.file, newPosition)
                                                                                    console.log(`Audio clip copied: trackIndex=${trackIndex}, newPosition=${newPosition}`)
                                                                                }
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
                                                height: contentGrid.height  // Уменьшаем высоту полосы на высоту треугольника
                                                color: "green"
                                                z: 7
                                                x: Math.max(0, Math.min(viewModel.playheadPosition * flickableArea.beatWidth, flickableArea.contentWidth - width))
                                                anchors.top: timeRuler.bottom

                                                // Треугольник в верхней части, направленный вниз
                                                Canvas {
                                                    id: triangleHandle
                                                    width: 20
                                                    height: 10
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    anchors.top: parent.top
                                                    z: 8

                                                    onPaint: {
                                                        var ctx = getContext("2d")
                                                        ctx.clearRect(0, 0, width, height)
                                                        ctx.beginPath()
                                                        ctx.moveTo(0, 0) // Верхняя левая точка
                                                        ctx.lineTo(width / 2, height) // Вершина треугольника (внизу)
                                                        ctx.lineTo(width, 0) // Верхняя правая точка
                                                        ctx.closePath()
                                                        ctx.fillStyle = "green"
                                                        ctx.fill()
                                                    }

                                                    MouseArea {
                                                        id: triangleMouseArea
                                                        anchors.fill: parent
                                                        drag.target: greenline
                                                        drag.axis: Drag.XAxis
                                                        drag.minimumX: 0
                                                        drag.maximumX: Math.max(0, flickableArea.contentWidth - greenline.width)

                                                        onPressed: {
                                                            viewModel.setIsDraggingPlayhead(true) // Устанавливаем флаг
                                                            viewModel.setIsPlaying(false) // Приостанавливаем воспроизведение (опционально)
                                                            console.log("MainWindow: Triangle handle pressed, isDraggingPlayhead=", viewModel.isDraggingPlayhead)
                                                        }

                                                        onReleased: {
                                                            var groupSize = flickableArea.cachedGroupSize
                                                            var snapStep = groupSize * flickableArea.beatWidth // Шаг сетки в пикселях
                                                            var nearestGridX = Math.round(greenline.x / snapStep) * snapStep
                                                            var distanceToGrid = Math.abs(greenline.x - nearestGridX)
                                                            var threshold = 8 // Порог привязки в пикселях (как у клипов)

                                                            // Привязываем к сетке, если расстояние до ближайшей точки меньше или равно порогу
                                                            var snappedX = distanceToGrid <= threshold ? nearestGridX : greenline.x
                                                            snappedX = Math.max(0, Math.min(snappedX, flickableArea.contentWidth - greenline.width))
                                                            var newPosition = flickableArea.beatWidth > 0 ? snappedX / flickableArea.beatWidth : 0
                                                            newPosition = Math.round(newPosition * 1000) / 1000 // Округление до 3 десятичных знаков

                                                            greenline.x = snappedX
                                                            viewModel.setPlayheadPosition(newPosition)

                                                            console.log("Greenline snapped: x=", greenline.x, 
                                                                        "newPosition=", newPosition, 
                                                                        "distanceToGrid=", distanceToGrid, 
                                                                        "nearestGridX=", nearestGridX, 
                                                                        "snapStep=", snapStep, 
                                                                        "zoomLevel=", flickableArea.zoomLevel)
                                                        }
                                                    }
                                                }

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

                                                    onPressed: {
                                                        viewModel.setIsDraggingPlayhead(true) // Устанавливаем флаг
                                                        viewModel.setIsPlaying(false) // Приостанавливаем воспроизведение (опционально)
                                                        console.log("MainWindow: Green line pressed, isDraggingPlayhead=", viewModel.isDraggingPlayhead)
                                                    }

                                                    onReleased: {
                                                        var groupSize = flickableArea.cachedGroupSize
                                                        var snapStep = groupSize * flickableArea.beatWidth // Шаг сетки в пикселях
                                                        var nearestGridX = Math.round(greenline.x / snapStep) * snapStep
                                                        var distanceToGrid = Math.abs(greenline.x - nearestGridX)
                                                        var threshold = 8 // Порог привязки в пикселях (как у клипов)

                                                        // Привязываем к сетке, если расстояние до ближайшей точки меньше или равно порогу
                                                        var snappedX = distanceToGrid <= threshold ? nearestGridX : greenline.x
                                                        snappedX = Math.max(0, Math.min(snappedX, flickableArea.contentWidth - greenline.width))
                                                        var newPosition = flickableArea.beatWidth > 0 ? snappedX / flickableArea.beatWidth : 0
                                                        newPosition = Math.round(newPosition * 1000) / 1000 // Округление до 3 десятичных знаков

                                                        greenline.x = snappedX
                                                        viewModel.setPlayheadPosition(newPosition)



                                                        console.log("Greenline snapped: x=", greenline.x, 
                                                                    "newPosition=", newPosition, 
                                                                    "distanceToGrid=", distanceToGrid, 
                                                                    "nearestGridX=", nearestGridX, 
                                                                    "snapStep=", snapStep, 
                                                                    "zoomLevel=", flickableArea.zoomLevel)
                                                    }
                                                }

                                                Connections {
                                                    target: viewModel
                                                    function onPlayheadPositionChanged(position) {
                                                        if (!triangleMouseArea.drag.active) {
                                                            greenline.x = position * flickableArea.beatWidth
                                                            greenline.x = Math.max(0, Math.min(greenline.x, flickableArea.contentWidth - greenline.width))
                                                        }
                                                    }
                                                }

                                                Connections {
                                                    target: flickableArea
                                                    function onBeatWidthChanged() {
                                                        if (!viewModel.isPlaying && !triangleMouseArea.drag.active) {
                                                            greenline.x = viewModel.playheadPosition * flickableArea.beatWidth
                                                            greenline.x = Math.max(0, Math.min(greenline.x, flickableArea.contentWidth - greenline.width))
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
                            }

                            PianoView {
                                Layout.fillWidth: true
                                SplitView.preferredHeight: parent.height * 0.6 // Начальная высота - 40%
                                visible: pianoRollVisible
                                trackIndex: selectedTrackIndex
                                clipIndex: selectedClipIndex
                                clipDuration: viewModel.midiModel.clipDuration
                                imagesPath: mainWindow.imagesPath

                                Component.onCompleted: {
                                    console.log("MainWindow: PianoView initialized with trackIndex=", selectedTrackIndex, "clipIndex=", selectedClipIndex, "clipDuration=", viewModel.midiModel.clipDuration)
                                }

                                onTrackIndexChanged: {
                                    console.log("MainWindow: PianoView trackIndex changed to", trackIndex, "clipDuration=", viewModel.midiModel.clipDuration)
                                }

                                onClipIndexChanged: {
                                    console.log("MainWindow: PianoView clipIndex changed to", clipIndex, "clipDuration=", viewModel.midiModel.clipDuration)
                                }
                            }

                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 5 // Толщина полосы
                        color: "white" // Белый цвет
                        z: 11 // Убеждаемся, что полоса поверх
                    }
                }
            }  
        }
    }
}