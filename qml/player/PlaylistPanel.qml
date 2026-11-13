import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

// Ant Design style playlist side panel.
Rectangle {
    id: root

    required property var playlist
    property var player: null

    color: Theme.colorBgContainer

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("播放列表")
                color: Theme.colorText
                font.pixelSize: Theme.fontSizeLG
                font.family: Theme.fontFamily
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            AntTag {
                text: root.playlist.count + qsTr(" 项")
                tone: "default"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AntButton {
                variant: "primary"
                small: true
                text: qsTr("添加文件")
                Layout.fillWidth: true
                onClicked: addDialog.open()
            }
            AntButton {
                small: true
                text: qsTr("清空")
                enabled: root.playlist.count > 0
                onClicked: root.playlist.clear()
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.playlist
            spacing: 4
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
                required property int index
                width: ListView.view ? ListView.view.width : 0
                height: 44
                radius: Theme.radius
                color: model.current ? Theme.colorPrimaryBg
                       : itemMouse.containsMouse ? Theme.colorFillTertiary : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    spacing: 8

                    Rectangle {
                        width: 6
                        height: 6
                        radius: 3
                        color: model.current ? Theme.colorPrimary : "transparent"
                    }
                    Text {
                        text: model.title
                        color: model.current ? Theme.colorPrimary : Theme.colorText
                        font.pixelSize: Theme.fontSize
                        font.family: Theme.fontFamily
                        font.weight: model.current ? Font.DemiBold : Font.Normal
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    AntIconButton {
                        dark: false
                        iconSource: "qrc:/Dbhx/icons/close.svg"
                        iconColor: Theme.colorTextTertiary
                        iconSize: 12
                        implicitWidth: 24
                        implicitHeight: 24
                        opacity: itemMouse.containsMouse ? 1 : 0
                        onClicked: root.playlist.removeAt(item.index)
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("移除")
                    }
                }

                MouseArea {
                    id: itemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.playlist.playAt(item.index)
                    z: -1
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8
                visible: listView.count === 0

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("列表为空")
                    color: Theme.colorTextTertiary
                    font.pixelSize: Theme.fontSize
                    font.family: Theme.fontFamily
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("添加文件或拖入视频到播放区")
                    color: Theme.colorTextQuaternary
                    font.pixelSize: Theme.fontSizeSM
                    font.family: Theme.fontFamily
                }
            }
        }
    }

    FileDialog {
        id: addDialog
        title: qsTr("添加到播放列表")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("视频文件 (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.ts *.webm *.m4v *.mpg *.mpeg)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: root.playlist.addUrls(selectedFiles)
    }
}