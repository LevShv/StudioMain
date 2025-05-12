import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.folderlistmodel 2.15

Rectangle {
    id: root
    color: "#2E3440"
    
    property alias currentFolder: fileBrowser.currentFolder
    
    FileBrowser {
        id: fileBrowser
    }

    FolderListModel {
        id: folderModel
        folder: "file://" + fileBrowser.currentFolder
        showDirsFirst: true
        showDotAndDotDot: true
        showOnlyReadable: true
        nameFilters: ["*"]
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 5
            Layout.rightMargin: 5
            Layout.topMargin: 5

            Button {
                text: "←"
                onClicked: currentFolder = fileBrowser.parentFolder()
                ToolTip.visible: hovered
                ToolTip.text: "Назад"
            }

            Button {
                text: "⌂"
                onClicked: currentFolder = fileBrowser.homeFolder
                ToolTip.visible: hovered
                ToolTip.text: "Домой"
            }

            TextField {
                id: pathField
                Layout.fillWidth: true
                text: currentFolder
                onAccepted: currentFolder = text
                color: "white"
                background: Rectangle {
                    color: "#4C566A"
                    radius: 3
                }
            }
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: 0

            TabButton { text: "ALL" }
            TabButton { text: "PROJECT" }
            TabButton { text: "PLUGINS" }
            TabButton { text: "LIBRARY" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Первая вкладка (ALL)
            ScrollView {
                ListView {
                    id: listView
                    model: folderModel
                    clip: true

                    delegate: ItemDelegate {
                        width: listView.width
                        height: 40

                        Row {
                            spacing: 10
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: 10

                           // Image {
                           //     source: fileIsDir ? "qrc:/folder-icon.png" : "qrc:/file-icon.png"
                           //   width: 24
                           //   height: 24
                           //   anchors.verticalCenter: parent.verticalCenter
                           //}

                            Text {
                                text: fileName
                                color: "white"
                                font.pixelSize: 14
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        background: Rectangle {
                            color: listView.currentIndex === index ? "#4C566A" : "transparent"
                        }

                        onClicked: {
                            if (fileIsDir) {
                                if (fileName === "..") {
                                    currentFolder = fileBrowser.parentFolder()
                                } else if (fileName === ".") {
                                    // Текущая директория - ничего не делаем
                                } else {
                                    currentFolder = currentFolder + "/" + fileName
                                }
                            } else {
                                fileBrowser.openFile(currentFolder + "/" + fileName)
                            }
                        }
                    }
                }
            }

            // Остальные вкладки могут быть пустыми или содержать другую логику
            Item {} // PROJECT
            Item {} // PLUGINS
            Item {} // LIBRARY
        }
    }
}