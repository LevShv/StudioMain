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

    // Сигнал для передачи пути к файлу и координат отпускания
    signal fileDropped(string filePath, real globalX, real globalY)

    width: 200
    height: 400

    Component.onCompleted: {
        console.log("Browser dragParent:", dragParent)
    }

    FileBrowser {
        id: browser
        onCurrentFolderChanged: {
            console.log("QML: Folder changed to", browser.currentFolder)
            folderModel.folder = "file://" + browser.currentFolder
            pathField.text = browser.currentFolder
        }
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
                    source: "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/left_arrow.png"
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

                ToolTip.visible: hovered
                ToolTip.text: "Домашняя папка"

                background: Rectangle {
                    color: "transparent"
                }

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/up_arrow.png"
                    sourceSize.width: 24
                    sourceSize.height: 24
                }

                onClicked: {
                    console.log("Navigating home")
                    var homefolder = browser.homeFolder()
                    browser.setCurrentFolder(homefolder)
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
                        case "*": return "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/filter.png";
                        case "": return "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/folder.png";
                        case "*.mp3": return "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/mp3.png";
                        case "*.wav": return "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/wav.png";
                        case "*.mp4": return "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/mp4.png";
                        default: return "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/filter.png";
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
                        icon.source: "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/filter.png";
                        onTriggered: root.currentFilter = "*"
                    }
                    MenuItem {
                        text: "Только папки"
                        icon.source: "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/folder.png"
                        onTriggered: root.currentFilter = ""
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: "MP3 аудио"
                        icon.source: "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/mp3.png"
                        onTriggered: root.currentFilter = "*.mp3"
                    }
                    MenuItem {
                        text: "WAV аудио"
                        icon.source: "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/wav.png"
                        onTriggered: root.currentFilter = "*.wav"
                    }
                    MenuItem {
                        text: "MP4 видео"
                        icon.source: "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/mp4.png"
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
                width: ListView.view ? ListView.view.width : root.width
                height: 30
                visible: fileIsDir || (root.currentFilter === "*") || 
                       (root.currentFilter !== "" && fileName.toLowerCase().endsWith(root.currentFilter.substring(1)))
                color: {
                    if (ListView.isCurrentItem) "#4C566A"
                    else if (root.selectedIndex === index) "#3B4252"
                    else if (dragArea.containsMouse) "#434C5E"
                    else "transparent"
                }
                
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
                        if (fileIsDir) "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/folder.png";
                        else if (fileName.endsWith(".mp3")) "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/mp3.png";
                        else if (fileName.endsWith(".wav")) "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/wav.png";
                        else if (fileName.endsWith(".mp4")) "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/mp4.png";
                        else "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/file.png";
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
                    width: 40
                    height: 40
                    visible: false
                    color: "#4C566A"
                    radius: 20
                    opacity: 0.9
                    z: 9999

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        source: {
                            if (fileName.endsWith(".mp3")) "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/mp3.png";
                            else if (fileName.endsWith(".wav")) "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/wav.png";
                            else if (fileName.endsWith(".mp4")) "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/mp4.png";
                            else "file:///C:/Users/user/source/repos/LevShv/StudioMain/images/file.png";
                        }
                        sourceSize.width: 24
                        sourceSize.height: 24
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
        nameFilters: root.currentFilter === "" ? [] : [root.currentFilter]
        onFolderChanged: console.log("Model folder updated:", folder)
    }
}