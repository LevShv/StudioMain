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
               id: rightpanel2
               color: "#2E3440"
               Layout.minimumWidth: 150
               Layout.preferredWidth: 200
               SplitView.preferredWidth: 200

               Column {
                   anchors.fill: parent
                   spacing: 10

                   Label {
                       y:0
                       text: "Звуковая дорожка"
                       color: "white"
                       font.bold: true
                       Layout.alignment: Qt.AlignHCenter
                       Layout.topMargin: 10
                   }

                   Rectangle {
                           id: pianoRoll
                           y:20
                           width: rightpanel2.width
                           height: 950
                           color: "#2D2D2D"

                           // Контейнер для клавиш пианино
                           Rectangle {
                               id: pianoKeys
                               width: 100
                               height: parent.height
                               color: "#1E1E1E"

                               // Отображение клавиш пианино
                               Column {
                                   spacing: 1

                                   // Белые и черные клавиши
                                   Repeater {
                                       model: 80 // Количество клавиш на фортепиано
                                       delegate: Rectangle {
                                           width: pianoKeys.width
                                           height: pianoRoll.height / 88
                                           color: (index % 12 === 1 || index % 12 === 3 || index % 12 === 6 || index % 12 === 8 || index % 12 === 10) ? "#1E1E1E" : "#FFFFFF"
                                           border.color: "#000000"

                                           // Текст для обозначения нот (опционально)
                                           Text {
                                               text:pl
                                               color: parent.color === "#FFFFFF" ? "#000000" : "#FFFFFF"
                                               anchors.centerIn: parent
                                               font.pixelSize: 10
                                           }
                                       }
                                   }
                               }
                           }

                           // Контейнер для сетки нот
                           Rectangle {
                               id: noteGrid
                               width: parent.width - pianoKeys.width
                               height: parent.height
                               color: "transparent"
                               anchors.left: pianoKeys.right

                               // Сетка для нот
                               Grid {
                                   rows: 80
                                   columns: 16
                                   spacing: 1
                                   x:0
                                   y:0
                                   width: 1200
                                   height: 900
                                   Repeater {
                                       model: 88 * 16
                                       delegate: Rectangle {
                                           width: noteGrid.width / 16
                                           height: noteGrid.height / 88
                                           color: index % 16 === 0 ? "#3A3A3A" : "#2D2D2D"
                                           border.color: "#1E1E1E"
                                       }
                                   }
                               }
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
                x: 0
                y: 0
                Layout.fillWidth: true
                height: 48
                color: "#4C566A"
                anchors.top:parent
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
                    anchors.top:rectangle3
                    RowLayout{
                    anchors.fill: parent
                  //  horizontalCenter:mainbuttons
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
