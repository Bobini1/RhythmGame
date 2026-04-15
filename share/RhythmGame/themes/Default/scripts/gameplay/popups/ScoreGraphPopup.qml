import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars

    model: ObjectModel {
        BooleanOption {
            description: qsTr("Enabled")
            src: popup.themeVars
            prop: "scoreGraphEnabled"
        }
        NumberWithSlider {
            from: -10
            to: 10
            prop: "scoreGraphZ"
            text: qsTr("Z-index")
            src: popup.themeVars
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "scoreGraphBarWidth"
            text: qsTr("Bar Width")
            src: popup.themeVars
        }
        ImageSelection {
            propertyId: "scoregraph"
            src: popup.themeVars
            label: qsTr("Background")
        }
    }
}
