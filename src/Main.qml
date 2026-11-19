import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Flip

ApplicationWindow {
    id: win
    width: 1200
    height: 820
    minimumWidth: 880
    minimumHeight: 540
    visible: true
    title: (backend.dirty ? "• " : "") + backend.title + " — Flip"
    color: "#0e0e10"

    Material.theme: Material.Dark
    // Checked buttons, sliders, and focus rings sit on dark chrome.
    Material.accent: theme.accentInk
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    // What to do once the open animation is saved or discarded: "quit",
    // "new", or "open".
    property string pendingAction: ""
    property bool closeConfirmed: false
    readonly property bool modal: discardDialog.visible
    readonly property var inks: ["#1a1a1a", "#ffffff", "#e5484d", "#f76b15", "#ffc53d",
                                 "#46a758", "#0090ff", "#8e4ec6", theme.accent]

    function whenSafe(action) {
        if (backend.dirty) {
            pendingAction = action
            discardDialog.open()
        } else {
            run(action)
        }
    }

    function run(action) {
        pendingAction = ""
        if (action === "quit") {
            closeConfirmed = true
            win.close()
        } else if (action === "new") {
            backend.newAnimation()
        } else if (action === "open") {
            backend.open()
        }
    }

    onClosing: (close) => {
        if (backend.dirty && !closeConfirmed) {
            close.accepted = false
            whenSafe("quit")
        }
    }

    Connections {
        target: backend
        function onSaved() {
            if (win.pendingAction !== "")
                win.run(win.pendingAction)
        }
    }

    Shortcut { sequences: ["?", "Shift+?"]; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; enabled: win.helpVisible; onActivated: win.helpVisible = false }
    // Q is Omarchy's habit; Ctrl+Q is every other desktop's.
    Shortcut { sequences: ["Q", "Ctrl+Q"]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: win.whenSafe("quit") }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.togglePlay() }
    Shortcut { sequences: [StandardKey.Undo]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.undo() }
    Shortcut { sequences: ["Ctrl+Shift+Z", "Ctrl+Y"]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.redo() }
    Shortcut { sequence: "B"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.erasing = false }
    Shortcut { sequence: "E"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.erasing = true }
    Shortcut { sequence: "["; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.growBrush(-1) }
    Shortcut { sequence: "]"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.growBrush(1) }
    Shortcut { sequence: "O"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.onionSkin = !backend.onionSkin }
    Shortcut { sequence: "N"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.newFrame() }
    Shortcut { sequence: "D"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.duplicateFrame() }
    Shortcut { sequences: ["Delete", "X"]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.deleteFrame() }
    Shortcut { sequence: "Backspace"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.clearFrame() }
    Shortcut { sequences: ["Left", ","]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.step(-1) }
    Shortcut { sequences: ["Right", "."]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.step(1) }
    Shortcut { sequence: "Home"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.goTo(0) }
    Shortcut { sequence: "End"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.goTo(backend.frameCount - 1) }
    Shortcut { sequence: "Ctrl+Left"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.moveFrame(-1) }
    Shortcut { sequence: "Ctrl+Right"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.moveFrame(1) }
    Shortcut { sequences: ["+", "="]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.fps += 1 }
    Shortcut { sequence: "-"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: backend.fps -= 1 }
    Shortcut { sequences: [StandardKey.Save]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: { win.pendingAction = ""; backend.save() } }
    Shortcut { sequence: "Ctrl+Shift+S"; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: { win.pendingAction = ""; backend.saveAs() } }
    Shortcut { sequences: [StandardKey.Open]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: win.whenSafe("open") }
    Shortcut { sequences: [StandardKey.New]; context: Qt.ApplicationShortcut; enabled: !win.modal; onActivated: win.whenSafe("new") }
    Shortcut { sequence: "Ctrl+E"; context: Qt.ApplicationShortcut; enabled: !win.modal && !backend.exporting; onActivated: backend.exportAnimation() }
    Repeater {
        model: win.inks.length
        delegate: Item {
            required property int index
            Shortcut {
                sequence: String(index + 1)
                context: Qt.ApplicationShortcut
                enabled: !win.modal
                onActivated: backend.color = win.inks[index]
            }
        }
    }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Toolbar {
                Layout.fillWidth: true
                inks: win.inks
                onExportRequested: backend.exportAnimation()
                onHelpRequested: win.helpVisible = !win.helpVisible
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                DrawingSurface {
                    id: surface
                    anchors.fill: parent
                    // Not "backend: backend", which would bind the property to itself.
                    animation: backend
                }

                Rectangle {
                    visible: backend.error !== ""
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: 12
                    width: Math.min(parent.width - 32, errorRow.implicitWidth + 32)
                    height: errorRow.implicitHeight + 20
                    radius: theme.radius
                    color: "#3b1a1d"
                    border.color: "#e5484d"

                    RowLayout {
                        id: errorRow
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 8
                        spacing: 12
                        Label {
                            Layout.fillWidth: true
                            Layout.maximumWidth: 720
                            text: backend.error
                            wrapMode: Text.Wrap
                            color: "#ffd6d8"
                        }
                        ToolButton {
                            text: qsTr("Dismiss")
                            focusPolicy: Qt.NoFocus
                            onClicked: backend.dismissError()
                        }
                    }
                }

                Rectangle {
                    visible: backend.status !== ""
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottomMargin: 14
                    width: statusRow.implicitWidth + 28
                    height: 34
                    radius: Math.min(17, theme.radius)
                    color: "#222226"

                    RowLayout {
                        id: statusRow
                        anchors.centerIn: parent
                        spacing: 10
                        BusyIndicator {
                            visible: backend.exporting
                            running: visible
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                        }
                        Label {
                            text: backend.status
                            color: "#e6e6e6"
                        }
                    }
                }
            }

            Timeline {
                Layout.fillWidth: true
            }
        }

        HelpOverlay {
            anchors.fill: parent
            visible: win.helpVisible
            onDismissed: win.helpVisible = false
        }
    }

    Dialog {
        id: discardDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("Save changes to %1?").arg(backend.title)
        Material.roundedScale: theme.radius
        Material.accent: theme.accentInk

        Label {
            text: qsTr("Your drawings will be lost if you don't save them.")
            wrapMode: Text.Wrap
            width: Math.min(420, win.width - 80)
        }

        footer: DialogButtonBox {
            Button {
                text: qsTr("Cancel")
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            Button {
                text: qsTr("Don't Save")
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                onClicked: {
                    discardDialog.close()
                    win.run(win.pendingAction)
                }
            }
            Button {
                text: qsTr("Save")
                highlighted: true
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }

        // Saving carries on through onSaved: at once for a file already on
        // disk, or after the picker for a new animation.
        onAccepted: backend.save()
        onRejected: win.pendingAction = ""
    }
}
