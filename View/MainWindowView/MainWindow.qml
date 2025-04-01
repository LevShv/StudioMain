import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.15
import QtQuick.Controls 
import QtQuick.Controls.Material

Window {
    Component.onCompleted: {
        Application.style = "Material" // Или "Material", "Universal", "Basic"
    }
    visible: true
    width: 1500
    height: 1080
    title: "StudioMain"
    
        SplitView {
        anchors.fill: parent
        anchors.topMargin: 102
        orientation: Qt.Horizontal

           // Левая панель (список треков)
           Rectangle {
               id: browser
               color: "#2E3440"
               Layout.minimumWidth: 150
               Layout.preferredWidth: 200
               SplitView.preferredWidth: 200

               ColumnLayout {
                   anchors.fill: parent
                   spacing: 10
                    RowLayout{
                        Label {
                            text: "ALL"
                            color: "white"
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 10
                        }
                        Label {
                            text: "PROJECT"
                            color: "white"
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 10
                        }
                        Label {
                            text: "PLUGINS"
                            color: "white"
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 10
                        }
                        Label {
                            text: "LIBRARY"
                            color: "white"
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 10
                        }
                    }
                   ListView {
                       id: trackList
                       Layout.fillWidth: true
                       Layout.fillHeight: true
                       model: ["📁", "📁", "📁", "📁"]
                       delegate: ItemDelegate {
                           text: modelData
                           width: parent.width
                           height: 40
                           background: Rectangle {
                               color: "#4C566A"
                           }
                           contentItem: Text {
                               text: parent.text
                               color: "white"
                               verticalAlignment: Text.AlignVCenter

                           }
                       }
                   }
               }
           }
           // ChanelRack (редактирование самих треков)
           Rectangle {
               id: chanelrack
               color: "#2E3440"
               Layout.minimumWidth: 150
               Layout.preferredWidth: 200
               SplitView.preferredWidth: 200

               ColumnLayout {
                   anchors.fill: parent
                   spacing: 10

                   Label {
                       text: "Треки"
                       color: "white"
                       font.bold: true
                       Layout.alignment: Qt.AlignHCenter
                       Layout.topMargin: 10
                   }

                   ScrollView {
                           Layout.fillWidth: true  // ← Заменяем anchors.fill
                           Layout.fillHeight: true
                           clip: true

                           GridView {
                               id: channelRack
                               anchors.fill: parent
                               cellWidth: 200 // Ширина одного канала
                               cellHeight: 100 // Высота одного канала
                               model: 16 // Количество каналов

                               delegate: Rectangle {
                                   width: channelRack.cellWidth - 5
                                   height: channelRack.cellHeight - 5
                                   color: index % 2 === 0 ? "#2E3440" : "#3B4252" // Чередование цветов
                                   border.color: "gray"
                                   radius: 5

                                   ColumnLayout {
                                       anchors.fill: parent
                                       spacing: 5
                                       anchors.margins: 5

                                       // Название канала
                                       Label {
                                           text: "Channel " + (index + 1)
                                           font.pixelSize: 14
                                           color: "white"
                                           Layout.alignment: Qt.AlignHCenter
                                       }

                                       // Кнопка Mute
                                       ToolButton {
                                           text: "Mute"
                                           Layout.alignment: Qt.AlignHCenter

                                       }

                                       // Кнопка Solo
                                       ToolButton {
                                           text: "Solo"
                                           Layout.alignment: Qt.AlignHCenter

                                           }
                                       }
                                   }
                               }
                           }

               }
           }
           //звуковые дорожки
           Rectangle {
                id: playlistRoot
                property color backgroundColor: "#1E1E1E"
                property color trackColor: "#2D2D2D"
                property color textColor: "#CCCCCC"
                property color highlightColor: "#3A3A3A"
                property color borderColor: "#444444"
                property int trackHeight: 30
                property int timeRulerHeight: 25
                property int trackHeaderWidth: 100
                property int beatWidth: 40
                property int beatsPerMeasure: 4
                property int totalMeasures: 16

                // Playlist data model would go here in a real implementation
                property var tracks: [
                    {name: "Track 1", clips: [{start: 0, length: 4, color: "#FF5722"}]},
                    {name: "Track 2", clips: [{start: 4, length: 8, color: "#4CAF50"}]},
                    {name: "Track 3", clips: [{start: 8, length: 4, color: "#2196F3"}]}
                ]


                Rectangle {
                    id: playlistContainer
                    anchors.fill: parent
                    anchors.leftMargin: 0
                    anchors.topMargin: 0
                    color: playlistRoot.backgroundColor
                    border.color: playlistRoot.borderColor
                    border.width: 1


                    // Time ruler (top header)
                    Rectangle {
                        id: timeRuler
                        width: parent.width - playlistRoot.trackHeaderWidth
                        height: playlistRoot.timeRulerHeight
                        color: playlistRoot.backgroundColor
                        anchors.top: parent.top
                        anchors.left: trackHeaders.right

                        Row {
                            spacing: 0
                            anchors.fill: parent

                            Repeater {
                                model: playlistRoot.totalMeasures * playlistRoot.beatsPerMeasure

                                Rectangle {
                                    width: playlistRoot.beatWidth
                                    height: parent.height
                                    color: "transparent"
                                    border.color: index % playlistRoot.beatsPerMeasure === 0 ? Qt.darker(playlistRoot.borderColor, 1.3) : playlistRoot.borderColor
                                    border.width: 1

                                    Text {
                                        text: index % playlistRoot.beatsPerMeasure === 0 ? Math.floor(index/playlistRoot.beatsPerMeasure) + 1 : ""
                                        color: playlistRoot.textColor
                                        font.pixelSize: 10
                                        anchors.centerIn: parent
                                    }
                                }
                            }
                        }
                    }

                    // Track headers (left side)
                    Column {
                        id: trackHeaders
                        width: playlistRoot.trackHeaderWidth
                        anchors.top: timeRuler.bottom
                        anchors.bottom: parent.bottom
                        spacing: 0

                        Repeater {
                            model: playlistRoot.tracks

                            Rectangle {
                                width: parent.width
                                height: playlistRoot.trackHeight
                                color: playlistRoot.trackColor
                                border.color: playlistRoot.borderColor
                                border.width: 1

                                Text {
                                    text: modelData.name
                                    color: playlistRoot.textColor
                                    font.pixelSize: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.leftMargin: 5
                                }

                                MouseArea {
                                    anchors.fill: parent

                                }
                            }
                        }
                    }

                    // Main playlist area (clips grid)
                    Flickable {
                        id: playlistArea
                        anchors.top: timeRuler.bottom
                        anchors.left: trackHeaders.right
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        clip: true
                        contentWidth: playlistRoot.totalMeasures * playlistRoot.beatsPerMeasure * playlistRoot.beatWidth
                        contentHeight: playlistRoot.tracks.length * playlistRoot.trackHeight


                            Rectangle {
                                x: 10
                                width: 2
                                height: 900
                                color: "green"

                                PropertyAnimation on x {
                                    duration: 50000  // 50 секунд
                                    to: 1420         // пока x не будет равно 250
                                    loops: Animation.Infinite   // бесконечная анимация
                                }
                            }


                        // Grid background
                        Grid {
                            columns: playlistRoot.totalMeasures * playlistRoot.beatsPerMeasure
                            rows: playlistRoot.tracks.length
                            spacing: 0

                            Repeater {
                                model: playlistRoot.totalMeasures * playlistRoot.beatsPerMeasure * playlistRoot.tracks.length

                                Rectangle {
                                    width: playlistRoot.beatWidth
                                    height: playlistRoot.trackHeight

                                    border.color: playlistRoot.borderColor
                                    border.width: 1
                                }
                            }
                        }

                        // Audio/MIDI clips
                        Repeater {
                            model: playlistRoot.tracks

                            Repeater {
                                model: modelData.clips

                                Rectangle {
                                    x: modelData.start * playlistRoot.beatWidth
                                    y: index * playlistRoot.trackHeight
                                    width: modelData.length * playlistRoot.beatWidth
                                    height: playlistRoot.trackHeight - 2
                                    color: modelData.color
                                    radius: 2
                                    border.color: Qt.darker(modelData.color, 1.3)
                                    border.width: 1

                                    Text {
                                        text: "Clip " + (index + 1)
                                        color: "white"
                                        font.pixelSize: 10
                                        anchors.centerIn: parent
                                        visible: width > 50
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        drag.target: parent
                                        drag.axis: Drag.XAxis
                                        drag.minimumX: 0
                                        drag.maximumX: playlistArea.contentWidth - parent.width


                                    }
                                }
                            }
                        }
                    }

                    // Scroll bars
                    ScrollBar {
                        id: verticalScroll
                        width: 12
                        anchors.top: timeRuler.bottom
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        policy: ScrollBar.AlwaysOn
                        orientation: Qt.Vertical
                        contentItem: Rectangle {
                            color: "#aa0000"
                            radius: width / 2
                        }
                    }

                    ScrollBar {
                        id: horizontalScroll
                        height: 12
                        anchors.left: trackHeaders.right
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        policy: ScrollBar.AlwaysOn
                        orientation: Qt.Horizontal
                        contentItem: Rectangle {
                            color: "#ff0000"
                            radius: height / 2
                        }
                    }
                }
            }
    }

    Rectangle {
        id: rectangle1//Верхнее меню (Tool bar)
        x: 0
        y: 0
        width: 1920
        height: 102
        color: "#2E3440"
        border.color: "#ffffff"
        ColumnLayout {
            width: parent.width  // ← Можно так, или просто убрать anchors
            height: parent.height
            spacing: 1
            Rectangle {
                id: rectangle2
                Layout.fillWidth: true
                Layout.preferredHeight: 48  // Фиксированная высота
                color: "#4C566A"

                RowLayout {
                    anchors.fill: parent
                    spacing: 10
                    anchors.leftMargin: 10
                }
            }
            Rectangle {
                id: rectangle3
                x: 0
                y: 0
                Layout.fillWidth: true
                height: 48
                color: "#4C566A"
                Layout.topMargin: 10  // ← Если внутри Layout, используй margin
                Layout.alignment: Qt.AlignTop  // ← Выравнивание по верхнему краю
                Slider {//громкость
                    id: volumeSlider
                    Layout.fillWidth: true
                    x: 1200
                    from: 0
                    to: 100
                    onMoved: {
                        viewModel.moveClip(0,0,value/10)
                    }
                }
                Slider {//громкость
                    id: volumeSlider2
                    Layout.fillWidth: true
                    x: 1500
                    from: 0
                    to: 100
                    onMoved: {
                        viewModel.setPlayheadPosition(value)
                    }
                }
                Rectangle {
                    id: mainbuttons //все кнопки play,stop,record
                    x: 638
                    y: 0
                    width: 192
                    Layout.fillWidth: true
                    height: 48
                    color: "#4C566A"
                    anchors.top:rectangle3.top

                    RowLayout{
                        anchors.fill: parent
                        spacing: 5
                    ToolButton {
                       id:play
                       text: viewModel.isPlaying ? "Pause" : "Play" // Текст кнопки зависит от состояния
                       onClicked: viewModel.togglePlayback() 
                           // contentItem: Text {
                           // text: "▶️"
                           // Layout.leftMargin: 15
                           // font.pixelSize: 16
                           // horizontalAlignment: Text.AlignHCenter // Выравнивание текста по центру
                           // verticalAlignment: Text.AlignVCenter
                           // elide: Text.ElideNone
                           // }
                       background: Rectangle {
                       color: parent.pressed ? "gray" : "#4C566A"
                       }
                    }
                    ToolButton {
                            id:record
                            contentItem: Text {
                            text: "⏺️"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter // Выравнивание текста по центру
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideNone
                            }
                             background: Rectangle {
                             color: parent.pressed ? "gray" : "#4C566A"
                                }
                    }
                   ToolButton {
                            id:pause
                            contentItem: Text {
                            text: "⏸️"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter // Выравнивание текста по центру
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideNone
                            }
                             background: Rectangle {
                             color: parent.pressed ? "gray" : "#4C566A"
                                }
                    }
                }

                }
                Rectangle {
                    id: rectangle4
                    x: 862
                    y: 0
                    width: 336
                    Layout.fillWidth: true
                    height: 48
                    color: "black"
                       Layout.topMargin: 10  // ← Если внутри Layout, используй margin
                       Layout.alignment: Qt.AlignTop  // ← Выравнивание по верхнему краю
                    Label {
                            anchors.centerIn: parent
                            text: "Main Content Area"
                        }
                }
                Rectangle {
                        id: checkbutton //все сохранения
                        x: 1
                        y: 0
                        width: 192
                        Layout.fillWidth: true
                        height: 48
                        color: "#4C566A"
                        Layout.topMargin: 10  // ← Если внутри Layout, используй margin
                        Layout.alignment: Qt.AlignTop  // ← Выравнивание по верхнему краю
                        RowLayout{
                        anchors.fill: parent
                        //horizontalCenter:mainbuttons
                        spacing: 5
                        ToolButton {
                                id: newfile
                                contentItem: Text {
                                text: "Создать новый"
                                Layout.leftMargin: 15
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter // Выравнивание текста по центру
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideNone
                                }
                                 background: Rectangle {
                                 color: parent.pressed ? "gray" : "#4C566A"
                                    }
                        }
                        ToolButton {
                                id:copy
                                contentItem: Text {
                                text: "Копировать"
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter // Выравнивание текста по центру
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideNone
                                }
                                 background: Rectangle {
                                 color: parent.pressed ? "gray" : "#4C566A"
                                    }
                        }
                       ToolButton {
                                id:checkpoint
                                contentItem: Text {
                                text: "Сохранить"
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter // Выравнивание текста по центру
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideNone
                                }
                                 background: Rectangle {
                                 color: parent.pressed ? "gray" : "#4C566A"
                                    }
                            }
                        }
                    }
            }

        }
    }
}
