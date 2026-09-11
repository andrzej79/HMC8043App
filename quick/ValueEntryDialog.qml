pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Numeric entry for a set-point: typed value or a preset, bounded by the
// limit the instrument reported for this channel.
Dialog {
    id: root

    required property string unit
    required property real maximum
    required property var presets
    required property real initialValue
    property int decimals: 3

    readonly property real parsedValue: SupplyBackend.parseNumber(field.text)
    readonly property bool inRange: !isNaN(parsedValue) && parsedValue >= 0 && parsedValue <= maximum

    signal valueAccepted(real value)

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 2 * Theme.margin, 360)
    modal: true

    onOpened: {
        field.text = initialValue.toFixed(decimals)
        field.selectAll()
        field.forceActiveFocus()
    }
    onAccepted: {
        if (inRange)
            valueAccepted(parsedValue)
    }

    ColumnLayout {
        width: root.availableWidth
        spacing: Theme.spacing

        Label {
            text: qsTr("Range 0 … %1 %2").arg(root.maximum.toFixed(root.decimals)).arg(root.unit)
            color: Theme.muted
            font.pixelSize: Theme.caption
        }

        RowLayout {
            spacing: Theme.spacing
            Layout.fillWidth: true

            TextField {
                id: field
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                font.pixelSize: Theme.headline
                Accessible.name: root.title
                Layout.fillWidth: true
                onAccepted: {
                    if (root.inRange)
                        root.accept()
                }
            }

            Label {
                text: root.unit
                font.pixelSize: Theme.title
            }
        }

        Label {
            visible: field.text.length > 0 && !root.inRange
            text: isNaN(root.parsedValue) ? qsTr("Not a number") : qsTr("Outside the instrument's range")
            color: Theme.danger
            font.pixelSize: Theme.caption
        }

        Label {
            text: qsTr("Presets")
            color: Theme.muted
            font.pixelSize: Theme.caption
            Layout.topMargin: Theme.spacing
        }

        Flow {
            spacing: Theme.spacing
            Layout.fillWidth: true

            Repeater {
                model: root.presets.filter(p => p <= root.maximum)

                Button {
                    required property real modelData
                    text: qsTr("%1 %2").arg(modelData).arg(root.unit)
                    onClicked: {
                        root.valueAccepted(modelData)
                        root.close()
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Cancel")
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: qsTr("Set")
            enabled: root.inRange
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
