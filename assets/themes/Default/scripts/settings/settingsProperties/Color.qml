import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick

Rectangle {
    id: colorPicker

    height: 30
    border {
        color: "black"
        width: 1
    }
    property var destination
    property string id_
    property string name

    color: destination[colorPicker.id_]

    MouseArea {
        anchors.fill: parent
        onClicked: colorDialog.open()
    }

    ColorDialog {
        id: colorDialog

        selectedColor: colorPicker.destination[colorPicker.id_]
        title: colorPicker.name

        onAccepted: {
            colorPicker.destination[colorPicker.id_] = colorDialog.selectedColor;
            colorDialog.selectedColor = Qt.binding(() => destination[colorPicker.id_])
        }
    }
}
