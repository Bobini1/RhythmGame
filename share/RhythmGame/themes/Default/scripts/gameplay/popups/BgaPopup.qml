import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var generalVars
    required property var themeVars

    model: ObjectModel {
        BooleanOption {
            description: qsTr("BGA Enabled")
            src: popup.generalVars
            prop: "bgaOn"
        }
        NumberWithSlider {
            from: -10
            src: popup.themeVars
            prop: "bgaZ"
            text: qsTr("Z-index")
            to: 10
        }
    }
}