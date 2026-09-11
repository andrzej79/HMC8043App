import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One channel: live readings first, then its set-points and output switch.
ScrollView {
    id: root

    required property ChannelState channel
    readonly property bool currentInMilliamps: channel.current < 1.0

    signal editVoltageRequested()
    signal editCurrentRequested()

    contentWidth: availableWidth

    ColumnLayout {
        x: Theme.margin
        width: root.availableWidth - 2 * Theme.margin
        spacing: Theme.margin

        Item {
            Layout.preferredHeight: Theme.spacing
        }

        Readout {
            label: qsTr("Voltage")
            accent: Theme.voltage
            valid: root.channel.hasVoltage
            valueText: root.channel.voltage.toFixed(3)
            unit: "V"
        }

        Readout {
            label: qsTr("Current")
            accent: Theme.current
            valid: root.channel.hasCurrent
            valueText: root.currentInMilliamps ? (root.channel.current * 1000).toFixed(1)
                                               : root.channel.current.toFixed(3)
            unit: root.currentInMilliamps ? "mA" : "A"
        }

        Readout {
            label: qsTr("Power")
            accent: Theme.power
            valid: root.channel.hasPower
            valueText: root.channel.power.toFixed(2)
            unit: "W"
            valueSize: Theme.readoutMedium
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: Theme.spacing
            Layout.fillWidth: true

            Label {
                text: qsTr("Set voltage")
                font.pixelSize: Theme.body
                Layout.fillWidth: true
            }
            Button {
                text: root.channel.hasTargetVoltage ? qsTr("%1 V").arg(root.channel.targetVoltage.toFixed(3)) : "—"
                enabled: SupplyBackend.connected
                Accessible.name: qsTr("Set voltage, currently %1").arg(text)
                Layout.minimumWidth: 140
                onClicked: root.editVoltageRequested()
            }
        }

        RowLayout {
            spacing: Theme.spacing
            Layout.fillWidth: true

            Label {
                text: qsTr("Current limit")
                font.pixelSize: Theme.body
                Layout.fillWidth: true
            }
            Button {
                text: root.channel.hasTargetCurrent ? qsTr("%1 A").arg(root.channel.targetCurrent.toFixed(3)) : "—"
                enabled: SupplyBackend.connected
                Accessible.name: qsTr("Current limit, currently %1").arg(text)
                Layout.minimumWidth: 140
                onClicked: root.editCurrentRequested()
            }
        }

        RowLayout {
            spacing: Theme.spacing
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true

                Label {
                    text: root.channel.outputEnabled ? qsTr("Output ON") : qsTr("Output off")
                    font.pixelSize: Theme.title
                    font.bold: root.channel.outputEnabled
                    color: root.channel.outputEnabled ? Theme.danger : Theme.muted
                }
                Label {
                    visible: root.channel.outputEnabled && !SupplyBackend.masterOutputEnabled
                    text: qsTr("Master output is off — nothing is energised yet")
                    color: Theme.muted
                    font.pixelSize: Theme.caption
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            Switch {
                checked: root.channel.outputEnabled
                enabled: SupplyBackend.connected
                Accessible.name: qsTr("Channel %1 output").arg(root.channel.number)
                onToggled: {
                    SupplyBackend.setChannelOutputEnabled(root.channel.number, checked)
                    // Show what the instrument reports, not what was tapped.
                    checked = Qt.binding(() => root.channel.outputEnabled)
                }
            }
        }
    }
}
