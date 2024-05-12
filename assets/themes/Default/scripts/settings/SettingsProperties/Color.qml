import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick

Rectangle {
    height: 30
    color: "black"
    Rectangle {
        id: colorMenuItem
        anchors.fill: parent
        anchors.leftMargin: 1
        anchors.rightMargin: 1
        anchors.topMargin: 1
        anchors.bottomMargin: 1

        color: destination[props.id]
    }

    MouseArea {
        anchors.fill: parent
        onClicked: colorDialog.open()
    }

    ColorDialog {
        id: colorDialog

        selectedColor: destination[props.id]
        title: props.id

        onAccepted: {
            destination[props.id] = colorDialog.selectedColor;
        }
    }
}
