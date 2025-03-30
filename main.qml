import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.15
import QtQuick.Controls 

Window {
    visible: true
    width: 640
    height: 480
    title: "StudioMain"
    SplitView {
           anchors.fill: parent
           orientation: Qt.Horizontal

           // Левая панель (список треков)
           Rectangle {
               id: leftPanel
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

                   ListView {
                       id: trackList
                       Layout.fillWidth: true
                       Layout.fillHeight: true
                       model: ["Трек 1", "Трек 2", "Трек 3", "Трек 4"]
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
                               leftPadding: 10
                           }
                       }
                   }
               }
           }

           // Центральная панель (плеер)
           Rectangle {
               id: centerPanel
               color: "#3B4252"
               Layout.fillWidth: true
               Layout.minimumWidth: 300

               ColumnLayout {
                   anchors.fill: parent
                   spacing: 20

                   Label {
                       text: "Плеер"
                       color: "white"
                       font.bold: true
                       Layout.alignment: Qt.AlignHCenter
                       Layout.topMargin: 20
                   }

                   Slider {
                       id: progressSlider
                       Layout.fillWidth: true
                       Layout.leftMargin: 20
                       Layout.rightMargin: 20
                       value: 0.5
                   }

                   RowLayout {
                       Layout.alignment: Qt.AlignHCenter
                       spacing: 20

                       Button {
                           text: "⏮"
                           flat: true
                           contentItem: Text {
                               text: parent.text
                               color: "white"
                               font.pixelSize: 20
                           }
                       }

                       Button {
                           text: "⏯"
                           flat: true
                           contentItem: Text {
                               text: parent.text
                               color: "white"
                               font.pixelSize: 20
                           }
                       }

                       Button {
                           text: "⏭"
                           flat: true
                           contentItem: Text {
                               text: parent.text
                               color: "white"
                               font.pixelSize: 20
                           }
                       }
                   }
               }
           }

           // Правая панель (плейлисты)
           Rectangle {
               id: rightPanel
               color: "#2E3440"
               Layout.minimumWidth: 150
               Layout.preferredWidth: 200
               SplitView.preferredWidth: 200

               ColumnLayout {
                   anchors.fill: parent
                   spacing: 10

                   Label {
                       text: "Плейлисты"
                       color: "white"
                       font.bold: true
                       Layout.alignment: Qt.AlignHCenter
                       Layout.topMargin: 10
                   }

                   ListView {
                       id: playlistList
                       Layout.fillWidth: true
                       Layout.fillHeight: true
                       model: ["Плейлист 1", "Плейлист 2", "Плейлист 3"]
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
                               leftPadding: 10
                           }
                       }
                   }
               }
           }
    }
}
