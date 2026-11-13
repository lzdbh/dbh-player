import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Dark overlay control bar with Ant Design accents.
Rectangle {
    id: root

    required property var player
    property var playlist: null
    property bool subtitleActive: false
    property bool subtitleLoaded: false
    property bool fullscreen: false
    readonly property bool hovered: barHover.hovered

    signal castToggled()
    signal playlistToggled()
    signal subtitleToggled()
    signal subtitleLoadRequested()
    signal screenshotRequested()
    signal fullscreenToggled()

    height: 76
    color: Theme.playerScrim

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
                color: Theme.playerTextSecondary
                font.pixelSize: Theme.fontSizeSM
                font.family: Theme.fontFamily
                Layout.preferredWidth: 52
                horizontalAlignment: Text.AlignRight
            }
            AntSlider {
                id: seekSlider
                dark: true
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, root.player.duration)
                value: root.player.position
                onMoved: root.player.seek(value)
            }
            Text {
                text: root.fmt(root.player.duration)
                color: Theme.playerTextSecondary
                font.pixelSize: Theme.fontSizeSM
                font.family: Theme.fontFamily
                Layout.preferredWidth: 52
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            AntIconButton {
                iconSource: "qrc:/Dbhx/icons/prev.svg"
                enabled: root.playlist && root.playlist.currentIndex > 0
                onClicked: root.playlist.previous()
                ToolTip.visible: hovered
                ToolTip.text: "上一个"
            }
            AntIconButton {
                iconSource: root.player.playing ? "qrc:/Dbhx/icons/pause.svg"
                                                : "qrc:/Dbhx/icons/play.svg"
                iconSize: 24
                implicitWidth: 44
                implicitHeight: 40
                onClicked: root.player.togglePlay()
                ToolTip.visible: hovered
                ToolTip.text: root.player.playing ? "暂停 (Space)" : "播放 (Space)"
            }
            AntIconButton {
                iconSource: "qrc:/Dbhx/icons/stop.svg"
                onClicked: root.player.stop()
                ToolTip.visible: hovered
                ToolTip.text: "停止"
            }
            AntIconButton {
                iconSource: "qrc:/Dbhx/icons/next.svg"
                enabled: root.playlist && root.playlist.currentIndex >= 0
                         && root.playlist.currentIndex < root.playlist.count - 1
                onClicked: root.playlist.next()
                ToolTip.visible: hovered
                ToolTip.text: "下一个"
            }

            Item { Layout.preferredWidth: 8 }

            // Playback rate
            AntButton {
                id: rateButton
                variant: "text"
                small: true
                dark: true
                Layout.preferredWidth: 52
                text: root.player.playbackRate.toFixed(2).replace(/\.?0+$/, "") + "x"
                onClicked: rateMenu.popup(rateButton, 0, -rateMenu.height - 4)
                ToolTip.visible: hovered
                ToolTip.text: "倍速"

                AntMenu {
                    id: rateMenu
                    Repeater {
                        model: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0]
                        AntMenuItem {
                            required property var modelData
                            text: modelData + "x"
                            checkable: true
                            checked: Math.abs(root.player.playbackRate - modelData) < 0.01
                            onTriggered: root.player.playbackRate = modelData
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            AntIconButton {
                iconSource: "qrc:/Dbhx/icons/subtitle.svg"
                iconColor: root.subtitleActive ? Theme.colorPrimary : "#ffffff"
                opacity: root.subtitleLoaded ? 1 : 0.5
                onClicked: root.subtitleLoaded ? root.subtitleToggled()
                                               : root.subtitleLoadRequested()
                ToolTip.visible: hovered
                ToolTip.text: root.subtitleLoaded ? "字幕开关" : "加载字幕 (SRT)"
            }
            AntIconButton {
                iconSource: "qrc:/Dbhx/icons/camera.svg"
                onClicked: root.screenshotRequested()
                ToolTip.visible: hovered
                ToolTip.text: "截图"
            }
            AntIconButton {
                iconSource: "qrc:/Dbhx/icons/playlist.svg"
                onClicked: root.playlistToggled()
                ToolTip.visible: hovered
                ToolTip.text: "播放列表"
            }
            AntIconButton {
                iconSource: "qrc:/Dbhx/icons/cast.svg"
                onClicked: root.castToggled()
                ToolTip.visible: hovered
                ToolTip.text: "投屏到设备"
            }

            Item { Layout.preferredWidth: 8 }

            AntIconButton {
                iconSource: root.player.muted || root.player.volume === 0
                            ? "qrc:/Dbhx/icons/mute.svg"
                            : "qrc:/Dbhx/icons/volume.svg"
                onClicked: root.player.muted = !root.player.muted
                ToolTip.visible: hovered
                ToolTip.text: root.player.muted ? "取消静音 (M)" : "静音 (M)"
            }
            AntSlider {
                id: volumeSlider
                dark: true
                Layout.preferredWidth: 90
                from: 0
                to: 1
                value: root.player.muted ? 0 : root.player.volume
                onMoved: {
                    root.player.muted = false
                    root.player.volume = value
                }
            }

            Item { Layout.preferredWidth: 8 }

            AntIconButton {
                iconSource: root.fullscreen ? "qrc:/Dbhx/icons/fullscreen-exit.svg"
                                            : "qrc:/Dbhx/icons/fullscreen.svg"
                onClicked: root.fullscreenToggled()
                ToolTip.visible: hovered
                ToolTip.text: root.fullscreen ? "退出全屏 (F)" : "全屏 (F)"
            }
        }
    }
}