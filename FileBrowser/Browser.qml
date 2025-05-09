import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.folderlistmodel 2.15
import FileBrowser 1.0

Item {
    id: root

    property Item dragParent: null
    
    property alias currentFolder: browser.currentFolder
    
    width: 200
    height: 400

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

        // Панель навигации
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

            TextField {
                id: pathField
                width: parent.width - 100
                text: browser.currentFolder
                onAccepted: {
                    console.log("Manual path input:", text)
                    browser.setCurrentFolder(text)
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
                width: parent.width
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
                        browser.viewClick(folderModel.folder.toString(), fileName)
                    
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
                    property string filePath: ""

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
                        dragItem.parent = root.dragParent
                        dragItem.filePath = browser.getFilePathForDrag(fileName)
                        dragItem.x = mapToItem(root.dragParent, mouseX, mouseY).x
                        dragItem.y = mapToItem(root.dragParent, mouseX, mouseY).y
                        dragItem.visible = true
                    }

                    onPositionChanged: {
                        dragItem.x = mapToItem(root.dragParent, mouseX, mouseY).x
                        dragItem.y = mapToItem(root.dragParent, mouseX, mouseY).y
                    }

                    onReleased: {
                        dragItem.visible = false
                        dragItem.parent = parent
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