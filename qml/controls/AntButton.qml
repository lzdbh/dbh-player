import QtQuick
import QtQuick.Controls

// Ant Design v5 button: primary / default / text / danger variants.
Button {
    id: root

    property string variant: "default"   // primary | default | text | danger
    property bool small: false
    property bool dark: false            // on dark player chrome

    readonly property color baseColor: variant === "primary" ? Theme.colorPrimary
                                     : variant === "danger" ? Theme.colorError
                                     : Theme.colorBgContainer
    readonly property color hoverColor: variant === "primary" ? Theme.colorPrimaryHover
                                      : variant === "danger" ? Theme.colorErrorHover
                                      : Theme.colorBgContainer
    readonly property color borderColor: variant === "primary" || variant === "danger"
                                         ? "transparent" : Theme.colorBorder
    readonly property color textColor: variant === "primary" || variant === "danger"
                                       ? "#ffffff"
                                       : dark ? "#ffffff"
                                       : hovered && variant === "default" ? Theme.colorPrimaryHover
                                       : Theme.colorText

    implicitHeight: small ? Theme.controlHeightSM : Theme.controlHeight
    implicitWidth: Math.max(implicitHeight, metrics.width + leftPadding + rightPadding + 2)
    leftPadding: small ? 7 : 15
    rightPadding: small ? 7 : 15
    topPadding: 0
    bottomPadding: 0

    TextMetrics {
        id: metrics
        font: root.font
        text: root.text
    }

    font.pixelSize: Theme.fontSize
    font.family: Theme.fontFamily

    background: Rectangle {
        radius: Theme.radius
        color: root.pressed ? (root.variant === "primary" ? Theme.colorPrimaryActive
                              : root.variant === "danger" ? Qt.darker(Theme.colorError, 1.1)
                              : root.dark ? "#33ffffff"
                              : root.variant === "text" ? Theme.colorFillSecondary
                              : Theme.colorBgContainer)
               : root.hovered ? (root.dark ? "#1affffff"
                                 : root.variant === "text" ? Theme.colorFillTertiary : root.hoverColor)
               : root.variant === "text" ? "transparent" : root.baseColor
        border.width: root.variant === "text" ? 0 : 1
        border.color: root.hovered && root.variant === "default"
                      ? Theme.colorPrimaryHover : root.borderColor
        opacity: root.enabled ? 1 : 0.5

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    contentItem: Text {
        text: root.text
        color: root.textColor
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}