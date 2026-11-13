import QtQuick

// Windows Fluent-style title bar: left title, caption buttons on the right.
Rectangle {
    id: root

    required property var window
    property string title: ""

    height: Theme.titleBarHeight
    color: Theme.titleBarBg

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: root.window.startSystemMove()
        onDoubleClicked: root.window.toggleMaximized()
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        color: Theme.text
        font.pixelSize: 12
        font.family: Theme.fontFamily
    }

    Row {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        Repeater {
            model: [
                { "glyph": "\uE921", "act": "min", "hover": Theme.colorFillSecondary },
                { "glyph": "\uE922", "act": "max", "hover": Theme.colorFillSecondary },
                { "glyph": "\uE8BB", "act": "close", "hover": Theme.colorError }
            ]
            delegate: Rectangle {
                required property var modelData
                width: 46
                height: root.height
                color: ma.containsMouse ? modelData.hover : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: modelData.act === "max"
                          ? (root.window.visibility === Window.Maximized ? "\uE923" : "\uE922")
                          : modelData.glyph
                    font.family: "Segoe MDL2 Assets"
                    font.pixelSize: 10
                    color: (modelData.act === "close" && ma.containsMouse) ? "#ffffff" : Theme.text
                }

                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (modelData.act === "min")
                            root.window.showMinimized()
                        else if (modelData.act === "close")
                            root.window.close()
                        else
                            root.window.toggleMaximized()
                    }
                }
            }
        }
    }
}