import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.15
import QtQuick.Controls 
import QtQuick.Controls.Material
import FileBrowser 1.0
import Qt.labs.folderlistmodel 2.15

Window {

    id: mainWindow
    visible: true
    width: 1500
    height: 1080

    title: "StudioMain"
    color: "#2E3440"

    property Item dragParent: contentItem
    signal createClipRequested(int trackIndex, int position, string filePath)

    function handleCreateClip(trackIndex, position, filePath) {
        console.log("Creating clip:", trackIndex, position, filePath)
        // Здесь реализуйте создание клипа в вашей модели данных
    }

    Component.onCompleted: {
        createClipRequested.connect(handleCreateClip)
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

                // Верхняя секция (пустая)
                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    color: "#4C566A"
                }

                // Нижняя секция с элементами управления
                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    color: "#4C566A"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        // Левая группа кнопок
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

                        // Центральная группа кнопок
                        Row {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 5

                            ToolButton {
                                text: viewModel.isPlaying ? "⏸️" : "▶️"
                                implicitWidth: 60
                            }
                            ToolButton {
                                text: "⏺️"
                                implicitWidth: 60
                            }
                        }

                        // Правая группа элементов
                        Row {
                            Layout.alignment: Qt.AlignRight
                            spacing: 10


                            Slider {
                                width: 150
                                from: 0
                                to: 100
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

                // Левая панель (200px фиксированная, но адаптивная)
                Rectangle {
                    id: leftPanel
                    SplitView.minimumWidth: 150
                    SplitView.preferredWidth: 200
                    SplitView.maximumWidth: 300
                    color: "#2E3440"

                     // Создаем экземпляр нашего C++ класса
                    FileBrowser {
                        id: browser
                        onCurrentFolderChanged: {
                            console.log("QML: Folder changed to", browser.currentFolder);
                            folderModel.folder = "file://" + browser.currentFolder;
                            pathField.text = browser.currentFolder;
                        }
                    }

                    Column {
                        anchors.fill: parent
                        spacing: 10

                        // Панель навигации
                        Row {
                            width: parent.width
                            padding: 5
                            spacing: 5

                            Button {
                                text: "←"
                                onClicked: {
                                    var parentFolder = browser.parentFolder();
                                    console.log("Navigating to parent:", parentFolder);
                                    browser.setCurrentFolder(parentFolder);
                                }
                            }

                            Button {
                                text: "⌂"
                    
                                onClicked: {
                                    console.log("Navigating home");
                                    browser.setCurrentFolder(browser.homeFolder());
                                }
                            }

                            TextField {
                                id: pathField
                                width: parent.width - 100
                                text: browser.currentFolder
                                onAccepted: {
                                    console.log("Manual path input:", text);
                                    browser.setCurrentFolder(text);
                                }
                            }
                        }

                        // Список файлов
                        ListView {
                            width: parent.width
                            height: parent.height - 50
                            model: folderModel
                            clip: true

                            delegate: Rectangle {
                                width: 200
                                height: 40
                                color: ListView.isCurrentItem ? "#4C566A" : "transparent"

                                Row {
                                    spacing: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    leftPadding: 10

                                    Text {
                                        text: fileIsDir ? "📁" : "📄"
                                        font.pixelSize: 16
                                    }

                                    Text {
                                        text: fileName
                                        color: "white"
                                        font.pixelSize: 14
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        // Получаем текущую папку и очищаем от file://
                                        var currentFolder = folderModel.folder.toString();
                                        currentFolder = currentFolder.startsWith("file://") ? currentFolder.substring(7) : currentFolder;
        
                                        // Удаляем qrc:/ если есть
                                        currentFolder = currentFolder.replace("qrc:/", "");
        
                                        // Формируем полный путь (удаляем возможные двойные слеши)
                                        var fullPath = (currentFolder + "/" + fileName).replace(/\/+/g, "/");
        
                                        // Для Windows: заменяем / на \, но не добавляем в начало
                                        fullPath = fullPath.replace(/\//g, "\\");
                                        if (fullPath.startsWith("\\")) { 
                                            fullPath = fullPath.substring(1);
                                        }
       
                                        console.log("Navigating to:", fullPath);
        
                                        if (fileIsDir) {
                                            browser.setCurrentFolder(fullPath);
                                        } else {
                                            browser.openFile(fullPath);
                                        }
                                    }
                                }

                                // Визуальный элемент для перетаскивания
                                Rectangle {
                                    id: dragItem
                                    width: 100
                                    height: 40
                                    visible: false
                                    color: "#4C566A"
                                    radius: 5
                                    opacity: 0.8

                                    property string filePath: ""  // Добавляем кастомное свойство вместо source

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Файл"
                                        color: "white"
                                    }

                                    Drag.active: dragItem.visible
                                    Drag.hotSpot.x: width / 2
                                    Drag.hotSpot.y: height / 2
                                    Drag.mimeData: {
                                        "text/uri-list": "file://" + filePath,
                                        "text/plain": filePath
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !fileIsDir
                                    drag.target: dragItem

                                    onPressed: {
                                        // ✅ Устанавливаем родителя (dragParent — это contentItem окна)
                                        dragItem.parent = dragParent
                                        dragItem.filePath = browser.getFilePathForDrag(fileName)
                                        dragItem.x = mapToItem(dragParent, mouseX, mouseY).x
                                        dragItem.y = mapToItem(dragParent, mouseX, mouseY).y
                                        dragItem.visible = true
                                    }

                                    onPositionChanged: {
                                        dragItem.x = mapToItem(dragParent, mouseX, mouseY).x
                                        dragItem.y = mapToItem(dragParent, mouseX, mouseY).y
                                    }

                                    onReleased: {
                                        dragItem.visible = false
                                        dragItem.parent = parent // Возвращаем обратно в исходный parent
                                    }
                                }
                            }
                        }
                    }

                    FolderListModel {
                        id: folderModel
                        folder: "file://" + browser.currentFolder
                        showDirsFirst: true
                        showDotAndDotDot: true
                        onFolderChanged: console.log("Model folder updated:", folder)
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

                        // Основная область с вертикальным разделением
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 0

                            // Фиксированные заголовки треков (левая колонка)
                            Column {
                                id: trackHeaders
                                width: 100
                                Layout.fillHeight: true
                                Rectangle {
                                    width: 100
                                    height: 30
                                    color: "transparent"
                                }

                                Repeater {
                                    model: 10
                                    Rectangle {
                                        width: 100
                                        height: 30
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

                            // Прокручиваемая область (правая часть)
                            Flickable {
                                id: flickableArea
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                contentWidth: 32 * 40
                                contentHeight: 11 * 30
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds
                                flickableDirection: Flickable.HorizontalFlick

                                // Линейка времени (прокручиваемая часть)
                                Rectangle {
                                    id: timeRuler
                                    width: contentGrid.width
                                    height: 30
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

                                // Прокручиваемая сетка
                                Grid {
                                    id: contentGrid
                                    columns: 32
                                    rows: 10
                                    anchors.top: timeRuler.bottom
                                    width: 32 * 40

                                    Repeater {
                                        model: 32 * 10
                                        delegate: Rectangle {
                                            width: 40
                                            height: 30
                                            color: "transparent"
                                            border.color: "#444"

                                            // Добавляем DropArea в каждый элемент сетки
                                            DropArea {
                                                anchors.fill: parent
                                                keys: ["text/uri-list", "text/plain"]

                                                onDropped: {
                                                    console.log("Dropped file:", drop.getDataAsString("text/plain"))
                                                    mainWindow.createClipRequested(index, drop.getDataAsString("text/plain"))
                                                }
                                            }
                                        }
                                    }
                                }

                                // Клипы
                                // В ScrollView, где находятся треки и клипы, обновляем Repeater:
                                Repeater {
                                    model: [
                                        {track: 0, start: 0, length: 4, color: "#FF5722"},
                                        {track: 1, start: 4, length: 8, color: "#4CAF50"},
                                        {track: 2, start: 8, length: 4, color: "#2196F3"}
                                    ]

                                    delegate: Rectangle {
                                        id: clipDelegate
                                        x: modelData.start * 40
                                        y: modelData.track * 30 + timeRuler.height
                                        width: modelData.length * 40
                                        height: 28
                                        color: modelData.color
                                        radius: 3
                                        border.width: 1
                                        border.color: Qt.darker(modelData.color, 1.2)

                                        Label {
                                            anchors.centerIn: parent
                                            text: "Clip"
                                            color: "white"
                                            font.pixelSize: 10
                                        }

                                        // Удаляем старый MouseArea и заменяем на этот:
                                        MouseArea {
                                            anchors.fill: parent
                                            drag.target: parent
                                            drag.axis: Drag.XAndYAxis
                                            drag.minimumX: 0
                                            drag.maximumX: contentGrid.width - parent.width
                                            drag.minimumY: timeRuler.height
                                            drag.maximumY: timeRuler.height + (contentGrid.rows-1) * 30

                                            onPressed: {
                                                // Поднимаем клип над другими элементами
                                                clipDelegate.z = 1
                                            }

                                            onReleased: {
                                                // Возвращаем z-index
                                                clipDelegate.z = 0
                
                                                // Привязываем к сетке
                                                parent.x = Math.round(parent.x / 40) * 40
                                                parent.y = timeRuler.height + Math.round((parent.y - timeRuler.height) / 30) * 30
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

                                    PropertyAnimation on x {
                                        duration: 50000
                                        from: 0
                                        to: contentGrid.width
                                        loops: Animation.Infinite
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
