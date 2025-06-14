import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Shapes 1.15
import QtQuick.Dialogs

Rectangle {
    id: topToolbar
    width: parent ? parent.width : 1500 // Fallback width if no parent
    height: 48
    color: "#4C566A"
    radius: 8
    border.color: "white"
    border.width: 1

    // Signals to communicate actions to the main window
    signal toggleSeparator()
    signal togglePianoRoll()
    // Свойство для хранения имени файла для рендеринга
    property string renderFileName: ""

    // Добавляем диалоговые окна для загрузки и сохранения
    FileDialog {
        id: loadDialog
        title: "Загрузить проект"
        nameFilters: ["ltproj файлы (*.ltproj)"]
        fileMode: FileDialog.OpenFile
        currentFolder: "file:///" + viewModel.applicationHomeFolder() + "/Saves"       
        onAccepted: {
            var filePath = loadDialog.selectedFile.toString()
            // Удаляем префикс "file:///" если он есть
            if (filePath.startsWith("file:///")) {
                filePath = filePath.substring(8)
            }
            console.log("Загрузка проекта из:", filePath)
            viewModel.OpenProject(filePath)     
            viewModel.setCurrentProjectPath(filePath); // Обновляем путь
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
        defaultSuffix: "ltproj" // Автоматически добавляет расширение .ltproj
        onAccepted: {
            var filePath = saveDialog.selectedFile.toString()
            // Удаляем префикс "file:///" если он есть
            if (filePath.startsWith("file:///")) {
                filePath = filePath.substring(8)
            }
            console.log("Сохранение проекта в:", filePath)
            viewModel.SaveProject(filePath)
            viewModel.setCurrentProjectPath(filePath); // Обновляем путь
        }
        onRejected: {
            console.log("Диалог сохранения отменен")
        }
    }
    
    FileDialog {
        id: renderDialog
        title: "Рендер проекта"
        nameFilters: ["WAV файлы (*.wav)", "MP3 файлы (*.mp3)", "Все файлы (*)"]
        fileMode: FileDialog.SaveFile
        currentFolder: "file:///" + viewModel.applicationHomeFolder() + "/Result"
        defaultSuffix: "wav" // По умолчанию сохраняем как .wav
        onAccepted: {
            var filePath = renderDialog.selectedFile.toString()
            if (filePath.startsWith("file:///")) {
                filePath = filePath.substring(8)
            }
            // Извлекаем имя файла из пути
            var fileName = filePath.split('/').pop()
            topToolbar.renderFileName = fileName
            console.log("Рендеринг проекта в:", filePath)
            viewModel.RenderToWave(filePath)
            renderProgressPopup.open() // Открываем прогресс-бар
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
                        text: "Новый проект"
                        
                        onTriggered: { 
                            resetMainWindowParameters()
                            viewModel.createNewProject() 
                        }
                    }

                    MenuItem {
                        text: "Загрузить..."                                
                        onTriggered: {
                            loadDialog.open() // Открываем диалог загрузки
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
                        text: "Рендер..."                                
                        onTriggered: {
                            renderDialog.open() // Открываем диалог рендеринга
                        }
                    }

                    MenuItem {
                        text: "Сохранить как..."                                
                        onTriggered: {
                            saveDialog.open() // Открываем диалог сохранения
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
                width: 1 // Явное расстояние между кнопками
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
                    currentIndex: 1 // По умолчанию SONG

                    // Вычисляемое свойство для проверки валидности индексов
                    readonly property bool isValidIndices: selectedTrackIndex >= 0 && selectedClipIndex >= 0

                    background: Rectangle {
                        color: "transparent"
                    }

                    TabButton {
                        id: patButton
                        width: 35
                        height: 25
                        text: "CLIP"
                        enabled: modeTabBar.isValidIndices // Отключаем, если индексы некорректны
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
                            opacity: parent.enabled ? 1.0 : 0.5 // Визуально показываем, что кнопка отключена
                        }

                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: parent.checked ? "#FFFFFF" : (parent.enabled ? "#5153FF" : "#888888") // Серый текст для отключенной кнопки
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
                        enabled: true // Кнопка SONG всегда активна
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

                    // Обработка смены вкладки
                    onCurrentIndexChanged: {
                        if (currentIndex === 0) { // PAT
                            if (isValidIndices) {
                                console.log("Switching to PAT mode: trackIndex=" + selectedTrackIndex + ", clipIndex=" + selectedClipIndex)
                                viewModel.enableLoopMode(selectedTrackIndex, selectedClipIndex)
                            } else {
                                console.log("Cannot switch to PAT mode: trackIndex or clipIndex is invalid")
                                currentIndex = 1 // Возвращаемся к SONG
                                ToolTip.show("Выберите MIDI-клип для активации режима PAT", 3000)
                            }
                        } else { // SONG
                            console.log("Switching to SONG mode")
                            viewModel.disableLoopMode()
                            
                        }
                    }

                    // Реакция на изменение trackIndex или clipIndex
                    Connections {
                        target: modeTabBar
                        function onIsValidIndicesChanged() {
                            if (!isValidIndices && modeTabBar.currentIndex === 0) {
                                console.log("Invalid indices detected, switching back to SONG mode")
                                modeTabBar.currentIndex = 1 // Переключаемся на SONG, если индексы стали некорректными
                                engine.DisableLoopMode()
                                engine.PlayMix()
                                ToolTip.show("MIDI-клип не выбран, режим PAT отключен", 3000)
                            }
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
                        ToolTip.visible: addButton.hovered && !addMenu.visible // Скрываем ToolTip, если меню открыто
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
                            mainWindow.pianoRollVisible = false // Закрываем PianoView сразу                            
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

                // Привязываем enabled к separatorVisible
                enabled: modeTabBar.isValidIndices // Кнопка активна только если separatorVisible == true

                background: Rectangle {
                    radius: parent.radius
                    color: {
                        if (!colorButton.enabled) {
                            return "#606060" // Серый цвет для отключённого состояния
                        } else if (colorButton.hovered) {
                            return "#d0d0d0" // Цвет при наведении
                        } else {
                            return "transparent" // Обычный цвет
                        }
                    }
                    border.color: {
                        if (!colorButton.enabled) {
                            return "transparent" // Без обводки для отключённого состояния
                        } else if (colorButton.hovered) {
                            return "#ffffff" // Белая обводка при наведении
                        } else {
                            return "transparent" // Обычное состояние
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
                    opacity: colorButton.down ? 0.7 : (colorButton.enabled ? 1.0 : 0.5) // Прозрачность для отключённого состояния
                    fillMode: Image.PreserveAspectFit
                    Behavior on opacity { NumberAnimation { duration: 100 } }
                }
                property color currentColor: "#FFFFFF"

                onClicked: {
                    colorButton.scale = 0.95
                    colorDialog.open() // Предполагаем, что colorDialog доступен (если ошибка сохраняется, используйте topToolbar.colorDialog)
                }

                Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
            }

            ColorDialog {
                id: colorDialog
                title: "Выберите цвет дорожки"
                modality: Qt.WindowModal
                onAccepted: {
                    colorButton.currentColor = selectedColor
                    viewModel.changeColor(selectedTrackIndex, selectedClipIndex, selectedColor) // Оставляем changeColor, как в предоставленном коде
                    console.log("Color selected:", selectedColor, "for track:", selectedTrackIndex, "clip:", selectedClipIndex)
                }
                onRejected: {
                    console.log("Color selection canceled")
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
                to: 200
                value: viewModel.volume * 100
                anchors.verticalCenter: parent.verticalCenter
                onValueChanged: viewModel.setUserVolume(value / 100)
                

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
    Popup {
        id: renderProgressPopup
        anchors.centerIn: Overlay.overlay // Центрируем по всему экрану
        width: 350
        height: 100
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#2E3440"
            radius: 8
            border.color: "white"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Label {
               text: "Рендеринг проекта " + topToolbar.renderFileName + "..."
                color: "white"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
            }

            ProgressBar {
                id: renderProgressBar
                Layout.fillWidth: true
                value: 0.0 // Пока статичное значение, для будущей привязки
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

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 200 }
        }
        onClosed: {
            topToolbar.renderFileName = "" // Сбрасываем имя файла при закрытии
        }
    }
}