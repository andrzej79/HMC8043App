import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Modal message with a single acknowledge button.
Dialog {
    id: root

    property alias message: body.text

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 2 * Theme.margin, 360)
    modal: true

    // Wrapped text sits in a layout: a bare wrapping Label as the popup's content
    // makes Material's implicitHeight binding depend on itself (height follows width).
    ColumnLayout {
        width: root.availableWidth

        Label {
            id: body
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.body
            Layout.fillWidth: true
        }
    }

    // Explicit buttons rather than standardButtons: those carry Qt's own
    // system-locale translations instead of the application's.
    footer: DialogButtonBox {
        Button {
            text: qsTr("OK")
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
