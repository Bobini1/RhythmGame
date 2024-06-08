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
    }
    Component.onCompleted: {
        let index = 0;
        for (let choice of props.choices) {
            if (destination[props.id] === choice) {
                currentIndex = index;
                break;
            }
            index++;
        }
    }
    onAccepted: (index) => {
        destination[props.id] = choiceComboBox.currentText;
    }
}