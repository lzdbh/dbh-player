import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var deviceModel

    color: Theme.windowBg

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "Cast Devices"
                color: Theme.text
                font.pixelSize: 15
                font.family: Theme.fontFamily
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Button {
                text: "Scan"
                onClicked: root.deviceModel.scan()
            }
        }

        Text {
            Layout.fillWidth: true
            text: "DLNA renderers on your LAN (e.g. Dangbei / Lebo)"
            color: Theme.textSecondary
            font.pixelSize: 11
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

            delegate: Rectangle {
                required property var model
                width: ListView.view ? ListView.view.width : 0
                height: 56
                radius: Theme.cornerRadius
                color: delegateMouse.containsMouse ? Theme.border : "transparent"
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 2

                    Text {
                        text: model.name
                        color: Theme.text
                        font.pixelSize: 13
                        font.family: Theme.fontFamily
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        text: model.manufacturer !== "" ? model.manufacturer : model.location
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    id: delegateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    // Casting itself is wired up in M2.
                }
            }

            Text {
                anchors.centerIn: parent
                visible: deviceList.count === 0
                text: "No devices yet.\nMake sure the TV is on the same Wi-Fi, then Scan."
                color: Theme.textSecondary
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}