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
            top: parent.top
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        width: Math.min(600, parent.width)

        Frame {
            width: 600
            Column {
                id: list
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                spacing: 5

                Range {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "noteScreenTimeMillis"
                    name: "Note Screen Time (ms)"
                    min: 0
                    default_: 1000
                    sliderMax: 1500
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Boolean {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "laneCoverOn"
                    name: "Lane Cover"
                    default_: false
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Range {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "laneCoverRatio"
                    name: "Lane Cover Ratio"
                    default_: 0.1
                    min: 0
                    max: 1
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Boolean {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "liftOn"
                    name: "Lift"
                    default_: false
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Range {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "liftRatio"
                    name: "Lift Ratio"
                    default_: 0.1
                    min: 0
                    max: 1
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Boolean {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "hiddenOn"
                    name: "Hidden"
                    default_: false
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Range {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "hiddenRatio"
                    name: "Hidden Ratio"
                    default_: 0.1
                    min: 0
                    max: 1
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Boolean {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "bgaOn"
                    name: "BGA On"
                    default_: true
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }

                Choice {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "noteOrderAlgorithm"
                    choices: ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]
                    name: "Note Order Algorithm"
                    assignIndex: true
                    default_: 0
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                Choice {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "noteOrderAlgorithmP2"
                    choices: ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]
                    name: "Note Order Algorithm P2"
                    assignIndex: true
                    default_: 0
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }

                Choice {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "hiSpeedFix"
                    choices: ["Off", "Main", "Start", "Min", "Max"]
                    name: "Hi-Speed Fix"
                    assignIndex: true
                    default_: 1
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Choice {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "dpOptions"
                    choices: ["Off", "Flip", "Battle", "BattleAs"]
                    name: "DP Options"
                    assignIndex: true
                    default_: 0
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Choice {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "gaugeType"
                    choices: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
                    name: "Gauge Type"
                    default_: "EXHARD"
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Choice {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "gaugeMode"
                    choices: ["Exclusive", "Best", "Select to Under"]
                    name: "Gauge Mode"
                    assignIndex: true
                    default_: 1
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Choice {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "bottomShiftableGauge"
                    name: "Bottom Shiftable Gauge"
                    choices: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
                    default_: "EASY"
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
            }
        }
    }
}

