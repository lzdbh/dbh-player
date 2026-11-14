import QtQuick
import QtQuick.Controls

// Ant Design style menu item: 32px row, hover fill, blue check.
MenuItem {
    id: root

    // The custom contentItem below draws its own checkmark; disable the
    // default indicator so a checked item doesn't show two checkmarks.
    indicator: null

    implicitHeight: 32
    padding: 0
    leftPadding: 12
    rightPadding: 12

    background: Rectangle {
        radius: Theme.radiusSM
        color: root.highlighted ? Theme.colorFillTertiary : "transparent"
    }

    contentItem: Item {
        implicitWidth: label.implicitWidth + 28
        implicitHeight: 32

        Text {
            id: check
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.verticalCenter: parent.verticalCenter
            text: "✓"
            color: Theme.colorPrimary
            font.pixelSize: Theme.fontSizeSM
            visible: root.checked
            width: 16
        }
        Text {
            id: label
            anchors.left: check.right
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: root.enabled ? Theme.colorText : Theme.colorTextQuaternary
            font.pixelSize: Theme.fontSize
            font.family: Theme.fontFamily
            elide: Text.ElideRight
        }
    }
}