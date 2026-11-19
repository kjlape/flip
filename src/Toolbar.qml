import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Pen and eraser, ink, brush size, onion skin, speed, and export.
Rectangle {
    id: bar
    property var inks: []
    signal exportRequested()
    signal helpRequested()

    implicitHeight: 56
    color: "#151518"
    Material.roundedScale: theme.radius

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 6

        ButtonGroup { id: tools }

        ToolButton {
            text: qsTr("Pen")
            checkable: true
            checked: !backend.erasing
            ButtonGroup.group: tools
            focusPolicy: Qt.NoFocus
            onClicked: backend.erasing = false
        }
        ToolButton {
            text: qsTr("Eraser")
            checkable: true
            checked: backend.erasing
            ButtonGroup.group: tools
            focusPolicy: Qt.NoFocus
            onClicked: backend.erasing = true
        }

        ToolSeparator {}

        Repeater {
            model: bar.inks
            delegate: Item {
                id: swatch
                required property string modelData
                required property int index
                readonly property bool selected: !backend.erasing && Qt.colorEqual(backend.color, modelData)
                implicitWidth: 26
                implicitHeight: 30

                // Swatches are true circles, so they keep a fixed radius.
                Rectangle {
                    anchors.centerIn: parent
                    width: 26
                    height: 26
                    radius: 13
                    color: "transparent"
                    border.width: 2
                    border.color: swatch.selected ? (swatch.index === bar.inks.length - 1 ? "#ffffff" : theme.accent) : "transparent"
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: 18
                    height: 18
                    radius: 9
                    color: swatch.modelData
                    border.width: 1
                    border.color: "#3a3a40"
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: backend.color = swatch.modelData
                }
                ToolTip.visible: hover.hovered
                ToolTip.delay: 600
                ToolTip.text: qsTr("Ink %1").arg((swatch.index + 1).toLocaleString(Qt.locale(), 'f', 0))
                HoverHandler { id: hover }
            }
        }

        ToolSeparator {}

        // The brush size; [ and ] change it too, so a narrow window drops
        // the slider and keeps the number.
        Slider {
            id: sizeSlider
            visible: bar.width > 1000
            Layout.preferredWidth: 120
            from: 1
            to: 160
            value: backend.brushSize
            focusPolicy: Qt.NoFocus
            onMoved: backend.brushSize = Math.round(value)
        }
        Label {
            Layout.minimumWidth: 48
            text: qsTr("%1 px").arg(backend.brushSize.toLocaleString(Qt.locale(), 'f', 0))
            color: "#e6e6e6"
        }

        ToolSeparator {}

        ToolButton {
            text: qsTr("Onion skin")
            checkable: true
            checked: backend.onionSkin
            focusPolicy: Qt.NoFocus
            onClicked: backend.onionSkin = checked
        }

        Item { Layout.fillWidth: true }

        Button {
            text: backend.exporting ? qsTr("Exporting…") : qsTr("Export")
            enabled: !backend.exporting
            focusPolicy: Qt.NoFocus
            onClicked: bar.exportRequested()
            // The one filled button: the way out of the app with your work.
            background: Rectangle {
                implicitHeight: 36
                radius: Math.min(4, theme.radius)
                color: theme.accent
                opacity: parent.enabled ? (parent.hovered ? 0.88 : 1) : 0.5
            }
            contentItem: Label {
                text: parent.text
                color: theme.accentForeground
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                leftPadding: 6
                rightPadding: 6
            }
        }
        ToolButton {
            text: "?"
            focusPolicy: Qt.NoFocus
            onClicked: bar.helpRequested()
        }
    }
}
