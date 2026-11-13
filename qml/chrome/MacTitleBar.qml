import QtQuick

// macOS-style title bar: traffic lights on the left, centered title.
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

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 13
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Repeater {
            model: [
                { "c": "#ff5f57", "act": "close" },
                { "c": "#febc2e", "act": "min" },
                { "c": "#28c840", "act": "zoom" }
            ]
            delegate: Rectangle {
                required property var modelData
                width: 12
                height: 12
                radius: 6
                color: modelData.c
                border.width: 1
                border.color: Qt.darker(modelData.c, 1.15)

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (modelData.act === "close")
                            root.window.close()
                        else if (modelData.act === "min")
                            root.window.showMinimized()
                        else
                            root.window.toggleMaximized()
                    }
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: root.title
        color: Theme.text
        font.pixelSize: 13
        font.family: Theme.fontFamily
    }
}