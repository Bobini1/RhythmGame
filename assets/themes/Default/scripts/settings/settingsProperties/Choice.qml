import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import "../../common/helpers.js" as Helpers

ComboBox {
    id: choiceComboBox
    model: props.choices

    palette {
        window: "white"
        light: palette.button
    }

    Binding {
        delayed: true
        choiceComboBox.currentIndex: Helpers.getIndex(props.choices, destination[props.id], currentIndex);
    }

    onActivated: (_) => {
        destination[props.id] = currentText;
    }
}