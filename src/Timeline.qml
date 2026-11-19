import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Play, every frame as a thumbnail, and a button for the next one.
Rectangle {
    id: timeline
    implicitHeight: 124
    color: "#151518"
    Material.roundedScale: theme.radius

    readonly property real thumbHeight: 68
    readonly property real thumbWidth: thumbHeight * backend.canvasSize.width / Math.max(1, backend.canvasSize.height)

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // Drawn rather than typed, so the symbols don't depend on the font.
        // A true circle, so it keeps its round shape whatever the theme.
        Rectangle {
            id: play
            Layout.preferredWidth: 52
            Layout.preferredHeight: 52
            radius: 26
            color: playArea.containsMouse ? Qt.lighter(theme.accent, 1.15) : theme.accent

            Canvas {
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: backend.playing ? 0 : 2
                width: 18
                height: 18
                property bool playing: backend.playing
                property color ink: theme.accentForeground
                onPlayingChanged: requestPaint()
                onInkChanged: requestPaint()
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.reset()
                    ctx.fillStyle = ink
                    if (playing) {
                        ctx.fillRect(2, 1, 5, 16)
                        ctx.fillRect(11, 1, 5, 16)
                    } else {
                        ctx.beginPath()
                        ctx.moveTo(3, 1)
                        ctx.lineTo(17, 9)
                        ctx.lineTo(3, 17)
                        ctx.closePath()
                        ctx.fill()
                    }
                }
            }
            MouseArea {
                id: playArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: backend.togglePlay()
            }
            ToolTip.visible: playArea.containsMouse
            ToolTip.delay: 600
            ToolTip.text: backend.playing ? qsTr("Pause") : qsTr("Play")
        }

        ListView {
            id: strip
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            spacing: 8
            clip: true
            model: backend.frames
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

            Connections {
                target: backend
                function onCurrentFrameChanged() { strip.positionViewAtIndex(backend.currentFrame, ListView.Contain) }
            }

            delegate: Item {
                id: cell
                required property int index
                required property string version
                readonly property bool current: index === backend.currentFrame
                width: timeline.thumbWidth + 6
                height: strip.height

                Rectangle {
                    id: frameBox
                    y: 0
                    width: parent.width
                    height: timeline.thumbHeight + 6
                    radius: Math.min(4, theme.radius)
                    color: "transparent"
                    border.width: cell.current ? 3 : 1
                    border.color: cell.current ? theme.accentInk : "#33333a"

                    Image {
                        anchors.fill: parent
                        anchors.margins: 3
                        source: "image://frames/" + cell.version
                        sourceSize.width: Math.round(timeline.thumbWidth * 2)
                        sourceSize.height: Math.round(timeline.thumbHeight * 2)
                        fillMode: Image.PreserveAspectFit
                        cache: true
                    }
                }
                Label {
                    anchors.top: frameBox.bottom
                    anchors.topMargin: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: (cell.index + 1).toLocaleString(Qt.locale(), 'f', 0)
                    font.pixelSize: 12
                    color: cell.current ? "#ffffff" : "#8a8a92"
                }
                MouseArea {
                    anchors.fill: frameBox
                    cursorShape: Qt.PointingHandCursor
                    onClicked: backend.goTo(cell.index)
                }
            }

            footer: Item {
                width: timeline.thumbWidth + 14
                height: strip.height
                Button {
                    x: 8
                    width: timeline.thumbWidth
                    height: timeline.thumbHeight + 6
                    topInset: 0
                    bottomInset: 0
                    flat: true
                    text: "+"
                    font.pixelSize: 26
                    focusPolicy: Qt.NoFocus
                    onClicked: {
                        backend.goTo(backend.frameCount - 1)
                        backend.newFrame()
                    }
                    ToolTip.visible: hovered
                    ToolTip.delay: 600
                    ToolTip.text: qsTr("Add a frame at the end")
                }
            }
        }

        ColumnLayout {
            spacing: 2
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Frame %1 of %2")
                    .arg((backend.currentFrame + 1).toLocaleString(Qt.locale(), 'f', 0))
                    .arg(backend.frameCount.toLocaleString(Qt.locale(), 'f', 0))
                color: "#9a9aa2"
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 0
                ToolButton {
                    text: "−"
                    focusPolicy: Qt.NoFocus
                    enabled: backend.fps > 1
                    onClicked: backend.fps -= 1
                }
                Label {
                    Layout.minimumWidth: 56
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("%1 fps").arg(backend.fps.toLocaleString(Qt.locale(), 'f', 0))
                    color: "#e6e6e6"
                }
                ToolButton {
                    text: "+"
                    focusPolicy: Qt.NoFocus
                    enabled: backend.fps < 60
                    onClicked: backend.fps += 1
                }
            }
        }
    }
}
