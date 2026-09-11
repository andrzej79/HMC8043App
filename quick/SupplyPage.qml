pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Main screen: connection status, one channel at a time (swipe or tab), and the
// connect / master-output controls within thumb reach at the bottom.
Page {
    id: root

    property ChannelState entryChannel: null
    property bool entryIsVoltage: true

    signal settingsRequested()

    function openEntry(channel, isVoltage) {
        entryChannel = channel
        entryIsVoltage = isVoltage
        entryLoader.active = true
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.margin
            spacing: Theme.spacing

            Label {
                text: "HMC8043"
                font.pixelSize: Theme.title
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Rectangle {
                radius: width / 2
                color: SupplyBackend.connected ? Theme.ok : SupplyBackend.connecting ? Theme.power : Theme.muted
                Accessible.ignored: true
                Layout.preferredWidth: 10
                Layout.preferredHeight: 10
            }
            Label {
                text: SupplyBackend.connected ? qsTr("Connected")
                                              : SupplyBackend.connecting ? qsTr("Connecting…") : qsTr("Offline")
                font.pixelSize: Theme.body
            }
            ToolButton {
                icon.source: "icons/settings.svg"
                Accessible.name: qsTr("Settings")
                onClicked: root.settingsRequested()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabs
            currentIndex: swipe.currentIndex
            Layout.fillWidth: true

            Repeater {
                model: SupplyBackend.channels

                TabButton {
                    required property ChannelState modelData
                    text: modelData.outputEnabled ? qsTr("CH %1 · ON").arg(modelData.number)
                                                  : qsTr("CH %1").arg(modelData.number)
                }
            }
        }

        SwipeView {
            id: swipe
            currentIndex: tabs.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            Repeater {
                model: SupplyBackend.channels

                ChannelPage {
                    required property ChannelState modelData
                    channel: modelData
                    onEditVoltageRequested: root.openEntry(modelData, true)
                    onEditCurrentRequested: root.openEntry(modelData, false)
                }
            }
        }

        PageIndicator {
            count: swipe.count
            currentIndex: swipe.currentIndex
            Layout.alignment: Qt.AlignHCenter
        }
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacing
            spacing: Theme.spacing

            Button {
                text: SupplyBackend.connected ? qsTr("Disconnect")
                                              : SupplyBackend.connecting ? qsTr("Cancel") : qsTr("Connect")
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                onClicked: {
                    if (SupplyBackend.connected || SupplyBackend.connecting)
                        SupplyBackend.disconnectDevice()
                    else
                        SupplyBackend.connectDevice()
                }
            }

            Button {
                text: SupplyBackend.masterOutputEnabled ? qsTr("Outputs ON") : qsTr("Outputs off")
                enabled: SupplyBackend.connected
                highlighted: SupplyBackend.masterOutputEnabled
                Material.accent: Theme.danger
                Accessible.name: SupplyBackend.masterOutputEnabled ? qsTr("Master output on, tap to switch off")
                                                                   : qsTr("Master output off, tap to switch on")
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                onClicked: {
                    // Switching off is the safe direction and happens at once;
                    // energising the outputs asks first.
                    if (SupplyBackend.masterOutputEnabled)
                        SupplyBackend.setMasterOutputEnabled(false)
                    else
                        confirmLoader.active = true
                }
            }
        }
    }

    Loader {
        id: entryLoader
        active: false
        sourceComponent: Component {
            ValueEntryDialog {
                title: root.entryIsVoltage ? qsTr("Channel %1 voltage").arg(root.entryChannel.number)
                                           : qsTr("Channel %1 current limit").arg(root.entryChannel.number)
                unit: root.entryIsVoltage ? "V" : "A"
                maximum: root.entryIsVoltage ? root.entryChannel.maxVoltage : root.entryChannel.maxCurrent
                presets: root.entryIsVoltage ? SupplyBackend.voltagePresets : SupplyBackend.currentPresets
                initialValue: root.entryIsVoltage ? root.entryChannel.targetVoltage : root.entryChannel.targetCurrent
                onValueAccepted: value => {
                    if (root.entryIsVoltage)
                        SupplyBackend.setChannelVoltage(root.entryChannel.number, value)
                    else
                        SupplyBackend.setChannelCurrent(root.entryChannel.number, value)
                }
                onClosed: Qt.callLater(() => entryLoader.active = false)
                Component.onCompleted: open()
            }
        }
    }

    Loader {
        id: confirmLoader
        active: false
        sourceComponent: Component {
            ConfirmDialog {
                title: qsTr("Enable outputs?")
                message: qsTr("This energises every channel whose output switch is on.")
                confirmText: qsTr("Enable")
                onConfirmed: SupplyBackend.setMasterOutputEnabled(true)
                onClosed: Qt.callLater(() => confirmLoader.active = false)
                Component.onCompleted: open()
            }
        }
    }
}
