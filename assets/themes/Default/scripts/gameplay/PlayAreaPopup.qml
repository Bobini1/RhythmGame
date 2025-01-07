import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    id: popup

    required property var globalVars
    required property var themeVars
    
    model: ObjectModel {
        NumberWithSlider {
            from: 0
            src: popup.globalVars
            prop: "noteScreenTimeMillis"
            text: "Green Number"
            to: 2000
        }
        NumberWithSlider {
            from: -10
            src: popup.themeVars
            prop: "playAreaZ"
            text: "Z-index"
            to: 10
        }
        NumberWithSlider {
            from: -1
            src: popup.themeVars
            prop: "laneBrightness"
            text: "Lane Brightness"
            to: 1
        }
        NumberWithSlider {
            from: 0
            src: popup.themeVars
            prop: "thickness"
            text: "Note Thickness"
            to: 200
        }
        NumberWithSlider {
            from: 0
            src: popup.themeVars
            prop: "judgeLineThickness"
            text: "Judge Line Thickness"
            to: 200
        }
        ColorChoice {
            src: popup.themeVars
            description: "Judge Line Color"
            prop: "judgeLineColor"
        }
        BooleanOption {
            src: popup.themeVars
            description: "Flip Scratch Lane"
            prop: "scratchOnRightSide"
        }
        BooleanOption {
            src: popup.themeVars
            description: "Stop notes at bottom"
            prop: "notesStay"
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "notes"
        }
        BooleanOption {
            description: "Enable Lane Cover"
            src: popup.globalVars
            prop: "laneCoverOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.globalVars
            prop: "laneCoverRatio"
            text: "Lane Cover Ratio"
            to: 1
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "lanecover"
        }
        BooleanOption {
            description: "Enable Lift"
            src: popup.globalVars
            prop: "liftOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.globalVars
            prop: "liftRatio"
            text: "Lift Ratio"
            to: 1
        }
        BooleanOption {
            description: "Enable Hidden"
            src: popup.globalVars
            prop: "hiddenOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.globalVars
            prop: "hiddenRatio"
            text: "Hidden Ratio"
            to: 1
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "liftcover"
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "mine"
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "keybeam"
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "bomb"
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "glow"
        }
    }
}