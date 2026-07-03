import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars
    panelTitle: qsTr("Fast/Slow indicator")
    panelSubtitle: qsTr("Timing indicator visibility and images.")

    model: ObjectModel {
        BooleanOption {
            src: popup.themeVars
            prop: "fastslowEnabled"
            description: qsTr("Enabled")
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "fastslow"
            label: qsTr("Image")
        }
    }
}
