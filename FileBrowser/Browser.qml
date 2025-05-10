import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.folderlistmodel 2.15
import FileBrowser 1.0

Item {
    id: root
    property Item dragParent: parent
    property alias currentFolder: browser.currentFolder
    property string selectedItem: ""
    property int selectedIndex: -2
    property string dragFilePath: ""

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

        ListView {
            width: parent.width
            height: parent.height - 50
            model: folderModel
            clip: true

            delegate: Rectangle {
                id: delegateItem
                width: parent.width
                height: 40
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
                    font.pixelSize: 16
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
                    font.pixelSize: 14
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

                    Drag.active: dragArea.drag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    Drag.supportedActions: Qt.CopyAction
                }

                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    drag.target: !fileIsDir ? dragItem : null
                    drag.threshold: 10

                    onPressed: {
                        if (!fileIsDir) {
                            root.dragFilePath = browser.NormalizePath(filePath)
                            console.log("Preparing drag with path:", root.dragFilePath)

                            // Устанавливаем MIME-данные
                            dragItem.Drag.mimeData = { "text/plain": root.dragFilePath }
                            dragItem.Drag.keys = ["text/plain"]
                            dragItem.Drag.supportedActions = Qt.CopyAction

                            // Дополнительно логируем MIME-данные
                            console.log("MIME data set:", JSON.stringify(dragItem.Drag.mimeData))

                            // Перемещаем dragItem в dragParent
                            dragItem.parent = root.dragParent
                            var pos = mapToItem(root.dragParent, mouseX, mouseY)
                            dragItem.x = pos.x - dragItem.width / 2
                            dragItem.y = pos.y - dragItem.height / 2
                            dragItem.visible = true

                            // Запускаем перетаскивание
                            dragItem.Drag.start()
                        }
                    }

                    onPositionChanged: {
                        if (!fileIsDir && dragArea.drag.active) {
                            var pos = mapToItem(root.dragParent, mouseX, mouseY)
                            dragItem.x = pos.x - dragItem.Drag.hotSpot.x
                            dragItem.y = pos.y - dragItem.Drag.hotSpot.y
                        }
                    }

                    onReleased: {
                        if (!fileIsDir) {
                            console.log("Drag released")
                            dragItem.Drag.drop()
                            dragItem.visible = false
                            dragItem.parent = delegateItem
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
        showDotAndDotDot: true
        onFolderChanged: console.log("Model folder updated:", folder)
    }
}