import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars

    model: ObjectModel {
        ImageSelection {
            src: popup.themeVars
            propertyId: "judge"
            label: qsTr("Judgements")
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "fastslow"
            label: qsTr("Fast/Slow")
        }
    }
}