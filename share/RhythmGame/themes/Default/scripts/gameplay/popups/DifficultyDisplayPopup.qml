import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars
    panelTitle: qsTr("Difficulty display")
    panelSubtitle: qsTr("Difficulty label visibility, layering, and font.")

    model: ObjectModel {
        BooleanOption {
            description: qsTr("Enabled")
            src: popup.themeVars
            prop: "difficultyDisplayEnabled"
        }
        NumberWithSlider {
            from: -10
            to: 10
            prop: "difficultyDisplayZ"
            text: qsTr("Z-index")
            src: popup.themeVars
        }
        FontSelection {
            src: popup.themeVars
            propertyId: "difficultyDisplayFont"
            label: qsTr("Font")
        }
    }
}
