import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars

    model: ObjectModel {
        NumberWithSlider {
            from: -10
            src: popup.themeVars
            prop: "judgementCountsZ"
            text: qsTr("Z-index")
            to: 10
        }
        FontSelection {
            src: popup.themeVars
            propertyId: "judgementCountsFont"
            label: qsTr("Font")
        }
    }
}
