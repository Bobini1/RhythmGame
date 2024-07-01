import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    model: ObjectModel {
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
            description: "Flip scratch lane"
            prop: "scratchOnRightSide"
        }
        ImageSelection {
            id: notes

            propertyId: "notes"
        }
        ImageSelection {
            id: mine

            propertyId: "mine"
        }
        ImageSelection {
            id: keybeam

            propertyId: "keybeam"
        }
        ImageSelection {
            id: bomb

            propertyId: "bomb"
        }
        ImageSelection {
            id: glow

            propertyId: "glow"
        }
    }
}