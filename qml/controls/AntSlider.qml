import QtQuick
import QtQuick.Controls

// Ant Design v5 slider: blue track, white handle with blue ring.
// `dark` adapts the rail for the video overlay control bar.
Slider {
    id: root

    property bool dark: false

    implicitHeight: 20

    background: Item {
        x: root.leftPadding
        y: root.topPadding + root.availableHeight / 2 - 2
        width: root.availableWidth
        height: 4

        Rectangle {
            anchors.fill: parent
            radius: 2
            color: root.dark ? "#33ffffff" : Theme.colorBorderSecondary
        }
        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            radius: 2
            color: root.hovered ? Theme.colorPrimaryHover : Theme.colorPrimary
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: root.pressed ? 14 : 12
        height: width
        radius: width / 2
        color: "#ffffff"
        border.width: 2
        border.color: root.pressed ? Theme.colorPrimaryActive
                      : root.hovered ? Theme.colorPrimaryHover : Theme.colorPrimary

        Behavior on width { NumberAnimation { duration: 100 } }
    }
}