import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    id: popup

    required property var globalVars
    required property var themeVars
    property bool dp: false

    model: ObjectModel {
        NumberWithSlider {
            from: 0
            src: popup.globalVars
            prop: "noteScreenTimeMillis"
            text: qsTr("Green Number")
            to: 2000
        }
        NumberWithSlider {
            from: -10
            src: popup.themeVars
            prop: "playAreaZ"
            text: qsTr("Z-index")
            to: 10
        }
        NumberWithSlider {
            from: -1
            src: popup.themeVars
            prop: "laneBrightness"
            text: qsTr("Lane Brightness")
            to: 1
        }
        NumberWithSlider {
            from: 0
            src: popup.themeVars
            prop: "thickness"
            text: qsTr("Note Thickness")
            to: 200
        }
        NumberWithSlider {
            from: 0
            src: popup.themeVars
            prop: "judgeLineThickness"
            text: qsTr("Judge Line Thickness")
            to: 200
        }
        ColorChoice {
            src: popup.themeVars
            description: qsTr("Judge Line Color")
            prop: "judgeLineColor"
        }
        Loader {
            active: !popup.dp
            sourceComponent: Component {
                BooleanOption {
                    src: popup.themeVars
                    description: qsTr("Flip Scratch Lane")
                    prop: "scratchOnRightSide"
                }
            }
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "notes"
            label: qsTr("Notes")
        }
        BooleanOption {
            description: qsTr("Enable Lane Cover")
            src: popup.globalVars
            prop: "laneCoverOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.globalVars
            prop: "laneCoverRatio"
            text: qsTr("Lane Cover Ratio")
            to: 1
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "lanecover"
            label: qsTr("Lane Cover")
        }
        BooleanOption {
            description: qsTr("Enable Lift")
            src: popup.globalVars
            prop: "liftOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.globalVars
            prop: "liftRatio"
            text: qsTr("Lift Ratio")
            to: 1
        }
        BooleanOption {
            description: qsTr("Enable Hidden")
            src: popup.globalVars
            prop: "hiddenOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.globalVars
            prop: "hiddenRatio"
            text: qsTr("Hidden Ratio")
            to: 1
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "liftcover"
            label: qsTr("Lift Cover")
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "mine"
            label: qsTr("Mine")
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "keybeam"
            label: qsTr("Key Beam")
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "bomb"
            label: qsTr("Bomb")
        }
        ImageSelection {
            src: popup.themeVars
            propertyId: "glow"
            label: qsTr("Glow")
        }
    }
}