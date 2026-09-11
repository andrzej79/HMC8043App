import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Modal yes/no question; confirmed() fires only on the confirm button.
Dialog {
    id: root

    property alias message: body.text
    required property string confirmText

    signal confirmed()

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 2 * Theme.margin, 360)
    modal: true

    onAccepted: confirmed()

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

    footer: DialogButtonBox {
        Button {
            text: qsTr("Cancel")
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: root.confirmText
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
