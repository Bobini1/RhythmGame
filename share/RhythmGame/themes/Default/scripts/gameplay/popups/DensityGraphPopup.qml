import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars

    model: ObjectModel {
        BooleanOption {
            description: qsTr("Enabled")
            src: popup.themeVars
            prop: "densityGraphEnabled"
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "densityGraphOpacity"
            text: qsTr("Opacity")
            src: popup.themeVars
        }
        BooleanOption {
            description: qsTr("Show Gaps")
            src: popup.themeVars
            prop: "densityGraphGapsEnabled"
        }
        BooleanOption {
            description: qsTr("Show Notes")
            src: popup.themeVars
            prop: "densityGraphShowNotes"
        }
        BooleanOption {
            description: qsTr("Show BPM")
            src: popup.themeVars
            prop: "densityGraphShowBpm"
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "densityGraphBackgroundOpacity"
            text: qsTr("Background Opacity")
            src: popup.themeVars
        }
        BooleanOption {
            description: qsTr("Frame")
            src: popup.themeVars
            prop: "densityGraphFrameEnabled"
        }
        BooleanOption {
            description: qsTr("Vertical")
            src: popup.themeVars
            prop: "densityGraphVertical"
        }
        NumberWithSlider {
            from: -10
            to: 10
            prop: "densityGraphZ"
            text: qsTr("Z-index")
            src: popup.themeVars
        }
    }
}




