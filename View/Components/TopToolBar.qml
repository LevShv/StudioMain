import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Shapes 1.15
import QtQuick.Dialogs

Rectangle {
    id: topToolbar
    width: parent ? parent.width : 1500 
    height: 48
    color: "#4C566A"
    radius: 8
    border.color: "white"
    border.width: 1

    signal toggleSeparator()
    signal togglePianoRoll()
    property string renderFileName: ""
    
    FileDialog {
        id: loadDialog
        title: "Загрузить проект"
        nameFilters: ["ltproj файлы (*.ltproj)"]
        fileMode: FileDialog.OpenFile
        currentFolder: "file:///" + viewModel.applicationHomeFolder() + "/Saves"       
        onAccepted: {
            var filePath = loadDialog.selectedFile.toString()
            if (filePath.startsWith("file:///")) {
                filePath = filePath.substring(8)
            }
            console.log("Загрузка проекта из:", filePath)
            viewModel.OpenProject(filePath)     
            viewModel.setCurrentProjectPath(filePath);
        }
        onRejected: {
            console.log("Диалог загрузки отменен")
        }        
    }

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
            viewModel.SaveProject(filePath)
            viewModel.setCurrentProjectPath(filePath);
        }
        onRejected: {
            console.log("Диалог сохранения отменен")
        }
    }
    
    FileDialog {
        id: renderDialog
        title: "Рендер проекта"
        nameFilters: ["WAV файлы (*.wav)", "MP3 файлы (*.mp3)", "Все файлы (*)", "AIFF файлы (*.aiff)"]
        fileMode: FileDialog.SaveFile
        currentFolder: "file:///" + viewModel.applicationHomeFolder() + "/Result"
        defaultSuffix: "wav"
        onAccepted: {
            var filePath = renderDialog.selectedFile.toString()
            if (filePath.startsWith("file:///")) {
                filePath = filePath.substring(8)
            }
            var fileName = filePath.split('/').pop()
            topToolbar.renderFileName = fileName
            console.log("Рендеринг проекта в:", filePath)
            renderProgressPopup.open()
            viewModel.RenderToWave(filePath)
        }
        onRejected: {
            console.log("Диалог рендеринга отменен")
        }
    }
    RowLayout {
        anchors.fill: parent
        spacing: 10
        Row {
            spacing: 0
            leftPadding: 10

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
                        text: "Новый проект"
                        
                        onTriggered: { 
                            resetMainWindowParameters()
                            viewModel.createNewProject() 
                        }
                    }

                    MenuItem {
                        text: "Открыть проект..."                                
                        onTriggered: {
                            loadDialog.open()
                        }
                    }

                    MenuItem {
                        text: "Сохранить"
                        onTriggered: {
                            if (viewModel.currentProjectPath === "") {
                                saveDialog.open();
                                console.log("Путь к проекту пуст, открываем 'Сохранить как'");
                            } else {
                                viewModel.SaveProject(viewModel.currentProjectPath);
                                console.log("Сохранение в файл:", viewModel.currentProjectPath);
                            }
                        }
                    }

                    MenuItem {
                        text: "Сохранить как..."                                
                        onTriggered: {
                            saveDialog.open() 
                        }
                    }


                    MenuItem {
                        text: "Рендер..."                                
                        onTriggered: {
                            renderDialog.open() 
                        }
                    }

                    MenuItem {
                        text: "Выход"
                        onTriggered: {
                            Qt.quit()
                        }
                    }
                }
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
                            var helpFilePath = "file:///" + viewModel.applicationHomeFolder() + "/help.html"
                            console.log("Opening help file:", helpFilePath)
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
                            width: 300
                            height: 80
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                            Label {
                                anchors.centerIn: parent
                                text: "LeTo corporation:\nЛев Швецов - директор, backend-программист\nТо Хоанг Ньат Фонг - frontend-программист"
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
            Rectangle {
                width: 1
                height: 50
                color: "transparent"
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
                    Material.accent: "transparent"
                    Material.background: "transparent"
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10
                    currentIndex: 1

                    readonly property bool isValidIndices: selectedTrackIndex >= 0 && selectedClipIndex >= 0

                    background: Rectangle {
                        color: "transparent"
                    }

                    TabButton {
                        id: patButton
                        width: 35
                        height: 25
                        text: "CLIP"
                        enabled: modeTabBar.isValidIndices
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: modeTabBar.isValidIndices ? "Проигрывать только pattern" : "Выберите MIDI-клип для активации режима PAT"

                        font {
                            family: "Tahoma"
                            pixelSize: 11
                        }

                        background: Rectangle {
                            radius: 5
                            color: parent.checked ? "#8690FA" : (parent.hovered && parent.enabled ? "#D8DDFC" : "#CCD2FC")
                            border.color: "#8293FC"
                            border.width: 2
                            opacity: parent.enabled ? 1.0 : 0.5
                        }

                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: parent.checked ? "#FFFFFF" : (parent.enabled ? "#5153FF" : "#888888")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                        }
                    }

                    TabButton {
                        id: songButton
                        width: 35
                        height: 25
                        text: "SONG"
                        enabled: true 
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: "Проигрывать вместе с треками"

                        font {
                            family: "Tahoma"
                            pixelSize: 11
                        }

                        background: Rectangle {
                            radius: 5
                            color: parent.checked ? "#8690FA" : (parent.hovered ? "#D8DDFC" : "#CCD2FC")
                            border.color: "#8293FC"
                            border.width: 2
                        }

                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: parent.checked ? "#FFFFFF" : "#5153FF"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                        }
                    }
                    
                    onCurrentIndexChanged: {
                        if (currentIndex === 0) {
                            if (isValidIndices) {
                                console.log("Switching to PAT mode: trackIndex=" + selectedTrackIndex + ", clipIndex=" + selectedClipIndex)
                                viewModel.enableLoopMode(selectedTrackIndex, selectedClipIndex)
                            } else {
                                console.log("Cannot switch to PAT mode: trackIndex or clipIndex is invalid")
                                currentIndex = 1
                                ToolTip.show("Выберите MIDI-клип для активации режима PAT", 3000)
                            }
                        } else {
                            console.log("Switching to SONG mode")
                            viewModel.disableLoopMode()
                            
                        }
                    }

                    Connections {
                        target: modeTabBar
                        function onIsValidIndicesChanged() {
                            if (!isValidIndices && modeTabBar.currentIndex === 0) {
                                console.log("Invalid indices detected, switching back to SONG mode")
                                modeTabBar.currentIndex = 1 
                                engine.DisableLoopMode()
                                engine.PlayMix()
                                ToolTip.show("MIDI-клип не выбран, режим PAT отключен", 3000)
                            }
                        }
                    }
                }

                Row {
                    anchors.verticalCenter: parent.verticalCenter
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
                    RoundButton {
                        id: addButton
                        width: 40
                        height: 40
                        radius: width / 2
                        ToolTip.visible: addButton.hovered && !addMenu.visible
                        ToolTip.delay: 500
                        ToolTip.text: "Добавить"

                        background: Rectangle {
                            radius: parent.radius
                            color: addButton.hovered ? "#d0d0d0" : "transparent"
                            border.color: addButton.hovered ? "#a0a0a0" : "transparent"
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
                            source: imagesPath + "plus.png"
                            sourceSize.width: 24
                            sourceSize.height: 24
                            opacity: addButton.down ? 0.7 : 1.0
                            fillMode: Image.PreserveAspectFit

                            Behavior on opacity {
                                NumberAnimation { duration: 100 }
                            }
                        }

                        onClicked: {
                            addButton.scale = 0.95
                            addMenu.open()
                        }

                        Behavior on scale {
                            NumberAnimation {
                                duration: 100
                                easing.type: Easing.OutQuad
                            }
                        }

                        Menu {
                            id: addMenu
                            y: addButton.height
                            width: 210

                            enter: Transition {
                                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 150 }
                                NumberAnimation { property: "scale"; from: 0.9; to: 1.0; duration: 150 }
                            }

                            exit: Transition {
                                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 }
                            }

                            MenuItem {
                                text: "Добавить AUDIO дорожку"
                                onTriggered: {
                                    viewModel.addAudioTrack()
                                    console.log("Добавлена AUDIO дорожка")
                                }

                                background: Rectangle {
                                    implicitHeight: 30
                                    color: parent.hovered ? "#e0e0e0" : "transparent"

                                    Behavior on color {
                                        ColorAnimation { duration: 100 }
                                    }
                                }
                            }

                            MenuItem {
                                text: "Добавить MIDI дорожку"
                                onTriggered: {
                                    viewModel.addMidiTrack()
                                    console.log("Добавлена MIDI дорожка")
                                }

                                background: Rectangle {
                                    implicitHeight: 30
                                    color: parent.hovered ? "#e0e0e0" : "transparent"

                                    Behavior on color {
                                        ColorAnimation { duration: 100 }
                                    }
                                }
                            }

                            MenuItem {
                                text: "Добавить SAMPLER дорожку"
                                onTriggered: {
                                    viewModel.addSamplerTrack()

                                    console.log("Добавлена SAMPLER дорожка")
                                }

                                background: Rectangle {
                                    implicitHeight: 30
                                    color: parent.hovered ? "#e0e0e0" : "transparent"

                                    Behavior on color {
                                        ColorAnimation { duration: 100 }
                                    }
                                }
                            }
                        }
                    }                    
                }
            }
        }
        Row {
            Layout.alignment: Qt.AlignRight
            anchors.verticalCenter: parent.verticalCenter 
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
                anchors.leftMargin: 2

                background: Rectangle {
                    radius: parent.radius
                    color: {
                        if (!pianoRollButton.enabled) {
                            return "#606060" 
                        } else if (pianoRollButton.hovered) {
                            return "#d0d0d0"
                        } else {
                            return "transparent"
                        }
                    }
                    border.color: {
                        if (!pianoRollButton.enabled) {
                            return "transparent" 
                        } else if (pianoRollButton.hovered) {
                            return "#a0a0a0" 
                        } else {
                            return "transparent"
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
                    source: imagesPath + (mainWindow.pianoRollAutoOpen ? "piano_roll_1.png" : "piano_roll_0.png")
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
                        mainWindow.pianoRollAutoOpen = !mainWindow.pianoRollAutoOpen
                        if (!mainWindow.pianoRollAutoOpen) {
                            mainWindow.pianoRollVisible = false                           
                        }
                    }
                }

                Behavior on scale {
                    NumberAnimation { 
                        duration: 100
                        easing.type: Easing.OutQuad 
                    }
                }
            }

            RoundButton {
                id: colorButton
                width: 40
                height: 40
                radius: width / 2
                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: "Выбрать цвет дорожки"
                enabled: modeTabBar.isValidIndices

                background: Rectangle {
                    radius: parent.radius
                    color: {
                        if (!colorButton.enabled) {
                            return "#606060" 
                        } else if (colorButton.hovered) {
                            return "#d0d0d0"
                        } else {
                            return "transparent" 
                        }
                    }
                    border.color: {
                        if (!colorButton.enabled) {
                            return "transparent" 
                        } else if (colorButton.hovered) {
                            return "#ffffff" 
                        } else {
                            return "transparent" 
                        }
                    }
                    border.width: 2
                    Behavior on color { ColorAnimation { duration: 100 } }
                    Behavior on border.color { ColorAnimation { duration: 100 } }
                }

                Image {
                    id: colorIcon
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: imagesPath + "color.png"
                    sourceSize.width: 24
                    sourceSize.height: 24
                    opacity: colorButton.down ? 0.7 : (colorButton.enabled ? 1.0 : 0.5) 
                    fillMode: Image.PreserveAspectFit
                    Behavior on opacity { NumberAnimation { duration: 100 } }
                }
                property color currentColor: "#FFFFFF"

                onClicked: {
                    colorButton.scale = 0.95
                    colorDialog.open() 
                }

                Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
            }

            ColorDialog {
                id: colorDialog
                title: "Выберите цвет дорожки"
                modality: Qt.WindowModal
                onAccepted: {
                    colorButton.currentColor = selectedColor
                    viewModel.changeColor(selectedTrackIndex, selectedClipIndex, selectedColor) 
                    console.log("Color selected:", selectedColor, "for track:", selectedTrackIndex, "clip:", selectedClipIndex)
                }
                onRejected: {
                    console.log("Color selection canceled")
                }
            }
            
            Rectangle { width: 15; height:15; color: "#4C566A"}
    
            Label { 
                text: "BPM:" 
                color: "white" 
                anchors.verticalCenter: parent.verticalCenter 
                rightPadding: 5
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
                
                property real bpmValue: viewModel.bpm || 120
                onBpmValueChanged: {
                    if (bpmValue >= 60 && bpmValue <= 200) {
                        viewModel.setBpm(bpmValue)
                    } else {
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

            Rectangle { width: 15; height:15; color: "#4C566A"} 

            Label {
                text: "Vol:" 
                color: "white" 
                anchors.verticalCenter: parent.verticalCenter 
                rightPadding: 5
            }

            Dial {
                id: volumeDial
                width: 35
                height: 35
                from: 0
                to: 200
                value: viewModel.volume * 100
                anchors.verticalCenter: parent.verticalCenter
                onValueChanged: viewModel.setUserVolume(value / 100)
                handle: null
                
                readonly property real fixedStartAngle: 130
                readonly property real fixedEndAngle: 270
                
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onWheel: {
                        if (wheel.angleDelta.y > 0) {
                            volumeDial.value = Math.min(volumeDial.to, volumeDial.value + 5);
                        } else {
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
                            startAngle: volumeDial.fixedStartAngle
                            sweepAngle: volumeDial.fixedEndAngle - volumeDial.fixedStartAngle+volumeDial.angle
                        }
                    }
                }
            }
        }
    } 
    Popup {
        id: renderProgressPopup
        anchors.centerIn: Overlay.overlay
        width: 350
        height: 100
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: "#2E3440"
            radius: 8
            border.color: "white"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label {
                text: "Рендеринг " + topToolbar.renderFileName + "..."
                color: "white"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
            }

            ProgressBar {
                id: renderProgressBar
                Layout.fillWidth: true
                value: viewModel.renderProgress
                from: 0.0
                to: 1.0
                Material.accent: "#8690FA"

                background: Rectangle {
                    implicitHeight: 6
                    color: "#4C566A"
                    radius: 3
                }

                contentItem: Rectangle {
                    implicitHeight: 4
                    radius: 2
                    color: "#8690FA"
                    width: renderProgressBar.visualPosition * parent.width
                }
            }
        }
        onClosed: {
            topToolbar.renderFileName = ""
        }
    }
    Popup {
        id: successDialog
        anchors.centerIn: Overlay.overlay
        width: 350
        height: 100
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            color: "#2E3440"
            radius: 8
            border.color: "white"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label {
                text: "Рендеринг успешно завершен!"
                color: "white"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Button {
                    id: okButton
                    text: "ОК"
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: 38
                    height: 34
                    font {
                        family: "Tahoma"
                        pixelSize: 10
                    }

                    background: Rectangle {
                        radius: 3
                        color: okButton.down ? "#B8C1FC" : (okButton.hovered ? "#D8DDFC" : "#CCD2FC")
                        border.color: "#8293FC"
                        border.width: 1
                    }

                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#5153FF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        anchors.centerIn: parent
                    }

                    onClicked: successDialog.close()
                }
            }
        }
    }
    
    Connections {
        target: viewModel
        function onRenderFinished(success, errorMessage) {
            renderProgressPopup.close()
            if (success) {
                console.log("Рендеринг успешно завершен")
                successDialog.open()
            } else {
                console.log("Ошибка рендеринга:", errorMessage)
            }
        }
    }
}