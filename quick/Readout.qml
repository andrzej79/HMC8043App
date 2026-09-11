import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One measured quantity: caption, large tabular value and unit.
// Shows a placeholder until the instrument has reported a value.
ColumnLayout {
    id: root

    required property string label
    required property color accent
    property bool valid: false
    property string valueText: ""
    property string unit: ""
    property int valueSize: Theme.readoutLarge

    spacing: 0

    Accessible.role: Accessible.StaticText
    Accessible.name: valid ? qsTr("%1: %2 %3").arg(label).arg(valueText).arg(unit)
                           : qsTr("%1: no reading").arg(label)

    Label {
        text: root.label
        font.pixelSize: Theme.caption
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 1
        color: root.accent
        Accessible.ignored: true
    }

    RowLayout {
        spacing: Theme.spacing

        Label {
            text: root.valid ? root.valueText : "-.---"
            font.pixelSize: root.valueSize
            font.features: ({ "tnum": 1 })
            color: root.valid ? root.accent : Theme.placeholder
            Layout.alignment: Qt.AlignBaseline
            Accessible.ignored: true
        }

        Label {
            text: root.unit
            font.pixelSize: Theme.title
            color: root.valid ? root.accent : Theme.placeholder
            Layout.alignment: Qt.AlignBaseline
            Accessible.ignored: true
        }
    }
}
