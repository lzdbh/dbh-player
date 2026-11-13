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

        // Content area: player view (Track A) and cast panel (Track B) plug in here from M1.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 14

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: AppContext.appName
                    color: Theme.text
                    font.pixelSize: 30
                    font.family: Theme.fontFamily
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "M0 skeleton ready · platform " + AppContext.platform + " · v" + AppContext.appVersion
                    color: Theme.textSecondary
                    font.pixelSize: 14
                    font.family: Theme.fontFamily
                }
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 220
                    height: 1
                    color: Theme.border
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: AppContext.isMac ? "macOS style shell" : "Windows style shell"
                    color: Theme.accent
                    font.pixelSize: 13
                    font.family: Theme.fontFamily
                }
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