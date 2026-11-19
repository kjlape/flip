import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Every shortcut, shown with ?.
Rectangle {
    id: overlay
    signal dismissed()
    color: "#cc0b0b0d"

    readonly property var keys: [
        ["Space", qsTr("Play or pause")],
        ["B / E", qsTr("Pen / eraser")],
        [qsTr("Right-click"), qsTr("Erase while drawing")],
        ["1 – 9", qsTr("Pick an ink")],
        ["[ / ]", qsTr("Smaller / bigger brush")],
        ["O", qsTr("Show the frames before and after")],
        ["← / →", qsTr("Previous / next frame")],
        ["Home / End", qsTr("First / last frame")],
        ["N", qsTr("New frame after this one")],
        ["D", qsTr("Duplicate this frame")],
        ["X / Delete", qsTr("Delete this frame")],
        ["Backspace", qsTr("Clear this frame")],
        ["Ctrl+← / →", qsTr("Move this frame")],
        ["− / +", qsTr("Slower / faster")],
        ["Ctrl+Z", qsTr("Undo")],
        ["Ctrl+Shift+Z", qsTr("Redo")],
        ["Ctrl+N", qsTr("New animation")],
        ["Ctrl+O", qsTr("Open")],
        ["Ctrl+S", qsTr("Save")],
        ["Ctrl+Shift+S", qsTr("Save as")],
        ["Ctrl+E", qsTr("Export GIF or MP4")],
        ["?", qsTr("Show or hide these shortcuts")],
        ["Q / Ctrl+Q", qsTr("Quit")]
    ]

    MouseArea {
        anchors.fill: parent
        onClicked: overlay.dismissed()
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, grid.implicitWidth + 64)
        height: Math.min(parent.height - 48, grid.implicitHeight + heading.implicitHeight + 64)
        radius: theme.radius
        color: "#18181c"
        border.color: "#2c2c33"

        // Clicks inside the panel don't close it.
        MouseArea { anchors.fill: parent }

        Label {
            id: heading
            x: 24
            y: 24
            text: qsTr("Shortcuts")
            font.pixelSize: 20
            font.bold: true
            color: "#ffffff"
        }

        // A short window scrolls the list rather than cutting it off.
        Flickable {
            anchors.fill: parent
            anchors.topMargin: heading.y + heading.height + 16
            anchors.leftMargin: 24
            anchors.rightMargin: 16
            anchors.bottomMargin: 16
            contentWidth: grid.implicitWidth
            contentHeight: grid.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            GridLayout {
                id: grid
                columns: overlay.width > 900 ? 4 : 2
                columnSpacing: 20
                rowSpacing: 8

                Repeater {
                    model: overlay.keys.length * 2
                    delegate: Label {
                        required property int index
                        readonly property var entry: overlay.keys[Math.floor(index / 2)]
                        readonly property bool isKey: index % 2 === 0
                        text: isKey ? entry[0] : entry[1]
                        color: isKey ? "#ffffff" : "#b8b8c0"
                        font.family: isKey ? "monospace" : Qt.application.font.family
                        font.bold: isKey
                        Layout.alignment: isKey ? Qt.AlignRight : Qt.AlignLeft
                    }
                }
            }
        }
    }
}
