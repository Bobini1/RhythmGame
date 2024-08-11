import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    model: ObjectModel {
        NumberWithSlider {
            text: "Green Number"
            prop: "noteScreenTimeMillis"
            global: true
            from: 0
            to: 2000
        }
        NumberWithSlider {
            text: "Z-index"
            prop: "playAreaZ"
            from: -10
            to: 10
        }
        NumberWithSlider {
            from: -1
            prop: "laneBrightness"
            text: "Lane Brightness"
            to: 1
        }
        NumberWithSlider {
            text: "Note Thickness"
            prop: "thickness"
            from: 0
            to: 200
        }
        BooleanOption {
            description: "Flip Scratch Lane"
            prop: "scratchOnRightSide"
        }
        ImageSelection {
            propertyId: "notes"
        }
        BooleanOption {
            description: "Enable Lane Cover"
            prop: "laneCoverOn"
            global: true
        }
        NumberWithSlider {
            text: "Lane Cover Ratio"
            prop: "laneCoverRatio"
            global: true
            from: 0
            to: 1
        }
        ImageSelection {
            propertyId: "lanecover"
        }
        BooleanOption {
            description: "Enable Lift"
            prop: "liftOn"
            global: true
        }
        NumberWithSlider {
            text: "Lift Ratio"
            prop: "liftRatio"
            global: true
            from: 0
            to: 1
        }
        ImageSelection {
            propertyId: "liftcover"
        }
        ImageSelection {
            propertyId: "mine"
        }
        ImageSelection {
            propertyId: "keybeam"
        }
        ImageSelection {
            propertyId: "bomb"
        }
        ImageSelection {
            propertyId: "glow"
        }
    }
}