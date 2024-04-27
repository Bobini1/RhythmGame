import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml

MenuItem {
    id: colorMenuItem

    text: qsTr("Color")

    onTriggered: colorDialog.open()

    ColorDialog {
        id: colorDialog

        title: props.id

        onAccepted: {
            //destination[props.id] = colorDialog.color;
            console.info(destination[props.id]);
        }
    }
}
