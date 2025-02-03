import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import "../../common/helpers.js" as Helpers

ComboBox {
    id: choiceComboBox
    model: props.choices
    // for global vars only
    property bool assignIndex: false

    Binding {
        delayed: true
        choiceComboBox.currentIndex: Helpers.getIndex(choiceComboBox.assignIndex ? Object.keys(props.choices) : props.choices, destination[props.id], currentIndex);
    }

    onActivated: (_) => {
        destination[props.id] = choiceComboBox.assignIndex ? currentIndex : currentText;
    }
}