import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.15
import QtQuick.Controls 
import QtQuick.Controls.Material
import "qrc:/FileBrowser"
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

    property var clipModel: [
        {track: 0, start: 0, length: 4, color: "#FF5722", name: "Audio 1"},
        {track: 1, start: 4, length: 8, color: "#4CAF50", name: "Audio 2"},
        {track: 2, start: 8, length: 4, color: "#2196F3", name: "Audio 3"}
    ]

    Connections {
    target: viewModel
    function onIsPlayingChanged() {
        if (viewModel.isPlaying) {
            greenlineAnimator.start()
        } else {
            greenlineAnimator.stop()
        }
    }
    
    function onPlayheadPositionChanged(position) {
        if (!greenlineMouseArea.drag.active) {
            greenline.x = position * 40
        }
    }
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
        
                                onClicked: {
                                    viewModel.togglePlayback()
                                    if (viewModel.isPlaying) {
                                        greenlineAnimator.resume()
                                    } else {
                                        greenlineAnimator.pause()
                                    }
                                }
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
                Rectangle {  // Добавляем контейнерный Rectangle
                    id: fileBrowserContainer
                    SplitView.minimumWidth: 200
                    SplitView.preferredWidth: 250
                    color: "transparent"  // Прозрачный фон
        
                    Browser {
                        width: parent.width
                        height: parent.height
                        dragParent: mainWindow.contentItem
                        onCurrentFolderChanged: console.log("Folder changed:", currentFolder)
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
                                    height: 50  // Высота заголовка времени
                                    color: "transparent"
                                }

                                Repeater {
                                    model: 10
                                    Rectangle {
                                        width: 100
                                        height: 50  // Фиксированная высота трека
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
                                contentHeight: 11 * 50  // 10 треков по 50px + заголовок 50px
                                clip: true
                                boundsBehavior: Flickable.StopAtBounds
                                flickableDirection: Flickable.HorizontalFlick

                                // Линейка времени (прокручиваемая часть)
                                Rectangle {
                                    id: timeRuler
                                    width: contentGrid.width
                                    height: 50  // Высота заголовка времени
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
                                            height: 50  // Высота ячейки трека
                                            color: "transparent"
                                            border.color: "#444"

                                            DropArea {
                                                anchors.fill: parent
                                                keys: ["text/uri-list", "text/plain"]

                                                onDropped: {
                                                    console.log("Dropped at track:", Math.floor(index/32), 
                                                              "position:", index%32,
                                                              "file:", drop.getDataAsString("text/plain"))
                                                    mainWindow.createClipRequested(Math.floor(index/32), index%32, drop.getDataAsString("text/plain"))
                                                }
                                            }
                                        }
                                    }
                                }

                                // Клипы
                                Repeater {
                                    model: mainWindow.clipModel

                                    delegate: Rectangle {
                                        id: clipDelegate
                                        x: modelData.start * 40
                                        y: modelData.track * 50 + timeRuler.height  // 50px на трек
                                        width: modelData.length * 40
                                        height: 48  // 48px с небольшим отступом
                                        color: modelData.color
                                        radius: 3
                                        border.width: 1
                                        border.color: Qt.darker(modelData.color, 1.2)

                                        Label {
                                            anchors.fill: parent
                                            text: modelData.name || "Clip"
                                            color: "white"
                                            font.pixelSize: 10
                                            padding: 5
                                            elide: Text.ElideRight
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            drag.target: parent
                                            drag.axis: Drag.XAndYAxis
                                            drag.minimumX: 0
                                            drag.maximumX: contentGrid.width - parent.width
                                            drag.minimumY: timeRuler.height
                                            drag.maximumY: timeRuler.height + (contentGrid.rows-1) * 50  // 50px на трек

                                            onPressed: clipDelegate.z = 1
                                            onReleased: {
                                                clipDelegate.z = 0
                                                // Привязка к сетке
                                                parent.x = Math.round(parent.x / 40) * 40
                                                parent.y = timeRuler.height + Math.round((parent.y - timeRuler.height) / 50) * 50
                                                // Обновляем модель
                                                mainWindow.updateClipPosition(index, parent.x/40, (parent.y-timeRuler.height)/50)
                                            }
                                        }
                                    }
                                }

                                // Зеленая линия воспроизведения
                                // Замените существующий Rectangle зеленой линии на этот код:
                                Rectangle {
                                        id: greenline
                                        width: 2
                                        height: parent.height
                                        color: "green"
                                        z: 10
                                        x: viewModel.playheadPosition * 40 // Привязываем к позиции из ViewModel
    
                                        // Добавляем MouseArea для перемещения
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            drag {
                                                target: greenline
                                                axis: Drag.XAxis
                                                minimumX: 0
                                                maximumX: contentGrid.width
                                            }
        
                                            onPressed: {
                                                // Приостанавливаем анимацию при ручном перемещении
                                                greenlineAnimator.pause()
                                            }
        
                                            onPositionChanged: {
                                                if (drag.active) {
                                                    // Обновляем позицию в ViewModel
                                                    var newPos = greenline.x / 40
                                                    viewModel.setPlayheadPosition(newPos)
                                                }
                                            }
        
                                            onReleased: {
                                                // Можно возобновить анимацию здесь, если нужно
                                                // greenlineAnimator.resume()
                                            }
                                        }
    
                                        // Аниматор для автоматического движения
                                        PropertyAnimation {
                                            id: greenlineAnimator
                                            target: greenline
                                            property: "x"
                                            from: 0
                                            to: contentGrid.width
                                            duration: 50000
                                            loops: Animation.Infinite
                                            running: viewModel.isPlaying
                                        }
    
                                        // Связь с ViewModel
                                        Connections {
                                            target: viewModel
                                            function onPlayheadPositionChanged() {
                                                if (!greenlineMouseArea.drag.active) {
                                                    greenline.x = viewModel.playheadPosition * 40
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
    }
}
