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
            src: popup.themeVars
            prop: "ghostScoreEnabled"
            description: qsTr("Enabled")
        }
        FontSelection {
            src: popup.themeVars
            propertyId: "ghostScoreFont"
            label: qsTr("Font")
        }
    }
}
