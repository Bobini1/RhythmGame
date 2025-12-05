import QtQuick
import RhythmGameQml
import QtQuick.Controls

Row {
    id: booleanOption

    required property string prop
    property string description: prop
    required property var src
    height: Math.max(40, checkBox.implicitHeight)
    spacing: 10

    Text {
        anchors.verticalCenter: checkBox.verticalCenter
        color: "white"
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        text: parent.description
        verticalAlignment: Text.AlignVCenter
        width: 160
        fontSizeMode: Text.Fit
    }
    CheckBox {
        id: checkBox

        checked: booleanOption.src[booleanOption.prop]
        width: 310

        onCheckedChanged: {
            booleanOption.src[booleanOption.prop] = checked;
        }
    }
}
