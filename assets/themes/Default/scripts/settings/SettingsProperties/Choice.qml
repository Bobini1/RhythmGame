import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml

ComboBox {
    id: choiceComboBox
    model: props.choices
    delegate: ItemDelegate {
        text: modelData
        width: parent.width
        Component.onCompleted: {
            if (destination[props.id] === modelData) {
                choiceComboBox.currentIndex = index;
            }
        }
    }
    onCurrentIndexChanged: {
        destination[props.id] = currentText;
    }
}