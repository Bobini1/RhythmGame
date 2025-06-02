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
        width: 600
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

                Range {
                    destination: Rg.profileList.mainProfile.vars.globalVars
                    id_: "noteScreenTimeMillis"
                    name: "Note Screen Time (ms)"
                    min: 0
                    default_: 1000
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
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
            }
        }
    }
}

