import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// Ant Design style dropdown menu: white card, 8px radius, soft shadow.
Menu {
    id: root

    padding: 4

    background: Rectangle {
        implicitWidth: 180
        radius: Theme.radiusLG
        color: Theme.colorBgElevated
        border.width: 1
        border.color: Theme.colorBorderSecondary

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#1f000000"
            shadowBlur: 0.6
            shadowVerticalOffset: 6
        }
    }
}