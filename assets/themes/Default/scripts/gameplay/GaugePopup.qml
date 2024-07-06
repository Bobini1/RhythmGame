import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    height: 200

    model: ObjectModel {
        NumberWithSlider {
            to: 10
            from: -10
            prop: "lifeBarZ"
            text: "Z-index"
        }
        BooleanOption {
            prop: "verticalGauge"
            description: "Vertical Gauge"
        }
        ImageSelection {
            id: gauge

            propertyId: "gauge"
        }
    }
}