import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars
    panelTitle: qsTr("Title display")
    panelSubtitle: qsTr("Song title visibility, layering, and font.")

    model: ObjectModel {
        BooleanOption {
            description: qsTr("Enabled")
            src: popup.themeVars
            prop: "titleDisplayEnabled"
        }
        NumberWithSlider {
            from: -10
            to: 10
            prop: "titleDisplayZ"
            text: qsTr("Z-index")
            src: popup.themeVars
        }
        FontSelection {
            src: popup.themeVars
            propertyId: "titleDisplayFont"
            label: qsTr("Font")
        }
    }
}
