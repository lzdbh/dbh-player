import QtQuick
import QtQuick.Effects

// Ant Design "message" style toast: white pill, soft shadow, auto-dismiss.
Item {
    id: root

    property alias text: label.text
    property string tone: "info"   // info | success | error

    function show(message, toneName) {
        label.text = message
        tone = toneName || "info"
        opacity = 1
        hideTimer.restart()
    }

    opacity: 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: 180 } }

    width: pill.width
    height: pill.height

    readonly property color toneColor: tone === "success" ? Theme.colorSuccess
                                     : tone === "error" ? Theme.colorError
                                     : Theme.colorPrimary

    Rectangle {
        id: pill
        width: row.implicitWidth + 28
        height: 36
        radius: Theme.radiusLG
        color: Theme.colorBgElevated

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#26000000"
            shadowBlur: 0.5
            shadowVerticalOffset: 4
        }

        Row {
            id: row
            anchors.centerIn: parent
            spacing: 8

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 8
                height: 8
                radius: 4
                color: root.toneColor
            }
            Text {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.colorText
                font.pixelSize: Theme.fontSize
                font.family: Theme.fontFamily
            }
        }
    }

    Timer {
        id: hideTimer
        interval: 2600
        onTriggered: root.opacity = 0
    }
}