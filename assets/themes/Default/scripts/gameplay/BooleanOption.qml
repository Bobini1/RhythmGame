import QtQuick
import RhythmGameQml
import QtQuick.Controls.Basic

Row {
    id: booleanOption
    height: checkBox.height

    required property string prop
    property string description: prop
    required property var src

    Text {
        anchors.verticalCenter: checkBox.verticalCenter
        color: "white"
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        text: parent.description
        verticalAlignment: Text.AlignVCenter
        width: 110
    }
    CheckBox {
        id: checkBox

        checked: booleanOption.src[booleanOption.prop]

        onCheckedChanged: {
            booleanOption.src[booleanOption.prop] = checked;
            checked = Qt.binding(() => booleanOption.src[booleanOption.prop]);
        }
    }
}
