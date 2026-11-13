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
                visible: root.castPanelVisible
            }
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