import QtQuick 
import QtQuick.Controls 
import QtQuick.Layouts 
import Qt.labs.folderlistmodel 
import FileBrowser 

Item {
    id: root
    property Item dragParent: parent
    property alias currentFolder: browser.currentFolder
    property string selectedItem: ""
    property int selectedIndex: -2
    property string dragFilePath
    property string currentFilter: "*" // Текущий фильтр файлов
    property var supportedFormats: ["*.mp3", "*.wav", "*.mp4", "*.vst3"]

    property string imagesPath: "file:///" + browser.applicationHomeFolder() + "/images/"

    // Сигнал для передачи пути к файлу и координат отпускания
    signal fileDropped(string filePath, real globalX, real globalY)

    width: 200
    height: 400

    Component.onCompleted: {
        console.log("Browser dragParent:", dragParent)
        browser.setCurrentFolder(browser.applicationHomeFolder())
    }

    FileBrowser {
        id: browser
        onCurrentFolderChanged: {
            console.log("QML: Folder changed to", browser.currentFolder)
            folderModel.folder = "file://" + browser.currentFolder
            pathField.text = browser.currentFolder
        }
    }
    
    function isSupportedFile(fileName) {
        if (root.currentFilter === "*") {
            for (var i = 0; i < supportedFormats.length; i++) {
                if (fileName.toLowerCase().endsWith(supportedFormats[i].substring(1))) {
                    return true;
                }
            }
            return false;
        }
        return true;
    }

    Column {
        anchors.fill: parent
        spacing: 10

        Row {
            id: controlPanel
            width: parent.width
            padding: 5
            spacing: 5

            // 1. Кнопка назад
            Button {
                id: backButton
                width: 30
                height: 30
                ToolTip.visible: hovered
                ToolTip.text: "Назад"

                background: Rectangle {
                    color: "transparent"
                }

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: root.imagesPath + "left_arrow.png"
                    sourceSize.width: 24
                    sourceSize.height: 24
                }

                onClicked: {
                    var parentFolder = browser.parentFolder()
                    console.log("Navigating to parent:", parentFolder)
                    browser.setCurrentFolder(parentFolder)
                }
            }

            // 2. Кнопка домой
            Button {
                id: homeButton
                width: 30
                height: 30


                ToolTip.visible: hovered
                ToolTip.text: "Домашняя папка"

                background: Rectangle {
                    color: "transparent"
                }

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: root.imagesPath + "up_arrow.png";
                    sourceSize.width: 24
                    sourceSize.height: 24
                }
                onClicked: {
                    var homePath = browser.applicationHomeFolder();
                    console.log("Navigating to home folder:", homePath);
                    browser.setCurrentFolder(homePath);
                }
            }

            // 3. Кнопка фильтра с индикацией
            Button {
                id: filterButton
                width: 30
                height: 30

                ToolTip.visible: hovered
                ToolTip.text: {
                    switch(root.currentFilter) {
                    case "*": return "Все файлы";
                    case "": return "Только папки";
                    case "*.mp3": return "MP3 аудио";
                    case "*.wav": return "WAV аудио";
                    case "*.mp4": return "MP4 видео";
                    default: return "Фильтр: " + root.currentFilter;
                    }
                }
                background: Rectangle {
                    color: "transparent"
                }

                Image {
                    id: filterIcon
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: {
                        switch(root.currentFilter) {
                        case "*": return root.imagesPath + "filter.png";
                        case "": return root.imagesPath + "folder.png";
                        case "*.mp3": return root.imagesPath + "mp3.png";
                        case "*.wav": return root.imagesPath + "wav.png";
                        case "*.mp4": return root.imagesPath + "mp4.png";
                        default: return root.imagesPath + "filter.png";
                        }
                    }
                    sourceSize.width: 24
                    sourceSize.height: 24
                }

                onClicked: filterMenu.open()

                Menu {
                    id: filterMenu
                    y: filterButton.height

                    MenuItem {
                        text: "Все файлы"
                        icon.source: root.imagesPath + "filter.png";
                        onTriggered: root.currentFilter = "*"
                    }
                    MenuItem {
                        text: "Только папки"
                        icon.source: root.imagesPath + "folder.png";
                        onTriggered: root.currentFilter = ""
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: "MP3 аудио"
                        icon.source: root.imagesPath + "mp3.png";
                        onTriggered: root.currentFilter = "*.mp3"
                    }
                    MenuItem {
                        text: "WAV аудио"
                        icon.source: root.imagesPath + "wav.png";
                        onTriggered: root.currentFilter = "*.wav"
                    }
                    MenuItem {
                        text: "MP4 видео"
                        icon.source: root.imagesPath + "mp4.png";
                        onTriggered: root.currentFilter = "*.mp4"
                    }
                }
            }
        }

        ListView {
            id: listView
            width: parent.width
            height: parent.height - 50
            model: folderModel
            clip: true
            interactive: true

            delegate: Rectangle {
                id: delegateItem
                width: ListView.view.width
                height: 30
                visible: {
                    if (fileIsDir) {
                        return true; // Всегда показываем папки
                    } else {
                        if (root.currentFilter === "*") {
                            return isSupportedFile(fileName);
                        } else if (root.currentFilter === "") {
                            return false; // В режиме "Только папки" не показываем файлы
                        } else {
                            return fileName.toLowerCase().endsWith(root.currentFilter.substring(1));
                        }
                    }
                }
                color: ListView.isCurrentItem ? "#4C566A" :
                      root.selectedIndex === index ? "#3B4252" :
                      dragArea.containsMouse ? "#434C5E" : "transparent"
                
                Behavior on color {
                    ColorAnimation { duration: 150; easing.type: Easing.InOutQuad }
                }

                scale: dragArea.containsMouse ? 1.02 : 1.0
                Behavior on scale {
                    NumberAnimation { duration: 100; easing.type: Easing.OutBack }
                }

                z: dragArea.containsMouse ? 1 : 0
                Behavior on z { NumberAnimation { duration: 100 } }

                property bool isPressed: false
                transform: Scale {
                    origin.x: delegateItem.width/2
                    origin.y: delegateItem.height/2
                    xScale: isPressed ? 0.95 : 1.0
                    yScale: isPressed ? 0.95 : 1.0
                }
                Behavior on isPressed {
                    NumberAnimation { duration: 80; easing.type: Easing.OutQuad }
                }

                // Иконка элемента
                Image {
                    id: icon
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: 16
                    height: 16
                    source: {
                        if (fileIsDir) return root.imagesPath + "folder.png";
                        else if (fileName.endsWith(".mp3")) return root.imagesPath + "mp3.png";
                        else if (fileName.endsWith(".wav")) return root.imagesPath + "wav.png";
                        else if (fileName.endsWith(".mp4")) return root.imagesPath + "mp4.png";
                        else return root.imagesPath + "file.png";
                    }
                    sourceSize.width: 16
                    sourceSize.height: 16
                }

                // Название файла
                Text {
                    id: nameText
                    anchors {
                        left: icon.right
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        margins: 10
                    }
                    text: fileName
                    color: (ListView.isCurrentItem || root.selectedIndex === index) ? "white" : "#D8DEE9"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                // Элемент для перетаскивания
                Rectangle {
                    id: dragItem
                    width: Math.min(150, dragText.implicitWidth + 20)  // Автоподбор ширины с ограничением
                    height: dragText.implicitHeight + 10  // Автоподбор высоты
                    visible: false
                    color: "#4C566A"
                    radius: 6  // Более скругленные углы
                    opacity: 0.9
                    z: 9999
    
                    Text {
                        id: dragText
                        anchors.centerIn: parent
                        width: parent.width - 12  // Отступы от краев
                        text: fileName
                        color: "white"
                        font.pixelSize: 12  // Увеличенный размер текста
                        font.bold: true  // Полужирный шрифт
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideMiddle
                        wrapMode: Text.NoWrap  // Текст в одну строку
                    }
                }

                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true

                    onPressed: (mouse) => {
                        delegateItem.isPressed = true;
                        if (!fileIsDir) {
                            if (!dragItem.visible) {
                                root.dragFilePath = browser.NormalizePath(folderModel.folder + "/" + fileName)
                                listView.interactive = false; 
                                dragItem.parent = root.dragParent
                                var pos = mapToItem(root.dragParent, mouse.x, mouse.y)
                                dragItem.x = pos.x - dragItem.width / 2
                                dragItem.y = pos.y - dragItem.height / 2
                                dragItem.visible = true
                            }
                        }
                    }

                    onPositionChanged: (mouse) => {
                        if (!fileIsDir && dragItem.visible) {
                            var pos = mapToItem(root.dragParent, mouse.x, mouse.y)
                            dragItem.x = pos.x - dragItem.width / 2
                            dragItem.y = pos.y - dragItem.height / 2
                        }
                    }

                    onReleased: (mouse) => {
                        delegateItem.isPressed = false;
                        if (!fileIsDir && dragItem.visible) {
                            var globalPos = mapToItem(root.dragParent, mouse.x, mouse.y)
                            root.fileDropped(root.dragFilePath, globalPos.x, globalPos.y)
                            dragItem.visible = false
                            dragItem.parent = delegateItem
                            listView.interactive = true;
                        }
                    }

                    onClicked: {
                        delegateItem.isPressed = false;
                        if (fileIsDir) {
                            browser.setCurrentFolder(folderModel.folder + "/" + fileName)
                        } else {
                            browser.viewClick(folderModel.folder.toString(), fileName)
                        }
                    }

                    onDoubleClicked: {
                        if (fileIsDir) {
                            browser.setCurrentFolder(folderModel.folder + "/" + fileName)
                        }
                    }
                    onCanceled: delegateItem.isPressed = false;
                }
            }
        }
    }

    FolderListModel {
        id: folderModel
        folder: "file://" + browser.currentFolder
        showDirsFirst: true
        showDotAndDotDot: false
        nameFilters: {
            if (root.currentFilter === "") {
                return []; // Только папки
            } else if (root.currentFilter === "*") {
                return root.supportedFormats; // Все поддерживаемые форматы
            } else {
                return [root.currentFilter]; // Конкретный фильтр
            }
        }
    }
}