import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import "../../common/helpers.js" as Helpers
import ".."

RowLayout {
    id: choice
    // for global vars only
    property bool assignIndex: false
    property var destination
    property string id_
    property var choices
    property var displayStrings: choices
    property alias name: strLabel.text
    property alias description: strLabel.description
    property var default_

    SettingsLabel {
        id: strLabel
    }
    ComboBox {
        id: choiceComboBox
        model: displayStrings
        Layout.fillWidth: true
        Layout.preferredWidth: 400
        Layout.minimumWidth: 200

        Binding {
            delayed: true
            choiceComboBox.currentIndex: Helpers.getIndex(
                choice.assignIndex ? Object.keys(choice.choices) : choice.choices,
                choice.destination[choice.id_],
                choiceComboBox.currentIndex) || 0;
        }
        onModelChanged: {
            choiceComboBox.currentIndex = Helpers.getIndex(
                choice.assignIndex ? Object.keys(choice.choices) : choice.choices,
                choice.destination[choice.id_],
                choiceComboBox.currentIndex) || 0;
        }

        onActivated: (_) => {
            destination[choice.id_] = choice.assignIndex ? currentIndex : choice.choices[currentIndex];
        }
    }

    ResetButton {
        destination: choice.destination
        id_: choice.id_
        default_: choice.default_
    }
}
