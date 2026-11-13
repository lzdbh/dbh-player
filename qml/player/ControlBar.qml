import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var player
    readonly property bool hovered: barHover.hovered

    signal castToggled()

    height: 72
    color: "#d9000000"

    function fmt(ms) {
        const s = Math.max(0, Math.floor(ms / 1000))
        const h = Math.floor(s / 3600)
        const m = Math.floor((s % 3600) / 60)
        const sec = s % 60
        const pad = (n) => (n < 10 ? "0" + n : "" + n)
        return h > 0 ? h + ":" + pad(m) + ":" + pad(sec) : m + ":" + pad(sec)
    }

    HoverHandler { id: barHover }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 6
        anchors.bottomMargin: 8
        spacing: 2

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: root.fmt(root.player.position)
                color: "#ffffff"
                font.pixelSize: 12
                Layout.preferredWidth: 52
                horizontalAlignment: Text.AlignRight
            }
            Slider {
                id: seekSlider
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, root.player.duration)
                value: root.player.position
                onMoved: root.player.seek(value)
            }
            Text {
                text: root.fmt(root.player.duration)
                color: "#ffffff"
                font.pixelSize: 12
                Layout.preferredWidth: 52
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Button {
                text: root.player.playing ? "⏸" : "▶"
                font.pixelSize: 16
                onClicked: root.player.togglePlay()
                ToolTip.visible: hovered
                ToolTip.text: root.player.playing ? "Pause" : "Play"
            }
            Button {
                text: "⏹"
                font.pixelSize: 16
                onClicked: root.player.stop()
                ToolTip.visible: hovered
                ToolTip.text: "Stop"
            }
            Button {
                text: "📺"
                font.pixelSize: 16
                onClicked: root.castToggled()
                ToolTip.visible: hovered
                ToolTip.text: "Cast to device"
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "🔊"
                font.pixelSize: 14
            }
            Slider {
                id: volumeSlider
                Layout.preferredWidth: 110
                from: 0
                to: 1
                value: root.player.volume
                onMoved: root.player.volume = value
            }
        }
    }
}