import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    readonly property bool hostValid: SupplyBackend.isValidHostAddress(hostField.text)
    readonly property bool linkIdle: !SupplyBackend.connected && !SupplyBackend.connecting

    signal backRequested()

    title: qsTr("Settings")

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 0

            ToolButton {
                text: "‹"
                font.pixelSize: Theme.headline
                Accessible.name: qsTr("Back")
                onClicked: root.backRequested()
            }
            Label {
                text: root.title
                font.pixelSize: Theme.title
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            x: Theme.margin
            width: scroll.availableWidth - 2 * Theme.margin
            spacing: Theme.spacing

            Label {
                text: qsTr("Connection")
                color: Theme.muted
                font.pixelSize: Theme.caption
                font.capitalization: Font.AllUppercase
                Layout.topMargin: Theme.margin
            }

            TextField {
                id: hostField
                text: SupplyBackend.hostAddress
                placeholderText: qsTr("Supply IP address")
                inputMethodHints: Qt.ImhPreferNumbers | Qt.ImhNoPredictiveText
                enabled: root.linkIdle
                Accessible.name: qsTr("Supply IP address")
                Layout.fillWidth: true
            }

            Label {
                visible: hostField.text.length > 0 && !root.hostValid
                text: qsTr("Enter an IPv4 or IPv6 address")
                color: Theme.danger
                font.pixelSize: Theme.caption
            }

            Label {
                visible: !root.linkIdle
                text: qsTr("Disconnect to change the address")
                color: Theme.muted
                font.pixelSize: Theme.caption
            }

            Button {
                text: qsTr("Save address")
                enabled: root.linkIdle && root.hostValid && hostField.text.trim() !== SupplyBackend.hostAddress
                Layout.alignment: Qt.AlignRight
                onClicked: SupplyBackend.hostAddress = hostField.text.trim()
            }

            SwitchDelegate {
                text: qsTr("Connect on start")
                leftPadding: 0
                rightPadding: 0
                checked: SupplyBackend.autoConnect
                Layout.fillWidth: true
                onToggled: {
                    SupplyBackend.autoConnect = checked
                    checked = Qt.binding(() => SupplyBackend.autoConnect)
                }
            }

            Label {
                text: qsTr("Device")
                color: Theme.muted
                font.pixelSize: Theme.caption
                font.capitalization: Font.AllUppercase
                Layout.topMargin: Theme.margin
            }

            Label {
                text: SupplyBackend.deviceIdentity.length > 0 ? SupplyBackend.deviceIdentity : qsTr("Not connected")
                wrapMode: Text.WrapAnywhere
                font.pixelSize: Theme.body
                Layout.fillWidth: true
            }

            Repeater {
                model: SupplyBackend.channels

                Label {
                    required property ChannelState modelData
                    text: qsTr("Channel %1: up to %2 V, %3 A")
                            .arg(modelData.number)
                            .arg(modelData.maxVoltage.toFixed(2))
                            .arg(modelData.maxCurrent.toFixed(3))
                    color: Theme.muted
                    font.pixelSize: Theme.caption
                }
            }
        }
    }
}
