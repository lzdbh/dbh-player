import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var castManager

    color: Theme.windowBg

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
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "Casting"
                color: Theme.text
                font.pixelSize: 15
                font.family: Theme.fontFamily
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Button {
                text: "Disconnect"
                onClicked: root.castManager.disconnect()
            }
        }

        Text {
            Layout.fillWidth: true
            text: "📺 " + root.castManager.deviceName
            color: Theme.text
            font.pixelSize: 13
            font.family: Theme.fontFamily
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: root.castManager.status
            color: Theme.textSecondary
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }

        Item { Layout.preferredHeight: 8 }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: root.fmt(root.castManager.position)
                color: Theme.text
                font.pixelSize: 11
                Layout.preferredWidth: 44
                horizontalAlignment: Text.AlignRight
            }
            Slider {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, root.castManager.duration)
                value: root.castManager.position
                onMoved: root.castManager.seek(value)
            }
            Text {
                text: root.fmt(root.castManager.duration)
                color: Theme.text
                font.pixelSize: 11
                Layout.preferredWidth: 44
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Button {
                text: root.castManager.playing ? "⏸" : "▶"
                font.pixelSize: 16
                onClicked: root.castManager.togglePlay()
                ToolTip.visible: hovered
                ToolTip.text: root.castManager.playing ? "Pause on TV" : "Play on TV"
            }
            Button {
                text: "⏹"
                font.pixelSize: 16
                onClicked: root.castManager.disconnect()
                ToolTip.visible: hovered
                ToolTip.text: "Stop casting"
            }

            Item { Layout.fillWidth: true }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "🔊"
                font.pixelSize: 14
            }
            Slider {
                Layout.fillWidth: true
                from: 0
                to: 100
                stepSize: 1
                value: root.castManager.volume
                onMoved: root.castManager.setVolume(value)
            }
            Text {
                text: root.castManager.volume + "%"
                color: Theme.textSecondary
                font.pixelSize: 11
                Layout.preferredWidth: 36
                horizontalAlignment: Text.AlignRight
            }
        }

        Item { Layout.fillHeight: true }

        Text {
            Layout.fillWidth: true
            text: "The TV pulls the stream from this computer. Keep the app running and on the same Wi-Fi."
            color: Theme.textSecondary
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }
}