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
    property string imagesPath: "file:///" + viewModel.applicationHomeFolder() + "/images/"

    property Item dragParent: contentItem
    property int countOfTracks: viewModel.trackModel.countOfTracks

    property bool multiSelectMode: false // Режим множественного выделения (удерживать Ctrl)
    property var selectedClips: []

    property bool separatorVisible: false
    signal clearSelectedClipsRequested()

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
                    leftPadding: 10  // Небольшой отступ слева для всего Row
                    //Кнопка "файл"
                    ToolButton {
                        id: fileButton
                        text: "Файл"
                        implicitWidth: 80
                        height: parent.height
                        anchors.verticalCenter: parent.verticalCenter                       
                        contentItem: Text {
                            text: fileButton.text
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter 
                            verticalAlignment: Text.AlignVCenter
                            color: "white"
                        }     
                        onClicked: fileMenu.open()
            
                        Menu {
                            id: fileMenu
                            y: fileButton.height
                            width: 130       
                            MenuItem {
                                text: "Создать"
                                
                                onTriggered: { /* действие */ }
                            }

                            MenuItem {
                                text: "Загрузить..."                                
                                onTriggered: { /* действие */ }
                            }

                            MenuItem {
                                text: "Открыть"                                
                                onTriggered: {
                                    viewModel.OpenProject("C:\\Users\\llvvv\\source\\repos\\Studio\\Result\\Save.json")
                                }
                            }

                            MenuItem {
                                text: "Сохранить"
                                onTriggered: {
                                    viewModel.SaveProject("C:\\Users\\llvvv\\source\\repos\\Studio\\Result\\Save.json")
                                }
                            }

                            MenuItem {
                                text: "Сохранить как..."                                
                                onTriggered: { /* действие */ }
                            }
                        }
                    }    
                    ToolButton{
                    }
                    // Кнопка "Справка"
                    ToolButton {
                        id: helpButton
                        text: "Справка"
                        implicitWidth: 80
                        height: parent.height
                        anchors.verticalCenter: parent.verticalCenter
                        contentItem: Text {
                            text: helpButton.text
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter 
                            verticalAlignment: Text.AlignVCenter
                            color: "white"
                        }
                        onClicked: helpMenu.open()

                        Menu {
                            id: helpMenu
                            y: helpButton.height
                            width: 130
                            MenuItem {
                                id: editItem
                                text: "Открыть справку"
                                hoverEnabled: true
                                onClicked: {
                                    // Формируем путь: "file:///[app_folder]/help.html"
                                    var helpFilePath = "file:///" + viewModel.applicationHomeFolder() + "/help.html"
                                    console.log("Opening help file:", helpFilePath) // Для отладки
                                    Qt.openUrlExternally(helpFilePath)
                                }
                            }
            
                            MenuItem {
                                id: aboutItem
                                text: "Об авторах"
                                hoverEnabled: true
                                
            
                                Menu {
                                    id: aboutMenu
                                    y: 0
                                    x: parent.width
                                    width: 150
                                    height: 40
                                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                
                                    
                
                                    Label {
                                        anchors.centerIn: parent
                                        text: "LeTo corporation"
                                        font.pixelSize: 11
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
            
                                onHoveredChanged: {
                                    if (hovered) {
                                        aboutMenu.open()
                                    } else {
                                        aboutMenu.close()
                                    }
                                }
                            }
                        }
                    }
                }   
                Item {
                    Layout.fillWidth: true
                    Row {
                        id: centerRow
                        anchors.centerIn: parent
                        spacing: 10
                        // Переключатель Pattern/Song (добавлен слева от центрального ряда)
                        TabBar {
                            id: modeTabBar
                            Material.accent: "transparent"  // Убираем акцентный цвет (красный)
                            Material.background: "transparent"
                            anchors.verticalCenter: parent.verticalCenter // Центрируем всю строку

                            spacing: 10
                            currentIndex: 1  // По умолчанию SONG
                            // Убираем стандартный индикатор TabBar

                    
            
                            background: Rectangle {
                                color: "transparent"
                            }                      

                            TabButton {
                                width: 35
                                height: 25  // Уменьшенная высота
                                text: "PAT"
                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: "Проигрывать только pattern"

                                font {
                                    family: "Tahoma"
                                    pixelSize: 11
                                }

                                background: Rectangle {
                                    radius: 5  // Более закругленные углы
                                    color: parent.checked ? "#8690FA" : (parent.hovered ? "#D8DDFC" : "#CCD2FC")
                                    border.color: "#8293FC"
                                    border.width: 2  // Более широкий контур
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: parent.checked ? "#FFFFFF" : "#5153FF"  // Белый текст при выборе
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    anchors.fill: parent
                                }
                            }

                            TabButton {
                                width: 35
                                height: 25  // Уменьшенная высота
                                text: "SONG"
                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: "Проигрывать вместе с треками"

                                font {
                                    family: "Tahoma"
                                    pixelSize: 11
                                }

                                background: Rectangle {
                                    radius: 5  // Более закругленные углы
                                    color: parent.checked ? "#8690FA" : (parent.hovered ? "#D8DDFC" : "#CCD2FC")
                                    border.color: "#8293FC"
                                    border.width: 2  // Более широкий контур
                                }

                                contentItem: Text {
                                    text: parent.text
                                    font: parent.font
                                    color: parent.checked ? "#FFFFFF" : "#5153FF"  // Белый текст при выборе
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    anchors.fill: parent
                                }
                            }                    
                        }
                        Row {
                            anchors.verticalCenter: parent.verticalCenter // Центрируем всю строку
                            spacing: 5
                            RoundButton {
                                id: playButton
                                width: 40
                                height: 40
                                radius: width / 2
                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: viewModel.isPlaying ? "Пауза" : "Воспроизвести"
            
                                background: Rectangle {
                                    radius: parent.radius
                                    color: playButton.hovered ? "#d0d0d0" : "transparent"
                                    border.color: playButton.hovered ? "#a0a0a0" : "transparent"
                                    border.width: 1

                                    Behavior on color {
                                        ColorAnimation { duration: 100 }
                                    }
                                    Behavior on border.color {
                                        ColorAnimation { duration: 100 }
                                    }
                                }

                                Image {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    source: viewModel.isPlaying ? imagesPath + "pause.png" : imagesPath + "play.png"
                                    sourceSize.width: 24
                                    sourceSize.height: 24
                                    opacity: playButton.down ? 0.7 : 1.0
                                    fillMode: Image.PreserveAspectFit

                                    Behavior on opacity {
                                        NumberAnimation { duration: 100 }
                                    }
                                }

                                onClicked: {
                                    playButton.scale = 0.95
                                    viewModel.togglePlayback()
                                }
            
                                Behavior on scale {
                                    NumberAnimation { 
                                        duration: 100
                                        easing.type: Easing.OutQuad 
                                    }
                                }
                            }
                            RoundButton {
                                id: stopButton
                                width: 40
                                height: 40
                                radius: width / 2
                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: "Стоп"
            
                                background: Rectangle {
                                    radius: parent.radius
                                    color: stopButton.hovered ? "#d0d0d0" : "transparent"
                                    border.color: stopButton.hovered ? "#a0a0a0" : "transparent"
                                    border.width: 1

                                    Behavior on color {
                                        ColorAnimation { duration: 100 }
                                    }
                                    Behavior on border.color {
                                        ColorAnimation { duration: 100 }
                                    }
                                }

                                Image {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    source: imagesPath + "stop.png"
                                    sourceSize.width: 24
                                    sourceSize.height: 24
                                    opacity: stopButton.down ? 0.7 : 1.0
                                    fillMode: Image.PreserveAspectFit

                                    Behavior on opacity {
                                        NumberAnimation { duration: 100 }
                                    }
                                }

                                onClicked: {
                                    stopButton.scale = 0.95
                                    // Добавьте здесь логику для кнопки стоп
                                }
            
                                Behavior on scale {
                                    NumberAnimation { 
                                        duration: 100
                                        easing.type: Easing.OutQuad 
                                    }
                                }
                            }
                            RoundButton {
                                id: rewindButton
                                width: 40
                                height: 40
                                radius: width / 2
                                ToolTip.visible: hovered
                                ToolTip.delay: 500
                                ToolTip.text: "Перемотать в начало"
            
                                background: Rectangle {
                                    radius: parent.radius
                                    color: rewindButton.hovered ? "#d0d0d0" : "transparent"
                                    border.color: rewindButton.hovered ? "#a0a0a0" : "transparent"
                                    border.width: 1

                                    Behavior on color {
                                        ColorAnimation { duration: 100 }
                                    }
                                    Behavior on border.color {
                                        ColorAnimation { duration: 100 }
                                    }
                                }

                                Image {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    source: imagesPath + "rewind.png"
                                    sourceSize.width: 24
                                    sourceSize.height: 24
                                    opacity: rewindButton.down ? 0.7 : 1.0
                                    fillMode: Image.PreserveAspectFit

                                    Behavior on opacity {
                                        NumberAnimation { duration: 100 }
                                    }
                                }

                                onClicked: {
                                    rewindButton.scale = 0.95
                                    viewModel.setPlayheadPosition(0)
                                    console.log("Reset playhead to start")
                                }
            
                                Behavior on scale {
                                    NumberAnimation { 
                                        duration: 100
                                        easing.type: Easing.OutQuad 
                                    }
                                }
                            }
                        }
                    }
                }
                Row {
                    Layout.alignment: Qt.AlignRight
                    anchors.verticalCenter: parent.verticalCenter // Центрируем всю строку
                    rightPadding: 20
                    spacing: 0

                    // Первая кнопка - plugins
                    RoundButton {
                        id: pluginsButton
                        width: 40
                        height: 40
                        radius: width / 2
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: "Плагины"

                        background: Rectangle {
                            radius: parent.radius
                            color: pluginsButton.hovered ? "#d0d0d0" : "transparent"
                            border.color: pluginsButton.hovered ? "#a0a0a0" : "transparent"
                            border.width: 1

                            Behavior on color {
                                ColorAnimation { duration: 100 }
                            }
                            Behavior on border.color {
                                ColorAnimation { duration: 100 }
                            }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: imagesPath + "plugins.png"
                            sourceSize.width: 24
                            sourceSize.height: 24
                            opacity: pluginsButton.down ? 0.7 : 1.0
                            fillMode: Image.PreserveAspectFit

                            Behavior on opacity {
                                NumberAnimation { duration: 100 }
                            }
                        }

                        onClicked: {
                            pluginsButton.scale = 0.95
                            separatorVisible = !separatorVisible
                            // Действие для кнопки плагинов
                        }

                        Behavior on scale {
                            NumberAnimation { 
                                duration: 100
                                easing.type: Easing.OutQuad 
                            }
                        }
                    }

                    // Вторая кнопка - piano roll
                    RoundButton {
                        id: pianoRollButton
                        width: 40
                        height: 40
                        radius: width / 2
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: "Piano Roll"
                        anchors.leftMargin: 2 // Уменьшенный отступ слева (было 10)

                        // Здесь нужно прописать условие для enabled
                        // Например: enabled: viewModel.isPianoRollAvailable
                        // enabled: false // Пример временного отключения

                        background: Rectangle {
                            radius: parent.radius
                            color: {
                                if (!pianoRollButton.enabled) {
                                    return "#606060" // Темный цвет для недоступной кнопки
                                } else if (pianoRollButton.hovered) {
                                    return "#d0d0d0" // Цвет при наведении для доступной кнопки
                                } else {
                                    return "transparent" // Обычный цвет
                                }
                            }
                            border.color: {
                                if (!pianoRollButton.enabled) {
                                    return "transparent" // Без рамки для недоступной
                                } else if (pianoRollButton.hovered) {
                                    return "#a0a0a0" // Рамка при наведении
                                } else {
                                    return "transparent" // Обычное состояние
                                }
                            }
                            border.width: 1

                            Behavior on color {
                                ColorAnimation { duration: 100 }
                            }
                            Behavior on border.color {
                                ColorAnimation { duration: 100 }
                            }
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: imagesPath + "piano_roll.png"
                            sourceSize.width: 24
                            sourceSize.height: 24
                            opacity: pianoRollButton.down ? 0.7 : (pianoRollButton.enabled ? 1.0 : 0.5)
                            fillMode: Image.PreserveAspectFit

                            Behavior on opacity {
                                NumberAnimation { duration: 100 }
                            }
                        }

                        onClicked: {
                            if (pianoRollButton.enabled) {
                                pianoRollButton.scale = 0.95
                                // Действие для кнопки piano roll
                            }
                        }

                        Behavior on scale {
                            NumberAnimation { 
                                duration: 100
                                easing.type: Easing.OutQuad 
                            }
                        }
                    }
                    // Отступ перед регулятором громкости
                    Rectangle { width: 15; height:15; color: "#4C566A"}  // Невидимый разделитель
            
                    Label { 
                        text: "BPM:" 
                        color: "white" 
                        anchors.verticalCenter: parent.verticalCenter 
                        rightPadding: 5  // Небольшой отступ перед прямоугольником
                    }
            
                    Rectangle {
                        id: bpmControl
                        width: 60
                        height: 30
                        color: "#8690fa"
                        radius: 4
                        border.color: "#555"
                        border.width: 1
                        anchors.verticalCenter: parent.verticalCenter

                        // Начальное значение 120
                        property real bpmValue: viewModel.bpm || 120
                        onBpmValueChanged: {
                            if (bpmValue >= 60 && bpmValue <= 200) {
                                viewModel.setBpm(bpmValue)
                            } else {
                                // Если пришло недопустимое значение, вернуть последнее корректное
                                bpmValue = Math.max(60, Math.min(200, bpmValue))
                            }
                        }

                        TextInput {
                            id: bpmInput
                            anchors.fill: parent
                            horizontalAlignment: TextInput.AlignHCenter
                            verticalAlignment: TextInput.AlignVCenter
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                            validator: IntValidator { bottom: 60; top: 200 }
                            visible: false
                            selectByMouse: true
                            activeFocusOnPress: true

                            Keys.onPressed: {
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
                                var newValue = parseInt(text)
                                if (!isNaN(newValue)) {
                                    // Если введено недопустимое значение, вернуть последнее корректное
                                    if (newValue < 60 || newValue > 200) {
                                        bpmInput.text = Math.round(bpmControl.bpmValue).toString()
                                    } else {
                                        bpmControl.bpmValue = newValue
                                    }
                                }
                                hideInput()
                            }

                            function hideInput() {
                                visible = false
                                bpmText.visible = true
                            }
                        }

                        Text {
                            id: bpmText
                            anchors.centerIn: parent
                            text: Math.round(bpmControl.bpmValue)
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor

                            onDoubleClicked: {
                                bpmInput.text = Math.round(bpmControl.bpmValue).toString()
                                bpmText.visible = false
                                bpmInput.visible = true
                                bpmInput.forceActiveFocus()
                                bpmInput.selectAll()
                            }
                        }                        
                    }
                    // Отступ перед регулятором громкости
                    Rectangle { width: 15; height:15; color: "#4C566A"}  // Невидимый разделитель
                    Label { 
                        text: "Vol:" 
                        color: "white" 
                        anchors.verticalCenter: parent.verticalCenter 
                        rightPadding: 5  // Небольшой отступ перед прямоугольником
                    }
            
                    // Круговой регулятор громкости
                    Dial {
                        id: volumeDial
                        width: 35
                        height: 35
                        from: 0
                        to: 100
                        value: viewModel.volume
                        anchors.verticalCenter: parent.verticalCenter
                        onValueChanged: viewModel.setVolume(value)
                        

                        handle: null

                        // Определяем углы для фиксированного закрашивания
                        readonly property real fixedStartAngle: 130  // начальный угол закрашивания
                        readonly property real fixedEndAngle: 270   // конечный угол закрашивания

                        // Обработка колесика мыши
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onWheel: {
                                if (wheel.angleDelta.y > 0) {
                                    // Прокрутка вверх - увеличиваем значение
                                    volumeDial.value = Math.min(volumeDial.to, volumeDial.value + 5);
                                } else {
                                    // Прокрутка вниз - уменьшаем значение
                                    volumeDial.value = Math.max(volumeDial.from, volumeDial.value - 5);
                                }
                                wheel.accepted = true;
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
                                rotation: volumeDial.angle
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
                                    centerX: volumeDial.width / 2
                                    centerY: volumeDial.height / 2
                                    radiusX: volumeDial.width / 2 - 1
                                    radiusY: volumeDial.height / 2 - 1
                                    // Фиксированный начальный угол (120°)
                                    startAngle: volumeDial.fixedStartAngle
                                    // Закрашиваем до фиксированного конечного угла (270°)
                                    sweepAngle: volumeDial.fixedEndAngle - volumeDial.fixedStartAngle+volumeDial.angle
                                }
                            }
                        }
                    }
                }
            }                    
            radius: 8
            border.color: "white"
            border.width: 1                    
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
                    SplitView.minimumWidth: 150
                    SplitView.preferredWidth: 200
                    SplitView.maximumWidth: 250
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
                                                        spacing: 6  // Увеличил промежуток между кнопками
                                                    
                                                        // Кнопка 1 - Аудио
                                                        Button {
                                                            text: "AUDIO"
                                                            width: 40  // Увеличил ширину
                                                            height: 40  // Вернул стандартную высоту
                                                            ToolTip.visible: hovered
                                                            ToolTip.delay: 500
                                                            ToolTip.text: "Добавить AUDIO дорожку"
                                                            font {
                                                                family: "Tahoma"
                                                                pixelSize: 11  // Увеличил шрифт
                                                            }
                                                            leftPadding: 0
                                                            rightPadding: 0
                                                        
                                                            background: Rectangle {
                                                                radius: 3
                                                                color: parent.down ? "#B8C1FC" : (parent.hovered ? "#D8DDFC" : "#CCD2FC")
                                                                border.color: "#8293FC"
                                                                border.width: 1
                                                            }

                                                            contentItem: Text {
                                                                text: parent.text
                                                                font: parent.font
                                                                color: "#5153FF"
                                                                horizontalAlignment: Text.AlignHCenter
                                                                verticalAlignment: Text.AlignVCenter
                                                                anchors.fill: parent
                                                            }

                                                            onClicked: viewModel.addAudioTrack()
                                                        }

                                                        // Кнопка 2 - MIDI
                                                        Button {
                                                            text: "MIDI"
                                                            width: 40
                                                            height: 40
                                                            ToolTip.visible: hovered
                                                            ToolTip.delay: 500
                                                            ToolTip.text: "Добавить MIDI дорожку"
                                                            font {
                                                                family: "Tahoma"
                                                                pixelSize: 12  // Увеличил шрифт
                                                            }
                                                            leftPadding: 0
                                                            rightPadding: 0
                                                        
                                                            background: Rectangle {
                                                                radius: 3
                                                                color: parent.down ? "#B8C1FC" : (parent.hovered ? "#D8DDFC" : "#CCD2FC")
                                                                border.color: "#8293FC"
                                                                border.width: 1
                                                            }

                                                            contentItem: Text {
                                                                text: parent.text
                                                                font: parent.font
                                                                color: "#5153FF"
                                                                horizontalAlignment: Text.AlignHCenter
                                                                verticalAlignment: Text.AlignVCenter
                                                                anchors.fill: parent
                                                            }

                                                            onClicked: viewModel.addMidiTrack()
                                                        }

                                                        // Кнопка 3 - SAMPLER (полный текст)
                                                        Button {
                                                            text: "SMPLR"  // Оптимальное сокращение
                                                            width: 40
                                                            height: 40
                                                            ToolTip.visible: hovered
                                                            ToolTip.delay: 500
                                                            ToolTip.text: "Добавить SAMPLER дорожку"
                                                            font {
                                                                family: "Tahoma"
                                                                pixelSize: 11  // Увеличил шрифт
                                                            }
                                                            leftPadding: 0
                                                            rightPadding: 0
                                                        
                                                            background: Rectangle {
                                                                radius: 3
                                                                color: parent.down ? "#B8C1FC" : (parent.hovered ? "#D8DDFC" : "#CCD2FC")
                                                                border.color: "#8293FC"
                                                                border.width: 1
                                                            }

                                                            contentItem: Text {
                                                                text: parent.text
                                                                font: parent.font
                                                                color: "#5153FF"
                                                                horizontalAlignment: Text.AlignHCenter
                                                                verticalAlignment: Text.AlignVCenter
                                                                anchors.fill: parent
                                                            }

                                                            onClicked: viewModel.addSamplerTrack()
                                                        }
                                                    }
                                                }

                                                Repeater {
                                                    model: viewModel.trackModel
                                                    Rectangle {
                                                        width: 150
                                                        height: 52
                                                        color: "#2D2D2D"
                                                        border.color: "#444"
                                                        // Свойство для хранения предыдущего значения громкости
                                                        property real lastVolume: model.volume !== undefined ? model.volume : 30
                                                        // Контекстное меню для удаления
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
                                                            width: 130  // Минимальная ширина под текст
                                                            topPadding: 2
                                                            bottomPadding: 2          
                                                            
                                                        
                                                            delegate: MenuItem {
                                                                id: menuItem
                                                                implicitHeight: 10  // Минимальная высота
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
                                                    
                                                        // Dial для управления громкостью своей дорожки
                                                        Dial {
                                                            id: trackVolumeDial
                                                            width: 35
                                                            height: 35
                                                            from: 0
                                                            to: 100
                                                            value: model.volume !== undefined ? model.volume : 30  // Если в модели нет volume, ставим 30
                                                            anchors.left: parent.left
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            anchors.leftMargin: 25
                                                            onValueChanged: {
                                                                if (value !== undefined) {
                                                                    // Обновляем последнее значение громкости, если не в режиме mute
                                                                    if (value > 0) {
                                                                        trackContainer.lastVolume = value
                                                                    }
                                                                    viewModel.setTrackVolume(index, value)
                                                                
                                                                    // Если громкость стала больше 0, автоматически выключаем mute
                                                                    if (value > 0 && model.muted) {
                                                                        viewModel.setTrackMute(index, false)
                                                                    }
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
                                                                        trackVolumeDial.value = Math.min(trackVolumeDial.to, trackVolumeDial.value + 5);
                                                                    } else {
                                                                        trackVolumeDial.value = Math.max(trackVolumeDial.from, trackVolumeDial.value - 5);
                                                                    }
                                                                    wheel.accepted = true;
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
                                                    
                                                        // Колонка с меткой и кнопками справа
                                                        Column {
                                                            anchors.left: trackVolumeDial.right
                                                            anchors.right: parent.right
                                                            anchors.top: parent.top
                                                            anchors.bottom: parent.bottom
                                                            anchors.leftMargin: 4
                                                        
                                                            Label {
                                                                width: parent.width
                                                                horizontalAlignment: Text.AlignHCenter
                                                                text: (index + 1) + " (" + model.trackType + ")"
                                                                color: "#CCC"
                                                                font.pixelSize: 12
                                                                elide: Text.ElideRight
                                                            }
                                                        
                                                            Row {
                                                                anchors.horizontalCenter: parent.horizontalCenter
                                                                spacing: 2
                                                            
                                                                // Кнопка Mute/Unmute
                                                                RoundButton {
                                                                    id: muteButton
                                                                    width: 30
                                                                    height: 30
                                                                    radius: width / 2
                                                                    ToolTip.visible: hovered
                                                                    ToolTip.delay: 500
                                                                    ToolTip.text: (trackVolumeDial.value === 0) ? "Unmute" : "Mute"
                                                                
                                                                    // Состояние кнопки зависит от значения Dial
                                                                    property bool isMuted: trackVolumeDial.value === 0
                                                                
                                                                    background: Rectangle {
                                                                        radius: parent.radius
                                                                        color: muteButton.hovered ? "#d0d0d0" : "transparent"
                                                                        border.color: muteButton.hovered ? "#a0a0a0" : "transparent"
                                                                        border.width: 1
                                                                        Behavior on color { ColorAnimation { duration: 100 } }
                                                                        Behavior on border.color { ColorAnimation { duration: 100 } }
                                                                    }

                                                                    Image {
                                                                        anchors.centerIn: parent
                                                                        width: 18
                                                                        height: 18
                                                                        source: muteButton.isMuted ? imagesPath + "muted.png" : imagesPath + "unmuted.png"
                                                                        sourceSize.width: 18
                                                                        sourceSize.height: 18
                                                                        opacity: muteButton.down ? 0.7 : 1.0
                                                                        fillMode: Image.PreserveAspectFit
                                                                        Behavior on opacity { NumberAnimation { duration: 100 } }
                                                                    }

                                                                    onClicked: {
                                                                        muteButton.scale = 0.95
                                                                    
                                                                        if (trackVolumeDial.value > 0) {
                                                                            // Сохраняем текущую громкость и устанавливаем 0
                                                                            trackContainer.lastVolume = trackVolumeDial.value
                                                                            trackVolumeDial.value = 0
                                                                            viewModel.setTrackMute(index, true)
                                                                        } else {
                                                                            // Восстанавливаем последнюю громкость
                                                                            trackVolumeDial.value = trackContainer.lastVolume
                                                                            viewModel.setTrackMute(index, false)
                                                                        }
                                                                    }
                                                                
                                                                    Behavior on scale {
                                                                        NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                                                                    }
                                                                }

                                                                // Кнопка Solo
                                                                RoundButton {
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
                                                                    
                                                                        Behavior on color {
                                                                            ColorAnimation { duration: 100 }
                                                                        }
                                                                        Behavior on border.color {
                                                                            ColorAnimation { duration: 100 }
                                                                        }
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
                                                                    
                                                                        Behavior on opacity {
                                                                            NumberAnimation { duration: 100 }
                                                                        }
                                                                    }

                                                                    onClicked: {
                                                                        soloButton.scale = 0.95
                                                                        viewModel.toggleSolo(index)
                                                                    }
                                                                
                                                                    Behavior on scale {
                                                                        NumberAnimation { 
                                                                            duration: 100
                                                                            easing.type: Easing.OutQuad 
                                                                        }
                                                                    }
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
                                                                                        if (!model.file) return "MIDI Clip";
                                                                                        var parts = model.file.split(/[\\/]/);
                                                                                        return parts[parts.length - 1];
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

                                                                        /*  Image {
                                                                                id: waveformImage
                                                                                anchors.fill: parent
                                                                                source: ""
                                                                                asynchronous: true
                                                                                cache: false
                                                                                visible: model.type === "audio" && source != ""
                                                                            } */

                                                                        /*  Timer {
                                                                                id: imageUpdateTimer
                                                                                interval: 1000
                                                                                running: clipItem.visible && model.type === "audio" && waveformImage.source == "" && !flickableArea.moving
                                                                                onTriggered: {
                                                                                    if (clipItem.visible) {
                                                                                        waveformImage.source = clipsModel.getWaveformImage(index, Math.round(clipRectangle.width), Math.round(clipRectangle.height))
                                                                                        console.log("Waveform updated via timer for clip:", index, "width:", clipRectangle.width, "source:", waveformImage.source)
                                                                                    }
                                                                                }
                                                                            } */

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

                                                                            /* Label {
                                                                                anchors.fill: parent
                                                                                text: model.file ? model.file.split("/").pop() : "MIDI Clip"
                                                                                color: "white"
                                                                                font.pixelSize: 10
                                                                                padding: 5
                                                                                elide: Text.ElideRight
                                                                                verticalAlignment: Text.AlignVCenter
                                                                                opacity: model.type === "audio" ? 0.5 : 1.0
                                                                                visible: !waveformImage.visible
                                                                            } */


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
                                                                                    if (model.type === "midi") {
                                                                                       
                                                                                        mainWindow.selectedClipIndex = index
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
                                                                                // clipsModel.setData(index, "startBeats", newPosition)
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
                                }

                            PianoView {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 300
                                visible: pianoRollVisible
                                trackIndex: selectedTrackIndex
                                clipIndex: selectedClipIndex
                                clipDuration: viewModel.midiModel.clipDuration

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

                    Rectangle {
                        id: separatorPanel
                        Layout.fillWidth: true
                        height: 60
                        color: "#2D2D2D"
                        border.color: "#444"
                        border.width: 1
                        anchors.left: rightSplitView.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        z: 4
                        visible: separatorVisible

                        // Горизонтальная линия сверху
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#666"
                            anchors.top: parent.top
                        }

                        // Горизонтальная линия снизу
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: "#666"
                            anchors.bottom: parent.bottom
                        }

                        // Прокручиваемая область для кнопок
                        Flickable {
                            id: buttonsFlickable
                            anchors.fill: parent
                            contentWidth: buttonsRow.width
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.HorizontalFlick

                            // Горизонтальный ряд кнопок
                            Row {
                                id: buttonsRow
                                height: parent.height
                                spacing: 10
                                leftPadding: 10  // Добавляем отступ слева для первого элемента
                                anchors.verticalCenter: parent.verticalCenter

                                Repeater {
                                    model: viewModel.pluginModel
                                    onCountChanged: console.log("Repeater count changed to:", count)

                                    // Контейнер для группы кнопок (FX контейнер)
                                    Rectangle {
                                        id: fxContainer
                                        width: 150  // Ширина контейнера
                                        height: 50  // Уменьшили высоту до 50
                                        radius: 3
                                        color: "#4C566A"
                                        border.color: "#ECEFF4"
                                        border.width: 1
                                        anchors.verticalCenter: parent.verticalCenter

                                        // Label сверху
                                        Text {
                                            id: fxLabel
                                            text: model.name || "FX " + (index + 1)
                                            color: "#ECEFF4"
                                            font {
                                                family: "Tahoma"
                                                pixelSize: 10
                                            }
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.top: parent.top
                                            anchors.topMargin: 2
                                        }

                                        // Контейнер для кнопок внизу
                                        Item {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            width: audioButton.width + minusButton.width + 4  // Ширина обеих кнопок + отступ
                                            height: audioButton.height
                                            anchors.bottom: parent.bottom
                                            anchors.bottomMargin: 1  // Отступ от нижней границы

                                            // Прямоугольная AUDIO кнопка с надписью Open
                                            Button {
                                                id: audioButton
                                                text: "Open"
                                                width: 40
                                                height: 38  // Уменьшенная высота кнопки
                                                anchors.left: parent.left
                                                ToolTip.visible: hovered
                                                ToolTip.delay: 500
                                                ToolTip.text: "Открыть " + (index + 1)
                                                font {
                                                    family: "Tahoma"
                                                    pixelSize: 11
                                                }
                                                leftPadding: 0
                                                rightPadding: 0
                        
                                                background: Rectangle {
                                                    radius: 3
                                                    color: parent.down ? "#B8C1FC" : (parent.hovered ? "#D8DDFC" : "#CCD2FC")
                                                    border.color: "#8293FC"
                                                    border.width: 1
                                                }

                                                contentItem: Text {
                                                    text: parent.text
                                                    font: parent.font
                                                    color: "#5153FF"
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                    anchors.fill: parent
                                                }

                                                onClicked: {
                                                    if (model.trackIndex >= 0 && model.pluginIndex >= 0) {
                                                        viewModel.openPluginEditor(model.trackIndex, model.pluginIndex);
                                                        console.log("Opening plugin editor: trackIndex=", model.trackIndex, "pluginIndex=", model.pluginIndex);
                                                    }
                                                }
                                            }

                                            // Круглая кнопка (с иконкой minus)
                                            RoundButton {
                                                id: minusButton
                                                width: 30
                                                height: 30
                                                radius: width / 2
                                                anchors.left: audioButton.right
                                              //  anchors.leftMargin: 4
                                                anchors.verticalCenter: parent.verticalCenter
                                                ToolTip.visible: hovered
                                                ToolTip.delay: 500
                                                ToolTip.text: "Закрыть FX " + (index + 1)

                                                background: Rectangle {
                                                    radius: parent.radius
                                                    color: parent.hovered ? "#d0d0d0" : "transparent"
                                                    border.color: parent.hovered ? "#a0a0a0" : "transparent"
                                                    border.width: 1
                                                }

                                                Image {
                                                    anchors.centerIn: parent
                                                    width: 16
                                                    height: 16
                                                    source: imagesPath + "minus.png"
                                                    sourceSize.width: 16
                                                    sourceSize.height: 16
                                                    opacity: parent.down ? 0.7 : 1.0
                                                    fillMode: Image.PreserveAspectFit
                                                }

                                                onClicked: {
                                                    if (model.trackIndex >= 0 && model.pluginIndex >= 0) {
                                                        viewModel.deletePlugin(model.trackIndex, model.pluginIndex);
                                                        viewModel.pluginModel.refresh();
                                                        console.log("Deleted plugin: trackIndex=", model.trackIndex, "pluginIndex=", model.pluginIndex);
                                                    }
                                                }
                                            }

                                            RoundButton {
                                                id: hideButton
                                                width: 30
                                                height: 30
                                                radius: width / 2
                                                anchors.left: audioButton.right
                                                anchors.leftMargin: 40
                                                anchors.verticalCenter: parent.verticalCenter
                                                ToolTip.visible: hovered
                                                ToolTip.delay: 500
                                                ToolTip.text: "Скрыть FX " + (index + 1)

                                                background: Rectangle {
                                                    radius: parent.radius
                                                    color: parent.hovered ? "#d0d0d0" : "transparent"
                                                    border.color: parent.hovered ? "#a0a0a0" : "transparent"
                                                    border.width: 1
                                                }

                                                Image {
                                                    anchors.centerIn: parent
                                                    width: 16
                                                    height: 16
                                                    source: imagesPath + "minus.png"
                                                    sourceSize.width: 16
                                                    sourceSize.height: 16
                                                    opacity: parent.down ? 0.7 : 1.0
                                                    fillMode: Image.PreserveAspectFit
                                                }

                                                onClicked: {
                                                    if (model.trackIndex >= 0 && model.pluginIndex >= 0) {
                                                        viewModel.HidePlugin(model.trackIndex, model.pluginIndex);
                                                        viewModel.pluginModel.refresh();
                                                        console.log("Deleted plugin: trackIndex=", model.trackIndex, "pluginIndex=", model.pluginIndex);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Индикатор прокрутки (если контент не помещается)
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: "#666"
                            visible: buttonsFlickable.contentWidth > buttonsFlickable.width

                            Rectangle {
                                width: (buttonsFlickable.width / buttonsFlickable.contentWidth) * parent.width
                                height: parent.height
                                x: (buttonsFlickable.contentX / buttonsFlickable.contentWidth) * parent.width
                                color: "#ECEFF4"
                            }
                        }
                    }
                }
            }  
        }
    }
}