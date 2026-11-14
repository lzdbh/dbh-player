import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// Ant Design style dropdown menu: dark card, 8px radius, soft shadow.
Menu {
    id: root

    padding: 4

    // Qt generates the submenu entry (for a child Menu with a title) with the
    // default style, ignoring delegates. Restyle it to match AntMenuItem.
    onOpened: Qt.callLater(restyleSubmenuEntries)

    function restyleSubmenuEntries() {
        for (let i = 0; i < count; ++i) {
            const item = itemAt(i)
            if (!item || !item.subMenu || (item.background && item.background.ant))
                continue
            item.background = antBackground.createObject(item)
            item.arrow = antArrow.createObject(item)
            item.padding = 0
            item.implicitHeight = 32
            item.leftPadding = 28
            item.rightPadding = 14
            item.spacing = 0
            item.font.pixelSize = Theme.fontSize
            item.font.family = Theme.fontFamily
        }
    }

    Component {
        id: antBackground
        Rectangle {
            property bool ant: true
            radius: Theme.radiusSM
            color: parent.highlighted ? Theme.colorFillTertiary : "transparent"
        }
    }

    Component {
        id: antArrow
        Text {
            text: "›"
            color: Theme.colorTextTertiary
            font.pixelSize: Theme.fontSizeLG
            font.family: Theme.fontFamily
            x: parent.width - width - 14
            y: (parent.height - height) / 2
        }
    }

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