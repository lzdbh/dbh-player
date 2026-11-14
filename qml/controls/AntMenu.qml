import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// Ant Design style dropdown menu: white card, 8px radius, soft shadow.
Menu {
    id: root

    padding: 4

    // Light palette so internally-created entries (e.g. a nested submenu row,
    // which Qt renders with the default MenuItem) are readable on dark.
    palette.windowText: Theme.colorText
    palette.text: Theme.colorText
    palette.buttonText: Theme.colorText
    palette.window: Theme.colorBgElevated
    palette.base: Theme.colorBgElevated
    palette.button: Theme.colorBgElevated
    palette.highlight: Theme.colorFillTertiary
    palette.highlightedText: Theme.colorText
    palette.mid: Theme.colorTextSecondary
    palette.midlight: Theme.colorTextSecondary

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