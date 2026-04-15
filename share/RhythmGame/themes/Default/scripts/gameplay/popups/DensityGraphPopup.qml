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
        BooleanOption {
            description: qsTr("Show Gaps")
            src: popup.themeVars
            prop: "densityGraphGapsEnabled"
        }
        BooleanOption {
            description: qsTr("Vertical")
            src: popup.themeVars
            prop: "densityGraphVertical"
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "densityGraphNotesOpacity"
            text: qsTr("Notes Opacity")
            src: popup.themeVars
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "densityGraphBpmOpacity"
            text: qsTr("BPM Opacity")
            src: popup.themeVars
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "densityGraphBpmConnectorOpacity"
            text: qsTr("BPM Connector Opacity")
            src: popup.themeVars
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "densityGraphBackgroundOpacity"
            text: qsTr("Background Opacity")
            src: popup.themeVars
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "densityGraphFrameOpacity"
            text: qsTr("Frame Opacity")
            src: popup.themeVars
        }
        NumberWithSlider {
            from: 0
            to: 1
            prop: "densityGraphPositionLineOpacity"
            text: qsTr("Position Line Opacity")
            src: popup.themeVars
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




