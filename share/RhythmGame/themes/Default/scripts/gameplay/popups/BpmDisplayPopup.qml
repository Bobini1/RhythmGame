import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars

    model: ObjectModel {
        BooleanOption {
            description: qsTr("Enabled")
            src: popup.themeVars
            prop: "bpmDisplayEnabled"
        }
        NumberWithSlider {
            from: -10
            to: 10
            prop: "bpmDisplayZ"
            text: qsTr("Z-index")
            src: popup.themeVars
        }
    }
}
