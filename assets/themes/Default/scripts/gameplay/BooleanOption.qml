import QtQuick
import RhythmGameQml
import QtQuick.Controls.Basic

Row {
    height: checkBox.height

    required property string prop
    property string description: prop
    property bool global: false
    readonly property var src: global ? ProfileList.currentProfile.vars.globalVars : root.vars

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

        checked: src[prop]

        onCheckedChanged: {
            src[prop] = checked;
            checked = Qt.binding(() => src[prop]);
        }
    }
}
