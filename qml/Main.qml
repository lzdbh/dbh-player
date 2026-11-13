import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root

    width: 1100
    height: 700
    minimumWidth: 820
    minimumHeight: 520
    visible: true
    title: AppContext.appName
    flags: Qt.Window | Qt.FramelessWindowHint
    color: Theme.windowBg

    function toggleMaximized() {
        if (visibility === Window.Maximized)
            showNormal()
        else
            showMaximized()
    }

    PlayerController {
        id: playerController
        onErrorOccurred: (message) => console.warn("Playback error:", message)
    }

    Component.onCompleted: {
        if (initialFile !== "")
            playerController.open(initialFile)
    }

    DlnaDeviceModel {
        id: dlnaModel
    }

    CastManager {
        id: castManager
        onCastStarted: {
            playerController.pause()
            root.castPanelVisible = true
        }
        onCastFailed: (message) => {
            castErrorBanner.text = message
            castErrorBanner.visible = true
            castErrorTimer.restart()
        }
    }

    property bool castPanelVisible: false

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Loader {
            Layout.fillWidth: true
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

        // Content area: player (Track A) + toggleable cast device panel (Track B).
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            PlayerView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                player: playerController
                onCastToggled: root.castPanelVisible = !root.castPanelVisible
            }

            DeviceList {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                deviceModel: dlnaModel
                castManager: castManager
                mediaSource: playerController.source
                visible: root.castPanelVisible && !castManager.active
            }

            CastPanel {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                castManager: castManager
                visible: castManager.active
            }
        }
    }

    Rectangle {
        id: castErrorBanner
        property alias text: castErrorText.text
        anchors.top: parent.top
        anchors.topMargin: 48
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 40, castErrorText.implicitWidth + 32)
        height: castErrorText.implicitHeight + 16
        radius: Theme.cornerRadius
        color: "#e05545"
        visible: false
        z: 10

        Text {
            id: castErrorText
            anchors.centerIn: parent
            color: "#ffffff"
            font.pixelSize: 12
            font.family: Theme.fontFamily
        }

        Timer {
            id: castErrorTimer
            interval: 5000
            onTriggered: castErrorBanner.visible = false
        }
    }

    // Frameless windows lose native resize borders; provide a corner grip.
    MouseArea {
        width: 16
        height: 16
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeFDiagCursor
        onPressed: root.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }
}