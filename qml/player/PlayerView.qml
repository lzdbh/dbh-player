import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtQuick.Dialogs

Item {
    id: root

    required property var player
    property var playlist: null
    property var subtitles: null
    property bool controlsVisible: true
    property bool subtitleVisible: true
    property bool fullscreen: false

    signal castToggled()
    signal playlistToggled()
    signal fullscreenToggled()
    signal toastRequested(string message, string tone)

    function showControls() {
        controlsVisible = true
        hideTimer.restart()
    }

    function takeScreenshot() {
        if (root.player.source.toString() === "") {
            root.toastRequested(qsTr("没有正在播放的视频"), "error")
            return
        }
        videoSurface.grabToImage(function(result) {
            const stamp = Qt.formatDateTime(new Date(), "yyyyMMdd-hhmmss")
            const path = AppContext.picturesDir() + "/DBHX-" + stamp + ".png"
            if (result.saveToFile(path))
                root.toastRequested(qsTr("截图已保存到 ") + path, "success")
            else
                root.toastRequested(qsTr("截图保存失败"), "error")
        })
    }

    // Black backdrop so the letterbox area around the video is black, not the
    // window background.
    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    VideoOutput {
        id: videoSurface
        anchors.fill: parent
        // Reserve the control bar's height so the video is vertically centered
        // in the visible area above the control bar.
        anchors.bottomMargin: controlBar.height
        fillMode: VideoOutput.PreserveAspectFit
    }

    Connections {
        target: root.player
        function onMediaPlayerChanged() {
            if (root.player.mediaPlayer)
                root.player.mediaPlayer.videoOutput = videoSurface
        }
    }

    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (drop.hasUrls) {
                if (root.playlist)
                    root.playlist.addAndPlay(drop.urls)
                else
                    root.player.open(drop.urls[0])
            }
        }
    }

    // Background interaction layer (kept below the controls and placeholder).
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton)
                contextMenu.popup(mouse.x, mouse.y)
            else
                root.player.togglePlay()
        }
        onDoubleClicked: root.fullscreenToggled()
        onPositionChanged: root.showControls()
        onEntered: root.showControls()
    }

    // Placeholder shown when no media is loaded.
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16
        visible: root.player.source.toString() === ""

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 88
            height: 88
            radius: 44
            color: Theme.colorPrimaryBg

            ColorIcon {
                anchors.centerIn: parent
                iconSize: 40
                source: "qrc:/Dbhx/icons/play.svg"
                iconColor: Theme.colorPrimary
            }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("拖入视频，或打开文件开始播放")
            color: Theme.colorTextSecondary
            font.pixelSize: 15
            font.family: Theme.fontFamily
        }
        AntButton {
            Layout.alignment: Qt.AlignHCenter
            variant: "primary"
            text: qsTr("打开文件")
            onClicked: fileDialog.open()
        }
    }

    // Subtitle overlay pinned just above the video's bottom edge (the video
    // area reserves the control bar height), so it doesn't move when the
    // control bar shows/hides.
    Text {
        id: subtitleText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: controlBar.height + 12
        width: parent.width * 0.85
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: "#ffffff"
        style: Text.Outline
        styleColor: "#c0000000"
        font.pixelSize: Math.max(18, Math.round(parent.height / 28))
        font.family: Theme.fontFamily
        visible: root.subtitleVisible && root.subtitles && root.subtitles.loaded
        text: visible ? root.subtitles.textAt(root.player.position) : ""
    }

    ControlBar {
        id: controlBar
        player: root.player
        playlist: root.playlist
        subtitleLoaded: root.subtitles && root.subtitles.loaded
        subtitleActive: root.subtitleVisible && root.subtitles && root.subtitles.loaded
        fullscreen: root.fullscreen
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        opacity: root.controlsVisible ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 150 } }
        onCastToggled: root.castToggled()
        onPlaylistToggled: root.playlistToggled()
        onSubtitleMenuRequested: subtitleMenu.popup()
        onSubtitleLoadRequested: subtitleDialog.open()
        onScreenshotRequested: root.takeScreenshot()
        onFullscreenToggled: root.fullscreenToggled()
    }

    AntMenu {
        id: subtitleMenu

        Repeater {
            model: root.player.subtitleTracks
            AntMenuItem {
                required property var modelData
                text: modelData.label
                checked: root.player.selectedSubtitleIndex === modelData.index
                onTriggered: root.player.selectSubtitleTrack(modelData.index)
            }
        }
        AntMenuItem {
            visible: root.player.subtitleTracks.length === 0
            enabled: false
            text: qsTr("无内嵌字幕轨")
        }
        MenuSeparator {}
        AntMenuItem {
            text: qsTr("关闭字幕")
            checked: !(root.subtitles && root.subtitles.loaded)
            enabled: root.subtitles && root.subtitles.loaded
            onTriggered: root.player.disableSubtitles()
        }
        AntMenuItem {
            text: qsTr("从文件加载字幕...")
            onTriggered: subtitleDialog.open()
        }
    }

    AntMenu {
        id: contextMenu
        AntMenuItem {
            text: qsTr("打开文件...")
            onTriggered: fileDialog.open()
        }
        AntMenuItem {
            text: qsTr("加载字幕 (SRT)...")
            onTriggered: subtitleDialog.open()
        }
        AntMenuItem {
            text: qsTr("关闭字幕")
            enabled: root.subtitles && root.subtitles.loaded
            onTriggered: root.subtitles.unload()
        }
        MenuSeparator {}
        AntMenuItem {
            text: root.player.playing ? qsTr("暂停") : qsTr("播放")
            onTriggered: root.player.togglePlay()
        }
        AntMenuItem {
            text: qsTr("停止")
            onTriggered: root.player.stop()
        }
        AntMenu {
            title: qsTr("倍速")
            Repeater {
                model: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0]
                AntMenuItem {
                    required property var modelData
                    text: modelData + "x"
                    checked: Math.abs(root.player.playbackRate - modelData) < 0.01
                    onTriggered: root.player.playbackRate = modelData
                }
            }
        }
        MenuSeparator {}
        AntMenuItem {
            text: qsTr("截图")
            onTriggered: root.takeScreenshot()
        }
        AntMenuItem {
            text: qsTr("播放列表")
            onTriggered: root.playlistToggled()
        }
        AntMenuItem {
            text: qsTr("投屏到设备")
            onTriggered: root.castToggled()
        }
        AntMenuItem {
            text: root.fullscreen ? qsTr("退出全屏") : qsTr("全屏")
            onTriggered: root.fullscreenToggled()
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("打开视频")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("视频文件 (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.ts *.webm *.m4v *.mpg *.mpeg)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            if (root.playlist)
                root.playlist.addAndPlay(selectedFiles)
            else if (selectedFiles.length > 0)
                root.player.open(selectedFiles[0])
        }
    }

    FileDialog {
        id: subtitleDialog
        title: qsTr("加载字幕")
        nameFilters: [qsTr("SRT 字幕 (*.srt)"), qsTr("所有文件 (*)")]
        onAccepted: {
            if (root.subtitles && root.subtitles.load(selectedFile)) {
                root.subtitleVisible = true
                root.toastRequested(qsTr("字幕已加载: ") + root.subtitles.fileName, "success")
            } else {
                root.toastRequested(qsTr("字幕加载失败（仅支持 SRT）"), "error")
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 2500
        onTriggered: {
            if (root.player.playing && !controlBar.hovered)
                root.controlsVisible = false
        }
    }

    Connections {
        target: root.player
        function onPlayingChanged() {
            if (root.player.playing)
                hideTimer.restart()
            else
                root.controlsVisible = true
        }
    }
}