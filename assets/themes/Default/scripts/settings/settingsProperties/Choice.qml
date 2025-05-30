import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import "../../common/helpers.js" as Helpers

ComboBox {
    id: choiceComboBox
    model: choices
    // for global vars only
    property bool assignIndex: false
    property var destination
    property string id_
    property var choices

    Binding {
        delayed: true
        choiceComboBox.currentIndex: Helpers.getIndex(
            choiceComboBox.assignIndex ? Object.keys(choiceComboBox.choices) : choiceComboBox.choices,
            choiceComboBox.destination[choiceComboBox.id_],
            choiceComboBox.currentIndex);
    }

    onActivated: (_) => {
        destination[choiceComboBox.id_] = choiceComboBox.assignIndex ? currentIndex : currentText;
    }
}