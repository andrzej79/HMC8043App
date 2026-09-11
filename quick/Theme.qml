pragma Singleton
import QtQuick

QtObject {
    // Type scale: base 16, perfect fourth (1.333). Readouts sit far above the
    // body steps because the phone may lie on the bench, read from arm's length.
    readonly property int caption: 12
    readonly property int body: 16
    readonly property int title: 21
    readonly property int headline: 28
    readonly property int readoutMedium: 38
    readonly property int readoutLarge: 64

    readonly property int spacing: 8
    readonly property int margin: 16
    readonly property int touchTarget: 48

    // Measured-quantity colours: the desktop LCD hues, lifted for contrast on dark.
    readonly property color voltage: "#66bb6a"
    readonly property color current: "#ef5350"
    readonly property color power: "#ffa726"

    readonly property color danger: "#ef5350"
    readonly property color ok: "#66bb6a"
    readonly property color muted: "#9e9e9e"
    readonly property color placeholder: "#757575"
    readonly property color panel: "#2c2c2c"
}
