import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars

    model: ObjectModel {
        BooleanOption {
            description: qsTr("Enabled")
            src: popup.themeVars
            prop: "judgementsEnabled"
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "judge"
            label: qsTr("Image")
        }
    }
}