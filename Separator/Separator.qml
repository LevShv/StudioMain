import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Dialogs


Rectangle {
    id: separatorPanel
    color: "#2D2D2D"
    border.color: "#444"
    border.width: 1
    property int clipIndex: 0
    property bool panelVisible: false
    property var pluginModel: null
    property string imagesPath: ""
    property int selectedTrackIndex: -1

    visible: panelVisible

    // Main container
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Fixed Label at the top (non-scrolling)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            anchors.top: parent.top
            color: "#2D2D2D"

            Label {
                anchors.centerIn: parent
                anchors.margins: 5
                width: parent.width - 10 // Account for margins
                text: "Инструменты: Дорожка " + (selectedTrackIndex + 1)
                color: "#ECEFF4"
                font.pixelSize: 12
                wrapMode: Text.WordWrap // Enable word wrapping
                maximumLineCount: 2 // Limit to two lines
                elide: Text.ElideRight // Elide if text still overflows
                horizontalAlignment: Text.AlignHCenter
            }
        }
        // Прокручиваемая область для кнопок
        Flickable {
            id: buttonsFlickable
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: buttonsColumn.width
            contentHeight: buttonsColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick // Только вертикальная прокрутка
            

            // Column для вертикального размещения контейнеров
            Column {
                id: buttonsColumn
                width: separatorPanel.width
                spacing: 10
                leftPadding: 15 // Константный отступ слева
                rightPadding: 15 // Константный отступ справа
                topPadding: 0

                Repeater {
                    model: viewModel.pluginModel
                    onCountChanged: console.log("Repeater count changed to:", count)

                    // Контейнер для группы кнопок (FX контейнер)
                    Rectangle {
                        id: fxContainer
                        width: parent.width - buttonsColumn.leftPadding - buttonsColumn.rightPadding // Заполняет ширину с учетом отступов
                        height: 60
                        radius: 3
                        color: "#4C566A"
                        border.color: "#ECEFF4"
                        border.width: 1

                        property bool isPinned: false // Добавляем свойство isPinned

                        RoundButton {
                            id: pinButton
                            width: 20
                            height: 20
                            radius: width / 2
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: 4
                            anchors.rightMargin: 4
                            ToolTip.visible: hovered
                            ToolTip.delay: 500
                            ToolTip.text: fxContainer.isPinned ? "Открепить" : "Закрепить"

                            background: Rectangle {
                                radius: parent.radius
                                color: pinButton.hovered ? "#d0d0d0" : "transparent"
                                border.color: pinButton.hovered ? "#a0a0a0" : "transparent"
                                border.width: 1
                                Behavior on color { ColorAnimation { duration: 100 } }
                                Behavior on border.color { ColorAnimation { duration: 100 } }
                            }

                            Image {
                                anchors.centerIn: parent
                                width: 20
                                height: 20
                                source: fxContainer.isPinned ? imagesPath + "закреплено.png" : imagesPath + "откреплено.png"
                                sourceSize.width: 12
                                sourceSize.height: 12
                                opacity: pinButton.down ? 0.7 : 1.0
                                fillMode: Image.PreserveAspectFit
                                Behavior on opacity { NumberAnimation { duration: 100 } }
                            }

                            onClicked: {
                                fxContainer.isPinned = !fxContainer.isPinned
                                pinButton.scale = 0.95
                                console.log("Pin button clicked for plugin: trackIndex=", model.trackIndex, "pluginIndex=", model.pluginIndex, "isPinned=", fxContainer.isPinned)
                                
                            }
                        }

                        // Контекстное меню для удаления
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: {
                                if (mouse.button === Qt.RightButton) {
                                    contextMenuSeparator.popup()
                                }
                            }
                        }

                        Menu {
                            id: contextMenuSeparator
                            width: 130
                            topPadding: 2
                            bottomPadding: 2

                            delegate: MenuItem {
                                id: menuItemSeparator
                                implicitHeight: 10
                                padding: 4

                                contentItem: Text {
                                    text: parent.text
                                    color: "#EEE"
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                }

                                background: Rectangle {
                                    color: parent.highlighted ? "#555" : "transparent"
                                    radius: 2
                                }
                            }

                            MenuItem {
                                text: "Удалить плагин"
                                onTriggered: {
                                    if (model.trackIndex >= 0 && model.pluginIndex >= 0) {
                                        viewModel.pluginModel.deletePlugin(model.trackIndex, model.pluginIndex)
                                        viewModel.pluginModel.refresh()
                                        console.log("Deleted plugin: trackIndex=", model.trackIndex, "pluginIndex=", model.pluginIndex)
                                    }
                                }
                            }
                        }

                        // Column для вертикального размещения Label и кнопок
                        Column {
                            anchors.fill: parent
                            spacing: 10

                            // Label сверху
                            Text {
                                id: fxLabel
                                text: model.name || "FX " + (index + 1)
                                color: "#ECEFF4"
                                font {
                                    family: "Tahoma"
                                    pixelSize: 10
                                }
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 4
                            }

                            // Row для кнопок внизу
                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom // Привязка к нижней части
                                anchors.bottomMargin: 4 // Отступ от нижней границы
                                spacing: 4

                                // Круглая кнопка Hide (слева)
                                RoundButton {
                                    id: hideButton
                                    width: 30
                                    height: 30
                                    radius: width / 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    ToolTip.visible: hovered
                                    ToolTip.delay: 500
                                    ToolTip.text: "Скрыть"

                                    background: Rectangle {
                                        radius: parent.radius
                                        color: hideButton.hovered ? "#d0d0d0" : "transparent"
                                        border.color: hideButton.hovered ? "#a0a0a0" : "transparent"
                                        border.width: 1
                                        Behavior on color { ColorAnimation { duration: 100 } }
                                        Behavior on border.color { ColorAnimation { duration: 100 } }
                                    }

                                    Image {
                                        anchors.centerIn: parent
                                        width: 18
                                        height: 18
                                        source: imagesPath + "minus.png"
                                        sourceSize.width: 18
                                        sourceSize.height: 18
                                        opacity: hideButton.down ? 0.7 : 1.0
                                        fillMode: Image.PreserveAspectFit
                                        Behavior on opacity { NumberAnimation { duration: 100 } }
                                    }

                                    onClicked: {
                                        hideButton.scale = 0.95
                                        console.log("Hide button clicked for plugin: trackIndex=", model.trackIndex, "pluginIndex=", model.pluginIndex)
                                    }

                                    Behavior on scale {
                                        NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                                    }
                                }

                                // Прямоугольная AUDIO кнопка с надписью Open
                                Button {
                                    id: audioButton
                                    text: "Open"
                                    width: 40
                                    height: 38
                                    anchors.verticalCenter: parent.verticalCenter
                                    ToolTip.visible: hovered
                                    ToolTip.delay: 500
                                    ToolTip.text: "Открыть " + (index + 1)
                                    font {
                                        family: "Tahoma"
                                        pixelSize: 11
                                    }
                                    leftPadding: 0
                                    rightPadding: 0

                                    background: Rectangle {
                                        radius: 3
                                        color: parent.down ? "#B8C1FC" : (parent.hovered ? "#D8DDFC" : "#CCD2FC")
                                        border.color: "#8293FC"
                                        border.width: 1
                                    }

                                    contentItem: Text {
                                        text: parent.text
                                        font: parent.font
                                        color: "#5153FF"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        anchors.fill: parent
                                    }

                                    onClicked: {
                                        if (model.trackIndex >= 0 && model.pluginIndex >= 0) {
                                            viewModel.pluginModel.openPluginEditor(model.trackIndex, model.pluginIndex);
                                            console.log("Opening plugin editor: trackIndex=", model.trackIndex, "pluginIndex=", model.pluginIndex);
                                        }
                                    }
                                }

                                // Круглая кнопка Hide2 (справа)
                                RoundButton {
                                    id: minusButton
                                    width: 30
                                    height: 30
                                    radius: width / 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    ToolTip.visible: hovered
                                    ToolTip.delay: 500
                                    ToolTip.text: "Скрыть2"

                                    background: Rectangle {
                                        radius: parent.radius
                                        color: minusButton.hovered ? "#d0d0d0" : "transparent"
                                        border.color: minusButton.hovered ? "#a0a0a0" : "transparent"
                                        border.width: 1
                                        Behavior on color { ColorAnimation { duration: 100 } }
                                        Behavior on border.color { ColorAnimation { duration: 100 } }
                                    }

                                    Image {
                                        anchors.centerIn: parent
                                        width: 18
                                        height: 18
                                        source: imagesPath + "minus.png"
                                        sourceSize.width: 18
                                        sourceSize.height: 18
                                        opacity: minusButton.down ? 0.7 : 1.0
                                        fillMode: Image.PreserveAspectFit
                                        Behavior on opacity { NumberAnimation { duration: 100 } }
                                    }

                                    onClicked: {
                                        minusButton.scale = 0.95
                                        viewModel.pluginModel.HidePlugin(model.trackIndex, model.pluginIndex);
                                        viewModel.pluginModel.refresh();
                                        console.log("Hide2 button clicked for plugin: trackIndex=", model.trackIndex, "pluginIndex=", model.pluginIndex)
                                    }

                                    Behavior on scale {
                                        NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Индикатор прокрутки (если контент не помещается)
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            color: "#666"
            visible: buttonsFlickable.contentHeight > buttonsFlickable.height

            Rectangle {
                width: (buttonsFlickable.height / buttonsFlickable.contentHeight) * parent.width
                height: parent.height
                x: (buttonsFlickable.contentY / buttonsFlickable.contentHeight) * parent.width
                color: "#ECEFF4"
            }
        }
    }
}