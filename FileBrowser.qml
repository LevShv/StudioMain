import Qt.labs.folderlistmodel 2.15
import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 300
    height: 400
      clip: true // Чтобы содержимое не выходило за границы
    
    Rectangle {
        anchors.fill: parent
        color: "#2E3440"
    
        // Публичные свойства для управления извне
        property string currentFolder: Qt.resolvedUrl("file://" + StandardPaths.standardLocations(StandardPaths.HomeLocation)[0])
        property alias folderModel: folderModel
    
        signal folderChanged(string newFolder)
    
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
                    onClicked: navigateUp()
                }
            
                Button {
                    text: "⌂"
                    onClicked: goHome()
                }
            
                TextField {
                    id: pathField
                    width: parent.width - 100
                    text: currentFolder
                    onAccepted: currentFolder = text
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
                        onClicked: handleClick(fileName, fileIsDir)
                    }
                }
            }
        }
    
        function navigateUp() {
            var dir = Qt.resolvedUrl(currentFolder + "/..")
            currentFolder = dir
        }
    
        function goHome() {
            currentFolder = Qt.resolvedUrl("file://" + StandardPaths.standardLocations(StandardPaths.HomeLocation)[0])
        }
    
        function handleClick(name, isDir) {
            if (isDir) {
                currentFolder = Qt.resolvedUrl(currentFolder + "/" + name)
            } else {
                Qt.openUrlExternally(Qt.resolvedUrl(currentFolder + "/" + name))
            }
        }
    
        FolderListModel {
            id: folderModel
            folder: currentFolder
            showDirsFirst: true
            showDotAndDotDot: true
        }
    
        onCurrentFolderChanged: {
            folderModel.folder = currentFolder
            folderChanged(currentFolder)
        }
    }
}