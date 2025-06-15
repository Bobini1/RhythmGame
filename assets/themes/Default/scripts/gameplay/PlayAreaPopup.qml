import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQml.Models

GameplayPopup {
    id: popup

    required property var generalVars
    required property var themeVars
    property bool dp: false

    model: ObjectModel {
        NumberWithSlider {
            from: 0
            src: popup.generalVars
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
            src: popup.generalVars
            prop: "laneCoverOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.generalVars
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
            src: popup.generalVars
            prop: "liftOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.generalVars
            prop: "liftRatio"
            text: qsTr("Lift Ratio")
            to: 1
        }
        BooleanOption {
            description: qsTr("Enable Hidden")
            src: popup.generalVars
            prop: "hiddenOn"
        }
        NumberWithSlider {
            from: 0
            src: popup.generalVars
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