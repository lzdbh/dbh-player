import QtQuick
import QtQuick.Effects

// White outline SVG colorized to a palette color, for light backgrounds.
Item {
    id: root

    property url source
    property color iconColor: Theme.colorPrimary
    property int iconSize: 20

    implicitWidth: iconSize
    implicitHeight: iconSize

    Image {
        id: img
        anchors.fill: parent
        source: root.source
        sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
        fillMode: Image.PreserveAspectFit
        visible: false
    }
    MultiEffect {
        anchors.fill: parent
        source: img
        colorization: 1.0
        colorizationColor: root.iconColor
    }
}