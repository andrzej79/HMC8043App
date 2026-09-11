pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    width: 400
    height: 800
    visible: true
    title: qsTr("HMC8043")

    // Android's back key closes the window; pop the settings page instead.
    onClosing: close => {
        if (stack.depth > 1) {
            close.accepted = false
            stack.pop()
        }
    }

    Component.onCompleted: {
        if (SupplyBackend.autoConnect)
            SupplyBackend.connectDevice()
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: SupplyPage {
            onSettingsRequested: stack.push(settingsComponent)
        }
    }

    Component {
        id: settingsComponent
        SettingsPage {
            onBackRequested: stack.pop()
        }
    }

    Loader {
        id: errorLoader

        property string pendingTitle
        property string pendingMessage

        active: false
        sourceComponent: Component {
            AlertDialog {
                title: errorLoader.pendingTitle
                message: errorLoader.pendingMessage
                onClosed: Qt.callLater(() => errorLoader.active = false)
                Component.onCompleted: open()
            }
        }
    }

    Connections {
        target: SupplyBackend
        function onErrorOccurred(title, message) {
            errorLoader.pendingTitle = title
            errorLoader.pendingMessage = message
            errorLoader.active = true
        }
    }
}
