import QtQuick

// Ant Design v5 tag: small status badge.
Rectangle {
    id: root

    property string text: ""
    property string tone: "processing"   // processing | success | warning | error | default

    readonly property color fg: tone === "success" ? Theme.colorSuccess
                              : tone === "warning" ? Theme.colorWarning
                              : tone === "error" ? Theme.colorError
                              : tone === "default" ? Theme.colorTextSecondary
                              : Theme.colorPrimary
    readonly property color bg: tone === "success" ? "#52c41a22"
                              : tone === "warning" ? "#faad1422"
                              : tone === "error" ? Theme.colorErrorBg
                              : tone === "default" ? Theme.colorFillTertiary
                              : Theme.colorPrimaryBg
    readonly property color borderColor: tone === "success" ? "#52c41a55"
                                       : tone === "warning" ? "#faad1455"
                                       : tone === "error" ? "#ff6b6b55"
                                       : tone === "default" ? Theme.colorBorder
                                       : Theme.colorPrimaryBorder

    implicitWidth: label.implicitWidth + 14
    implicitHeight: 20
    radius: Theme.radiusSM
    color: bg
    border.width: 1
    border.color: borderColor

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.fg
        font.pixelSize: Theme.fontSizeSM
        font.family: Theme.fontFamily
    }
}