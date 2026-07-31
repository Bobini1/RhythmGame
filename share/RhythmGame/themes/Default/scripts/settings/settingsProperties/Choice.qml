import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import "../../common/helpers.js" as Helpers
import ".."

RowLayout {
    id: choice
    spacing: 12
    Layout.fillWidth: true
    Layout.minimumHeight: 34

    // for global vars only
    property bool assignIndex: false
    property var destination
    property string id_
    property var choices
    property var displayStrings: choices
    property alias name: strLabel.text
    property alias description: strLabel.description
    property var default_
    property var fontForIndex: null
    readonly property bool hasPerItemFonts: typeof fontForIndex === "function"

    Component {
        id: fontAwareDelegate

        ItemDelegate {
            required property var model
            required property int index

            width: ListView.view.width
            text: model[choiceComboBox.textRole]
            font: choice.fontForIndex(index)
            palette.text: choiceComboBox.palette.text
            palette.highlightedText: choiceComboBox.palette.highlightedText
            highlighted: choiceComboBox.highlightedIndex === index
            hoverEnabled: choiceComboBox.hoverEnabled
        }
    }

    SettingsLabel {
        id: strLabel
    }
    ComboBox {
        id: choiceComboBox
        model: displayStrings
        Layout.fillWidth: true
        Layout.preferredWidth: 460
        Layout.minimumWidth: 220

        Binding {
            target: choiceComboBox
            property: "delegate"
            value: fontAwareDelegate
            when: choice.hasPerItemFonts
        }

        Binding {
            target: choiceComboBox
            property: "font"
            value: choice.hasPerItemFonts && choiceComboBox.currentIndex >= 0
                ? choice.fontForIndex(choiceComboBox.currentIndex)
                : Application.font
            when: choice.hasPerItemFonts && choiceComboBox.currentIndex >= 0
        }

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
            let old = choice.destination[choice.id_];
            choice.destination[choice.id_] = choice.assignIndex ? currentIndex : choice.choices[currentIndex];
            if (old === choice.destination[choice.id_]) {
                choiceComboBox.currentIndex = Helpers.getIndex(
                    choice.assignIndex ? Object.keys(choice.choices) : choice.choices,
                    choice.destination[choice.id_],
                    choiceComboBox.currentIndex) || 0;
            }
        }
    }

    ResetButton {
        destination: choice.destination
        id_: choice.id_
        default_: choice.default_
        Layout.preferredWidth: 84
        Layout.minimumWidth: 76
    }
}
