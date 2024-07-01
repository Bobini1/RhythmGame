import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick

Rectangle {
    height: 30
    border {
        color: "black"
        width: 1
    }

    color: destination[props.id]

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
            colorDialog.selectedColor = Qt.binding(() => destination[props.id])
        }
    }
}
