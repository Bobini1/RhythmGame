import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars
    panelTitle: qsTr("Gauge")
    panelSubtitle: qsTr("Life gauge visibility, layering, font, and image.")

    model: ObjectModel {
        BooleanOption {
            prop: "lifeBarEnabled"
            src: popup.themeVars
            description: qsTr("Enabled")
        }
        NumberWithSlider {
            to: 10
            from: -10
            prop: "lifeBarZ"
            text: qsTr("Z-index")
            src: popup.themeVars
        }
        BooleanOption {
            prop: "verticalGauge"
            src: popup.themeVars
            description: qsTr("Vertical Gauge")
        }
        FontSelection {
            src: popup.themeVars
            propertyId: "lifeNumberFont"
            label: qsTr("Life Number Font")
        }
        ImageSelection {
            id: gauge

            propertyId: "gauge"
            src: popup.themeVars
            label: qsTr("Gauge")
        }
    }
}
