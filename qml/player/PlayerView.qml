import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtQuick.Dialogs

Item {
    id: root

    required property var player
    property bool controlsVisible: true

    signal castToggled()

    function showControls() {
        controlsVisible = true
        hideTimer.restart()
    }

    VideoOutput {
        id: videoSurface
        anchors.fill: parent
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
            if (drop.hasUrls)
                root.player.open(drop.urls[0])
        }
    }

    // Background interaction layer (kept below the controls and placeholder).
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton)
                contextMenu.popup()
            else
                root.player.togglePlay()
        }
        onPositionChanged: root.showControls()
        onEntered: root.showControls()
    }

    // Placeholder shown when no media is loaded.
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16
        visible: root.player.source.toString() === ""

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "🎬"
            font.pixelSize: 48
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Drag a video here, or open a file"
            color: Theme.textSecondary
            font.pixelSize: 15
            font.family: Theme.fontFamily
        }
        Button {
            Layout.alignment: Qt.AlignHCenter
            text: "Open File"
            onClicked: fileDialog.open()
        }
    }

    ControlBar {
        id: controlBar
        player: root.player
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        opacity: root.controlsVisible ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 150 } }
        onCastToggled: root.castToggled()
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: "Open File..."
            onTriggered: fileDialog.open()
        }
        MenuSeparator {}
        MenuItem {
            text: root.player.playing ? "Pause" : "Play"
            onTriggered: root.player.togglePlay()
        }
        MenuItem {
            text: "Stop"
            onTriggered: root.player.stop()
        }
        MenuSeparator {}
        MenuItem {
            text: "Cast to Device"
            enabled: false   // wired up in M2
        }
    }

    FileDialog {
        id: fileDialog
        title: "Open Video"
        nameFilters: [
            "Video files (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.ts *.webm *.m4v *.mpg *.mpeg)",
            "All files (*)"
        ]
        onAccepted: root.player.open(selectedFile)
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