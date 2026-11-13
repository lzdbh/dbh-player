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
        onErrorOccurred: (message) => toast.show("播放错误: " + message, "error")
        onEndOfMedia: playlist.onMediaEnded()
        onSourceChanged: {
            subtitles.unload()
            if (source.toString() !== "" && subtitles.autoLoadFor(source))
                toast.show("已自动加载字幕: " + subtitles.fileName, "success")
        }
    }

    PlaylistController {
        id: playlist
        onRequestOpen: (url) => playerController.open(url)
    }

    SubtitleTrack {
        id: subtitles
    }

    Component.onCompleted: {
        if (initialFiles.length > 0)
            playlist.addUrls(initialFiles)
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
        title: "打开视频"
        fileMode: FileDialog.OpenFiles
        nameFilters: [
            "视频文件 (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.ts *.webm *.m4v *.mpg *.mpeg)",
            "所有文件 (*)"
        ]
        onAccepted: playlist.addUrls(selectedFiles)
    }
}