import QtQuick
import QtQuick.Controls

// [component]
Button {
    id: control
    required property string label
    text: control.label
    font.pixelSize: 20
    implicitHeight: 48
}
// [component]
