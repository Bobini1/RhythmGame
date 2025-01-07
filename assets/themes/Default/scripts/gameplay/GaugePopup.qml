import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    id: popup
    height: 200

    required property var themeVars

    model: ObjectModel {
        NumberWithSlider {
            to: 10
            from: -10
            prop: "lifeBarZ"
            text: "Z-index"
            src: popup.themeVars
        }
        BooleanOption {
            prop: "verticalGauge"
            src: popup.themeVars
            description: "Vertical Gauge"
        }
        ImageSelection {
            id: gauge

            propertyId: "gauge"
            src: popup.themeVars
        }
    }
}