import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Ant Design style remote-control panel shown while casting.
Rectangle {
    id: root

    required property var castManager

    color: Theme.colorBgContainer

    function fmt(ms) {
        const s = Math.max(0, Math.floor(ms / 1000))
        const h = Math.floor(s / 3600)
        const m = Math.floor((s % 3600) / 60)
        const sec = s % 60
        const pad = (n) => (n < 10 ? "0" + n : "" + n)
        return h > 0 ? h + ":" + pad(m) + ":" + pad(sec) : m + ":" + pad(sec)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("投屏中")
                color: Theme.colorText
                font.pixelSize: Theme.fontSizeLG
                font.family: Theme.fontFamily
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            AntButton {
                variant: "danger"
                small: true
                text: qsTr("断开")
                onClicked: root.castManager.disconnect()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: deviceRow.implicitHeight + 20
            radius: Theme.radiusLG
            color: Theme.colorPrimaryBg
            border.width: 1
            border.color: Theme.colorPrimaryBorder

            RowLayout {
                id: deviceRow
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                ColorIcon {
                    iconSize: 20
                    source: "qrc:/Dbhx/icons/cast.svg"
                    iconColor: Theme.colorPrimary
                }
                Text {
                    text: root.castManager.deviceName
                    color: Theme.colorText
                    font.pixelSize: Theme.fontSize
                    font.family: Theme.fontFamily
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                AntTag {
                    visible: root.castManager.transcoding
                    text: qsTr("转码中")
                    tone: "processing"
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.castManager.status
            color: Theme.colorTextTertiary
            font.pixelSize: Theme.fontSizeSM
            font.family: Theme.fontFamily
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: root.fmt(root.castManager.position)
                color: Theme.colorTextSecondary
                font.pixelSize: Theme.fontSizeSM
                font.family: Theme.fontFamily
                Layout.preferredWidth: 44
                horizontalAlignment: Text.AlignRight
            }
            AntSlider {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, root.castManager.duration)
                value: root.castManager.position
                enabled: !root.castManager.transcoding
                onMoved: root.castManager.seek(value)
                ToolTip.visible: hovered && root.castManager.transcoding
                ToolTip.text: qsTr("转码流不支持拖动进度")
            }
            Text {
                text: root.fmt(root.castManager.duration)
                color: Theme.colorTextSecondary
                font.pixelSize: Theme.fontSizeSM
                font.family: Theme.fontFamily
                Layout.preferredWidth: 44
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AntButton {
                variant: "primary"
                text: root.castManager.playing ? qsTr("⏸ 暂停") : qsTr("▶ 播放")
                Layout.fillWidth: true
                onClicked: root.castManager.togglePlay()
            }
            AntButton {
                text: qsTr("停止投屏")
                Layout.fillWidth: true
                onClicked: root.castManager.disconnect()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ColorIcon {
                iconSize: 16
                source: "qrc:/Dbhx/icons/volume.svg"
                iconColor: Theme.colorTextSecondary
            }
            AntSlider {
                Layout.fillWidth: true
                from: 0
                to: 100
                stepSize: 1
                value: root.castManager.volume
                onMoved: root.castManager.setVolume(value)
            }
            Text {
                text: root.castManager.volume + "%"
                color: Theme.colorTextSecondary
                font.pixelSize: Theme.fontSizeSM
                font.family: Theme.fontFamily
                Layout.preferredWidth: 38
                horizontalAlignment: Text.AlignRight
            }
        }

        Item { Layout.fillHeight: true }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: hint.implicitHeight + 16
            radius: Theme.radius
            color: Theme.colorFillTertiary

            Text {
                id: hint
                anchors.fill: parent
                anchors.margins: 8
                text: qsTr("电视直接从本机拉流播放。请保持应用运行、电脑与电视在同一 Wi-Fi。")
                color: Theme.colorTextTertiary
                font.pixelSize: Theme.fontSizeSM
                font.family: Theme.fontFamily
                wrapMode: Text.WordWrap
            }
        }
    }
}