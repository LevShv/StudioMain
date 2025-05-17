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
            width: parent.width
            padding: 5
            spacing: 5

            Button {
                text: "←"
                onClicked: {
                    var parentFolder = browser.parentFolder()
                    console.log("Navigating to parent:", parentFolder)
                    browser.setCurrentFolder(parentFolder)
                }
            }

            Button {
                text: "⌂"
                onClicked: {
                    console.log("Navigating home")
                    var homefolder = browser.homeFolder()
                    browser.setCurrentFolder(homefolder)
                }
            }
            /* удаление отображение пути к текущей папке
            TextField {
                id: pathField
                width: parent.width - 100
                text: browser.currentFolder
                onAccepted: {
                    console.log("Manual path input:", text)
                    browser.setCurrentFolder(text)
                }
            }*/
        }

        ListView {
            id: listView
            width: parent.width
            height: parent.height - 50
            model: folderModel
            clip: true
            interactive: true /////

            delegate: Rectangle {
            id: delegateItem
            width: ListView.view ? ListView.view.width : root.width // Безопасная привязка
            height: 30
            color: {
                if (ListView.isCurrentItem) "#4C566A"
                else if (root.selectedIndex === index) "#3B4252"
                else "transparent"
            }

            Text {
                id: icon
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: fileIsDir ? "📁" : "📄"
                font.pixelSize: 12
            }

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

            Rectangle {
                id: dragItem
                width: 120
                height: 40
                visible: false
                color: "#4C566A"
                radius: 4
                opacity: 0.9
                z: 9999

                Text {
                    anchors.centerIn: parent
                    text: fileName
                    color: "white"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    width: parent.width - 10
                }
            }

            MouseArea {
                id: dragArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true

                onPressed: (mouse) => {
                    if (!fileIsDir) {
                        if (!dragItem.visible) {
                            console.log("Raw filePath:",fileUrl)

                            root.dragFilePath = browser.NormalizePath(folderModel.folder + "/" + fileName) //

                            console.log("Preparing drag with path:", root.dragFilePath)
                            listView.interactive = false; 
                            dragItem.parent = root.dragParent
                            var pos = mapToItem(root.dragParent, mouse.x, mouse.y)
                            dragItem.x = pos.x - dragItem.width / 2
                            dragItem.y = pos.y - dragItem.height / 2
                            dragItem.visible = true
                            console.log("Drag item positioned at:", dragItem.x, dragItem.y, "parent:", dragItem.parent)
                        } else {
                            console.warn("Drag item already in use, ignoring press")
                        }
                    }
                }

                onPositionChanged: (mouse) => {
                    if (!fileIsDir && dragItem.visible) {
                        var pos = mapToItem(root.dragParent, mouse.x, mouse.y)
                        dragItem.x = pos.x - dragItem.width / 2
                        dragItem.y = pos.y - dragItem.height / 2
                        console.log("Dragging at:", dragItem.x, dragItem.y)
                    }
                }

                onReleased: (mouse) => {
                    if (!fileIsDir && dragItem.visible) {
                        console.log("Drag released")
                        var globalPos = mapToItem(root.dragParent, mouse.x, mouse.y)
                        console.log("Emitting fileDropped with path:", root.dragFilePath, "at:", globalPos.x, globalPos.y)
                        root.fileDropped(root.dragFilePath, globalPos.x, globalPos.y)
                        dragItem.visible = false
                        dragItem.parent = delegateItem
                        dragItem.x = 0
                        dragItem.y = 0
                        listView.interactive = true;
                        console.log("Drag item reset: visible:", dragItem.visible, "parent:", dragItem.parent)
                    }
                }

                onClicked: {
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
            }

            states: State {
                name: "hovered"
                when: dragArea.containsMouse && !ListView.isCurrentItem && root.selectedIndex !== index
                PropertyChanges {
                    target: delegateItem
                    color: "#434C5E"
                }
            }
        }
        }
    }

    FolderListModel {
        id: folderModel
        folder: "file://" + browser.currentFolder
        showDirsFirst: true
        showDotAndDotDot: false // параметр отвечает за первые 2 папки с ".." и "."
        onFolderChanged: console.log("Model folder updated:", folder)
    }
}