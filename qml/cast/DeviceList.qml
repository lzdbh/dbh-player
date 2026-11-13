import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Ant Design style cast device panel.
Rectangle {
    id: root

    required property var deviceModel
    required property var castManager
    property url mediaSource

    color: Theme.colorBgContainer

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "投屏设备"
                color: Theme.colorText
                font.pixelSize: Theme.fontSizeLG
                font.family: Theme.fontFamily
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            AntButton {
                variant: "primary"
                small: true
                text: "扫描"
                onClicked: root.deviceModel.scan()
            }
        }

        Text {
            Layout.fillWidth: true
            text: "搜索同一 Wi-Fi 下的 DLNA 电视 / 投屏器（当贝、乐播等）"
            color: Theme.colorTextTertiary
            font.pixelSize: Theme.fontSizeSM
            font.family: Theme.fontFamily
            wrapMode: Text.WordWrap
        }

        ListView {
            id: deviceList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.deviceModel
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 6
                    radius: 3
                    color: parent.pressed ? Theme.colorTextQuaternary : Theme.colorBorder
                    opacity: parent.active ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }

            delegate: Rectangle {
                id: item
                required property var model
                width: ListView.view ? ListView.view.width : 0
                height: 60
                radius: Theme.radiusLG
                color: delegateMouse.containsMouse ? Theme.colorPrimaryBg : Theme.colorBgContainer
                border.width: 1
                border.color: delegateMouse.containsMouse ? Theme.colorPrimaryBorder
                                                          : Theme.colorBorderSecondary

                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on border.color { ColorAnimation { duration: 120 } }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    Rectangle {
                        width: 36
                        height: 36
                        radius: Theme.radius
                        color: Theme.colorPrimaryBg

                        ColorIcon {
                            anchors.centerIn: parent
                            iconSize: 20
                            source: "qrc:/Dbhx/icons/cast.svg"
                            iconColor: Theme.colorPrimary
                        }
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true

                        Text {
                            text: model.name
                            color: Theme.colorText
                            font.pixelSize: Theme.fontSize
                            font.family: Theme.fontFamily
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: model.manufacturer !== "" ? model.manufacturer : model.location
                            color: Theme.colorTextTertiary
                            font.pixelSize: Theme.fontSizeSM
                            font.family: Theme.fontFamily
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                MouseArea {
                    id: delegateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.castManager.cast(model.avTransport,
                                                     model.renderingControl,
                                                     model.name,
                                                     root.mediaSource)
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                visible: deviceList.count === 0

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "暂无设备"
                    color: Theme.colorTextTertiary
                    font.pixelSize: Theme.fontSize
                    font.family: Theme.fontFamily
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "确认电视与电脑在同一 Wi-Fi，然后点击「扫描」"
                    color: Theme.colorTextQuaternary
                    font.pixelSize: Theme.fontSizeSM
                    font.family: Theme.fontFamily
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}