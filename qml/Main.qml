import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root

    width: 1100
    height: 700
    minimumWidth: 820
    minimumHeight: 520
    visible: true
    title: AppContext.appName
    flags: Qt.Window | Qt.FramelessWindowHint
    color: Theme.colorBgLayout

    property bool panelVisible: false
    property string panelMode: "cast"   // cast | playlist
    readonly property bool fullscreen: visibility === Window.FullScreen

    function toggleMaximized() {
        if (visibility === Window.Maximized)
            showNormal()
        else
            showMaximized()
    }

    function toggleFullscreen() {
        if (fullscreen)
            showNormal()
        else
            showFullScreen()
    }

    function togglePanel(mode) {
        if (panelVisible && panelMode === mode) {
            panelVisible = false
        } else {
            panelMode = mode
            panelVisible = true
        }
    }

    PlayerController {
        id: playerController
        objectName: "playerController"
        onErrorOccurred: (message) => toast.show(qsTr("播放错误: ") + message, "error")
        onEndOfMedia: playlist.onMediaEnded()
        onSourceChanged: {
            subtitles.unload()
            if (source.toString() !== "" && subtitles.autoLoadFor(source))
                toast.show(qsTr("已自动加载字幕: ") + subtitles.fileName, "success")
        }
        onSubtitleFileReady: (file) => {
            if (subtitles.load(file))
                toast.show(qsTr("已加载内嵌字幕"), "success")
        }
        onSubtitlesCleared: subtitles.unload()
    }

    PlaylistController {
        id: playlist
        onRequestOpen: (url) => playerController.open(url)
    }

    SubtitleTrack {
        id: subtitles
    }

    Component.onCompleted: {
        playerController.volume = AppContext.loadSetting("player/volume", 1.0)
        playerController.muted = AppContext.loadSetting("player/muted", false)
        playerController.playbackRate = AppContext.loadSetting("player/rate", 1.0)
        const geo = AppContext.loadSetting("window/geometry", "")
        if (geo !== "") {
            const p = geo.split(",")
            if (p.length === 4) {
                root.x = parseInt(p[0]); root.y = parseInt(p[1])
                root.width = parseInt(p[2]); root.height = parseInt(p[3])
            }
        }
        if (initialFiles.length > 0)
            playlist.addUrls(initialFiles)
    }

    onClosing: {
        AppContext.saveSetting("player/volume", playerController.volume)
        AppContext.saveSetting("player/muted", playerController.muted)
        AppContext.saveSetting("player/rate", playerController.playbackRate)
        AppContext.saveSetting("window/geometry",
                               root.x + "," + root.y + "," + root.width + "," + root.height)
    }

    Timer {
        id: settingsTimer
        interval: 800
        onTriggered: {
            AppContext.saveSetting("player/volume", playerController.volume)
            AppContext.saveSetting("player/muted", playerController.muted)
            AppContext.saveSetting("player/rate", playerController.playbackRate)
        }
    }

    Connections {
        target: playerController
        function onVolumeChanged() { settingsTimer.restart() }
        function onMutedChanged() { settingsTimer.restart() }
        function onPlaybackRateChanged() { settingsTimer.restart() }
    }

    DlnaDeviceModel {
        id: dlnaModel
    }

    CastManager {
        id: castManager
        onCastStarted: {
            playerController.pause()
            root.panelVisible = true
            root.panelMode = "cast"
        }
        onCastFailed: (message) => toast.show(message, "error")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Loader {
            Layout.fillWidth: true
            visible: !root.fullscreen
            sourceComponent: AppContext.isMac ? macChrome : winChrome
        }

        Component {
            id: macChrome
            MacTitleBar { window: root; title: root.title }
        }
        Component {
            id: winChrome
            WindowsTitleBar { window: root; title: root.title }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            PlayerView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                player: playerController
                playlist: playlist
                subtitles: subtitles
                fullscreen: root.fullscreen
                onCastToggled: root.togglePanel("cast")
                onPlaylistToggled: root.togglePanel("playlist")
                onFullscreenToggled: root.toggleFullscreen()
                onToastRequested: (message, tone) => toast.show(message, tone)
            }

            Rectangle {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                visible: root.panelVisible || castManager.active
                color: Theme.colorBgContainer

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    color: Theme.colorBorderSecondary
                }

                DeviceList {
                    anchors.fill: parent
                    deviceModel: dlnaModel
                    castManager: castManager
                    mediaSource: playerController.source
                    visible: root.panelVisible && root.panelMode === "cast" && !castManager.active
                }

                CastPanel {
                    anchors.fill: parent
                    castManager: castManager
                    visible: castManager.active
                }

                PlaylistPanel {
                    anchors.fill: parent
                    playlist: playlist
                    player: playerController
                    visible: root.panelVisible && root.panelMode === "playlist" && !castManager.active
                }
            }
        }
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 100
        z: 20
    }

    // Frameless windows lose native resize borders; provide a corner grip.
    MouseArea {
        width: 16
        height: 16
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeFDiagCursor
        visible: !root.fullscreen
        onPressed: root.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
        z: 5
    }

    // Keyboard shortcuts
    Shortcut { sequence: "Space"; onActivated: playerController.togglePlay() }
    Shortcut { sequence: "Left"; onActivated: playerController.seek(Math.max(0, playerController.position - 5000)) }
    Shortcut { sequence: "Right"; onActivated: playerController.seek(Math.min(playerController.duration, playerController.position + 5000)) }
    Shortcut { sequence: "Up"; onActivated: playerController.volume = Math.min(1, playerController.volume + 0.05) }
    Shortcut { sequence: "Down"; onActivated: playerController.volume = Math.max(0, playerController.volume - 0.05) }
    Shortcut { sequence: "F"; onActivated: root.toggleFullscreen() }
    Shortcut { sequence: "M"; onActivated: playerController.muted = !playerController.muted }
    Shortcut { sequence: "Esc"; enabled: root.fullscreen; onActivated: root.showNormal() }
    Shortcut { sequence: "Ctrl+O"; onActivated: openShortcutDialog.open() }

    FileDialog {
        id: openShortcutDialog
        title: qsTr("打开视频")
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            qsTr("视频文件 (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.ts *.webm *.m4v *.mpg *.mpeg)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: playlist.addAndPlay(selectedFiles)
    }
}