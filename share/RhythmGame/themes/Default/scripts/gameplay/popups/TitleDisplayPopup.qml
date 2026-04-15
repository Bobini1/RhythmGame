import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars

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
    }
}
