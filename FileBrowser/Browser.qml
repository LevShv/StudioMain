import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.folderlistmodel 2.15
import FileBrowser 1.0

Item {
    id: root
    property Item dragParent: root // Явно укажите родительский элемент
    
    property alias currentFolder: browser.currentFolder
    property string selectedItem: ""  // Хранит путь выбранного элемента
    property int selectedIndex: -2    // Хранит индекс выбранного элемента
    
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
                id: delegateItem
                width: parent.width
                height: 40

    
                // Цвет фона в зависимости от состояния
                color: {
                    if (ListView.isCurrentItem) "#4C566A"           // Активный элемент
                    else if (root.selectedIndex === index) "#3B4252" // Выбранный элемент
                    else "transparent"                              // Обычное состояние
                }
    
                // Иконка (папка/файл)
                Text {
                    id: icon
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: fileIsDir ? "📁" : "📄"
                    font.pixelSize: 16
                }
    
                // Имя файла/папки
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
    
                // Элемент для перетаскивания (видим только при перетаскивании)
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
                }
    
                // Основная MouseArea
                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton
                    drag.target: !fileIsDir ? dragItem : null  // Перетаскивание только для файлов
                    drag.threshold: 10
        
                    // При нажатии запоминаем позицию
                    onPressed: {
                        root.selectedIndex = index
                        root.selectedItem = filePath

                        dragItem.parent = root.dragParent
                        var pos = mapToItem(root.dragParent, mouseX, mouseY)
                        dragItem.x = pos.x - dragItem.Drag.hotSpot.x
                        dragItem.y = pos.y - dragItem.Drag.hotSpot.y
    
                    }
        
                    // При движении начинаем перетаскивание
                    onPositionChanged: {
                        if (!fileIsDir && drag.active) {
                            var pos = mapToItem(root.dragParent, mouseX, mouseY)
                            dragItem.x = pos.x - dragItem.Drag.hotSpot.x
                            dragItem.y = pos.y - dragItem.Drag.hotSpot.y
                            dragItem.visible = true
                        }
                    }
        
                    // При отпускании кнопки
                    onReleased: {
                        dragItem.visible = false
                        dragItem.parent = delegateItem
                    }
        
                    // Обработка клика
                    onClicked: {
                        if (fileIsDir) {
                            browser.setCurrentFolder(folderModel.folder + "/" + fileName)
                        } else {
                            browser.viewClick(folderModel.folder.toString(), fileName)
                        }
                    }
        
                    // Обработка двойного клика
                    onDoubleClicked: {
                        if (fileIsDir) {
                            browser.setCurrentFolder(folderModel.folder + "/" + fileName)
                        }
                    }
                }
    
                // Состояние при наведении
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