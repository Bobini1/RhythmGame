import QtQuick
import QtQml.Models

GameplayPopup {
    id: popup

    required property var themeVars

    model: ObjectModel {
        BooleanOption {
            description: qsTr("Enabled")
            src: popup.themeVars
            prop: "hitDistributionEnabled"
        }
        BooleanOption {
            description: qsTr("Vertical")
            src: popup.themeVars
            prop: "hitDistributionVertical"
        }
        BooleanOption {
            description: qsTr("EWMA Mode")
            src: popup.themeVars
            prop: "hitDistributionEwmaMode"
        }
        NumberWithSlider {
            text: qsTr("EWMA Alpha")
            from: 0.01
            to: 1.0
            src: popup.themeVars
            prop: "hitDistributionEwmaAlpha"
        }
        NumberWithSlider {
            text: qsTr("Max Trail Length")
            from: 1
            to: 100
            src: popup.themeVars
            prop: "hitDistributionMaxTrail"
        }
        ColorChoice {
            description: qsTr("Hit Line Color")
            src: popup.themeVars
            prop: "hitDistributionLineColor"
        }
        ColorChoice {
            description: qsTr("Center Line Color")
            src: popup.themeVars
            prop: "hitDistributionCenterLineColor"
        }
        NumberWithSlider {
            text: qsTr("Hit Line Width")
            from: 1
            to: 10
            src: popup.themeVars
            prop: "hitDistributionLineWidth"
        }
        NumberWithSlider {
            text: qsTr("Center Line Width")
            from: 1
            to: 10
            src: popup.themeVars
            prop: "hitDistributionCenterLineWidth"
        }
        NumberWithSlider {
            text: qsTr("Background Opacity")
            from: 0
            to: 1
            src: popup.themeVars
            prop: "hitDistributionBackgroundOpacity"
        }
        NumberWithSlider {
            text: qsTr("Z-index")
            from: -10
            to: 10
            src: popup.themeVars
            prop: "hitDistributionZ"
        }
    }
}


