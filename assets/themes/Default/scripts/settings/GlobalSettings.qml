import QtQuick
import QtQuick.Controls.Basic
import "settingsProperties"
import QtQml.Models
import QtQuick.Layouts
import RhythmGameQml

Item {
    ScrollView {
        id: scrollView
        clip: true
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            bottom: parent.bottom
        }
        width: Math.min(parent.width / 2, 600)
        padding: 1

        Frame {
            width: 600
            z: -1
            Column {
                id: list
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                spacing: 5

                component SettingLabel:
                    TextEdit {
                    id: text
                    font.pixelSize: 16
                    font.bold: true
                    readOnly: true
                    text: "Green Number"
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 200
                    wrapMode: TextEdit.Wrap
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Green Number"
                    }
                    Range {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "noteScreenTimeMillis"
                        min: 0
                        default_: 1000
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "noteScreenTimeMillis"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Lane Cover"
                    }
                    Boolean {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "laneCoverOn"
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "laneCoverOn"
                    }
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Lane Cover Ratio"
                    }
                    Range {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "laneCoverRatio"
                        default_: 0.1
                        min: 0
                        max: 1
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "laneCoverRatio"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Lift"
                    }
                    Boolean {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "liftOn"
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "liftOn"
                    }
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Lift Ratio"
                    }
                    Range {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "liftRatio"
                        default_: 0.1
                        min: 0
                        max: 1
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "liftRatio"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Hidden"
                    }
                    Boolean {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "hiddenOn"
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "hiddenOn"
                    }
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Hidden Ratio"
                    }
                    Range {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "hiddenRatio"
                        default_: 0.1
                        min: 0
                        max: 1
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "hiddenRatio"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "BGA Enabled"
                    }
                    Boolean {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "bgaOn"
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "bgaOn"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Note Order Algorithm"
                    }
                    Choice {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "noteOrderAlgorithm"
                        choices: ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]
                        assignIndex: true
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "noteOrderAlgorithm"
                    }
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Note Order Algorithm P2"
                    }
                    Choice {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "noteOrderAlgorithmP2"
                        choices: ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]
                        assignIndex: true
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "noteOrderAlgorithmP2"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Hi Speed Fix"
                    }
                    Choice {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "hiSpeedFix"
                        choices: ["Off", "Main", "Start", "Min", "Max"]
                        assignIndex: true
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "hiSpeedFix"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "DP Options"
                    }
                    Choice {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "dpOptions"
                        choices: ["Off", "Flip", "Battle", "BattleAs"]
                        assignIndex: true
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "dpOptions"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Gauge Type"
                    }
                    Choice {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "gaugeType"
                        choices: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "gaugeType"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Gauge Mode"
                    }
                    Choice {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "gaugeMode"
                        choices: ["Exclusive", "Best", "Select to Under"]
                        assignIndex: true
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "gaugeMode"
                    }
                }
                Separator {
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    SettingLabel {
                        text: "Bottom Shiftable Gauge"
                    }
                    Choice {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "bottomShiftableGauge"
                        choices: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
                    }
                    ResetButton {
                        destination: Rg.profileList.mainProfile.vars.globalVars
                        id_: "bottomShiftableGauge"
                    }
                }
            }
        }
    }
}

