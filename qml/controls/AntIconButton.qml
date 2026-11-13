import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// Icon button using white outline SVGs colorized to any palette color.
Button {
    id: root

    property url iconSource
    property color iconColor: "#ffffff"
    property int iconSize: 20
    property bool dark: true

    implicitWidth: 36
    implicitHeight: 36

    Accessible.role: Accessible.Button
    Accessible.name: root.ToolTip.text

    background: Rectangle {
        radius: Theme.radius
        color: root.pressed ? (root.dark ? "#33ffffff" : Theme.colorFillSecondary)
               : root.hovered ? (root.dark ? "#1affffff" : Theme.colorFillTertiary)
               : "transparent"
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    contentItem: Item {
        Image {
            id: iconImage
            anchors.centerIn: parent
            width: root.iconSize
            height: root.iconSize
            source: root.iconSource
            sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
            fillMode: Image.PreserveAspectFit
            visible: false
        }
        MultiEffect {
            anchors.fill: iconImage
            source: iconImage
            colorization: 1.0
            colorizationColor: root.iconColor
            opacity: root.enabled ? 1 : 0.4
        }
    }
}