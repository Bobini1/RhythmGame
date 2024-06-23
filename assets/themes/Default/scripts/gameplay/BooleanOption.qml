import QtQuick
import RhythmGameQml
import QtQuick.Controls.Basic

Row {
    height: checkBox.height

    required property string prop
    property string description: prop

    Text {
        anchors.verticalCenter: checkBox.verticalCenter
        color: "white"
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        text: parent.description
        verticalAlignment: Text.AlignVCenter
        width: 100
    }
    CheckBox {
        id: checkBox

        checked: root.vars[prop]

        onCheckedChanged: {
            root.vars[prop] = checked;
            checked = Qt.binding(() => root.vars[prop]);
        }
    }
}
