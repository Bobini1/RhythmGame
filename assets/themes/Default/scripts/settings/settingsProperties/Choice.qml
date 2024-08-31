import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml

ComboBox {
    id: choiceComboBox
    model: props.choices

    palette {
        window: "white"
        light: palette.button
    }

    function getIndex(text) {
        let index = 0;
        for (let choice of props.choices) {
            if (text === choice) {
                return index;
                break;
            }
            index++;
        }
    }

    currentIndex: getIndex(destination[props.id]);
    onActivated: (_) => {
        destination[props.id] = currentText;
    }
}