import QtQuick
import QtQuick.Controls.Basic
import "settingsProperties"
import QtQml.Models
import QtQuick.Layouts
import RhythmGameQml

Item {
    ScrollView {
        id: scrollView
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
                Choice {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "language"
                    choices: Rg.languages.languages
                    displayStrings: Rg.languages.languages.map(lang => Rg.languages.getLanguageName(lang))
                    name: qsTr("Language")
                    default_: Qt.locale().name

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Range {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "noteScreenTimeMillis"
                    name: qsTr("Note Screen Time (ms)")
                    min: 0
                    default_: 1000
                    sliderMax: 1500

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Range {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "offset"
                    name: qsTr("Offset (ms)")
                    default_: 0
                    sliderMin: -15
                    sliderMax: 15

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Boolean {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "laneCoverOn"
                    name: qsTr("Lane Cover")
                    default_: false

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Range {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "laneCoverRatio"
                    name: qsTr("Lane Cover Ratio")
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
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "liftOn"
                    name: qsTr("Lift")
                    default_: false

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Range {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "liftRatio"
                    name: qsTr("Lift Ratio")
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
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "hiddenOn"
                    name: qsTr("Hidden")
                    default_: false

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Range {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "hiddenRatio"
                    name: qsTr("Hidden Ratio")
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
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "bgaOn"
                    name: qsTr("BGA On")
                    default_: true

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }

                Choice {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "noteOrderAlgorithm"
                    choices: ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]
                    displayStrings: qsTr("Normal;Mirror;Random;S-Random;R-Random;Random+;S-Random+").split(";")
                    name: qsTr("Note Order Algorithm")
                    assignIndex: true
                    default_: 0

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }

                Choice {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "noteOrderAlgorithmP2"
                    choices: ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]
                    displayStrings: qsTr("Normal;Mirror;Random;S-Random;R-Random;Random+;S-Random+").split(";")
                    name: qsTr("Note Order Algorithm P2")
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
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "hiSpeedFix"
                    choices: ["Off", "Main", "Start", "Min", "Max"]
                    displayStrings: qsTr("Off;Main;Start;Min;Max").split(";")
                    name: qsTr("Hi-Speed Fix")
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
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "dpOptions"
                    choices: ["Off", "Flip", "Battle"]
                    displayStrings: qsTr("Off;Flip;Battle").split(";")
                    name: qsTr("DP Options")
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
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "gaugeType"
                    choices: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
                    displayStrings: qsTr("ASSISTED EASY;EASY;NORMAL;HARD;EXHARD;FC").split(";")
                    name: qsTr("Gauge Type")
                    default_: "EXHARD"

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Choice {
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "gaugeMode"
                    choices: ["Exclusive", "Best", "Select to Under"]
                    displayStrings: qsTr("Exclusive;Best;Select to Under").split(";")
                    name: qsTr("Gauge Mode")
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
                    destination: Rg.profileList.mainProfile.vars.generalVars
                    id_: "bottomShiftableGauge"
                    name: qsTr("Bottom Shiftable Gauge")
                    choices: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
                    displayStrings: qsTr("ASSISTED EASY;EASY;NORMAL;HARD;EXHARD;FC").split(";")
                    default_: "EASY"

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Separator {
                }
                Choice {
                    destination: Rg.audioEngine
                    id_: "backend"
                    name: qsTr("Audio Backend")
                    choices: Rg.audioEngine.backendNames
                    default_: Rg.audioEngine.backendNames[0]

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
                Choice {
                    destination: Rg.audioEngine
                    id_: "device"
                    name: qsTr("Audio Device")
                    choices: {
                        let choices = Rg.audioEngine.deviceNames.slice();
                        choices.unshift("");
                        return choices;
                    }
                    property string defaultDeviceName: qsTr("Default")
                    displayStrings: {
                        let choices = Rg.audioEngine.deviceNames.slice();
                        choices.unshift(defaultDeviceName);
                        return choices;
                    }
                    default_: ""

                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                }
            }
        }
    }
}
