import QtQuick
import QtQuick.Controls
import "settingsProperties"
import QtQml.Models
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: root

    readonly property var generalVars: Rg.profileList.mainProfile.vars.generalVars

    ScrollView {
        id: scrollView

        anchors.fill: parent
        contentWidth: Math.max(list.implicitWidth, availableWidth)

        ColumnLayout {
            id: list

            x: Math.max(0, (scrollView.availableWidth - width) / 2)
            width: Math.min(1220, scrollView.contentWidth)
            spacing: 16

            SettingsPageHeader {
                title: qsTr("General Settings")
                subtitle: qsTr("System, gameplay, display, audio, replay, and gauge defaults.")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                WorkbenchPanel {
                    title: qsTr("System Settings")
                    Layout.alignment: Qt.AlignTop
                    Layout.minimumWidth: 560
                    Layout.preferredWidth: 560

                    Choice {
                        destination: root.generalVars
                        id_: "language"
                        choices: Rg.languages.languages
                        displayStrings: Rg.languages.languages.map(lang => Rg.languages.getLanguageName(lang))
                        name: qsTr("Language")
                        default_: Qt.locale().name
                        Layout.fillWidth: true
                    }

                    Choice {
                        destination: Window.window || {}
                        id_: "visibility"
                        choices: [Window.Windowed, Window.FullScreen]
                        displayStrings: qsTr("Windowed;Fullscreen").split(";")
                        name: qsTr("Display Mode")
                        default_: Window.Windowed
                        Layout.fillWidth: true
                    }

                    Choice {
                        destination: root.generalVars
                        id_: "rankingProvider"
                        choices: [OnlineRankingModel.RhythmGame, OnlineRankingModel.LR2IR, OnlineRankingModel.Tachi]
                        displayStrings: qsTr("RhythmGame;LR2IR;Bokutachi").split(";")
                        name: qsTr("Ranking Provider")
                        default_: OnlineRankingModel.RhythmGame
                        Layout.fillWidth: true
                    }

                    Choice {
                        destination: root.generalVars
                        id_: "bgm"
                        name: qsTr("BGM")
                        choices: root.generalVars.getAvailableBgms()
                        default_: "Trance"
                        Layout.fillWidth: true
                    }

                    Choice {
                        destination: root.generalVars
                        id_: "soundset"
                        name: qsTr("Soundset")
                        choices: root.generalVars.getAvailableSoundsets()
                        default_: "Brook"
                        Layout.fillWidth: true
                    }

                    Choice {
                        destination: Rg.audioEngine
                        id_: "backend"
                        name: qsTr("Audio Backend")
                        choices: Rg.audioEngine.backendNames
                        default_: Rg.audioEngine.backendNames[0]
                        Layout.fillWidth: true
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
                        Layout.fillWidth: true
                    }
                }

                WorkbenchPanel {
                    title: qsTr("Gameplay Settings")
                    Layout.fillWidth: true
                    Layout.minimumWidth: 620

                Range {
                    destination: root.generalVars
                    id_: "noteScreenTimeMillis"
                    name: qsTr("Note Screen Time (ms)")
                    min: 0
                    default_: 1000
                    sliderMax: 1500
                    Layout.fillWidth: true
                }

                Range {
                    destination: root.generalVars
                    id_: "offset"
                    name: qsTr("Offset (ms)")
                    default_: 0
                    sliderMin: -15
                    sliderMax: 15
                    Layout.fillWidth: true
                }

                Range {
                    destination: Rg.inputTranslator
                    id_: "debounceMs"
                    name: qsTr("Debounce (ms)")
                    default_: 5
                    min: 0
                    sliderMax: 150
                    decimals: 0
                    Layout.fillWidth: true
                }

                Boolean {
                    destination: root.generalVars
                    id_: "laneCoverOn"
                    name: qsTr("Lane Cover")
                    default_: false
                    Layout.fillWidth: true
                }

                Range {
                    destination: root.generalVars
                    id_: "laneCoverRatio"
                    name: qsTr("Lane Cover Ratio")
                    default_: 0.1
                    min: 0
                    max: 1
                    Layout.fillWidth: true
                }

                Boolean {
                    destination: root.generalVars
                    id_: "liftOn"
                    name: qsTr("Lift")
                    default_: false
                    Layout.fillWidth: true
                }

                Range {
                    destination: root.generalVars
                    id_: "liftRatio"
                    name: qsTr("Lift Ratio")
                    default_: 0.1
                    min: 0
                    max: 1
                    Layout.fillWidth: true
                }

                Boolean {
                    destination: root.generalVars
                    id_: "hiddenOn"
                    name: qsTr("Hidden")
                    default_: false
                    Layout.fillWidth: true
                }

                Range {
                    destination: root.generalVars
                    id_: "hiddenRatio"
                    name: qsTr("Hidden Ratio")
                    default_: 0.1
                    min: 0
                    max: 1
                    Layout.fillWidth: true
                }

                Boolean {
                    destination: root.generalVars
                    id_: "bgaOn"
                    name: qsTr("BGA On")
                    default_: true
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "bgaSize"
                    choices: [0, 1]
                    displayStrings: qsTr("Normal;Extend").split(";")
                    name: qsTr("LR2 BGA Size")
                    default_: 0
                    Layout.fillWidth: true
                }

                Boolean {
                    destination: root.generalVars
                    id_: "scoreGraphEnabled"
                    name: qsTr("Score Graph")
                    default_: true
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "ghostPosition"
                    choices: [0, 1, 2, 3]
                    displayStrings: qsTr("Off;Type A;Type B;Type C").split(";")
                    name: qsTr("Ghost Position")
                    default_: 0
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "replayType"
                    choices: [0, 1, 2, 3]
                    displayStrings: qsTr("Newest;Best Score;Best Clear;Best Combo").split(";")
                    name: qsTr("Replay Type")
                    default_: 0
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "scoreTarget"
                    choices: [ScoreTarget.Fraction, ScoreTarget.BestScore, ScoreTarget.LastScore, ScoreTarget.NextRank]
                    displayStrings: qsTr("Percentage;Best Score;Last Score;Next Rank").split(";")
                    name: qsTr("Score Target Type")
                    default_: ScoreTarget.BestScore
                    Layout.fillWidth: true
                }

                Range {
                    destination: root.generalVars
                    id_: "targetScoreFraction"
                    name: qsTr("Target Score Percentage")
                    min: 0
                    max: 100
                    default_: 8 / 9
                    increment: 1
                    decimals: 6
                    displayMultiplier: 100
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "noteOrderAlgorithm"
                    choices: ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]
                    displayStrings: qsTr("Normal;Mirror;Random;S-Random;R-Random;Random+;S-Random+").split(";")
                    name: qsTr("Note Order Algorithm")
                    assignIndex: true
                    default_: 0
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "noteOrderAlgorithmP2"
                    choices: ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]
                    displayStrings: qsTr("Normal;Mirror;Random;S-Random;R-Random;Random+;S-Random+").split(";")
                    name: qsTr("Note Order Algorithm P2")
                    assignIndex: true
                    default_: 0
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "hiSpeedFix"
                    choices: ["Off", "Main", "Start", "Min", "Max", "Avg"]
                    displayStrings: qsTr("Off;Main;Start;Min;Max;Average").split(";")
                    name: qsTr("Hi-Speed Fix")
                    assignIndex: true
                    default_: 1
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "dpOptions"
                    choices: ["Off", "Flip", "Battle"]
                    displayStrings: qsTr("Off;Flip;Battle").split(";")
                    name: qsTr("DP Options")
                    assignIndex: true
                    default_: 0
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "gaugeType"
                    choices: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
                    displayStrings: qsTr("ASSISTED EASY;EASY;NORMAL;HARD;EXHARD;FC").split(";")
                    name: qsTr("Gauge Type")
                    default_: "FC"
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "gaugeMode"
                    choices: ["Exclusive", "Best", "Select to Under"]
                    displayStrings: qsTr("Exclusive;Best;Select to Under").split(";")
                    name: qsTr("Gauge Mode")
                    assignIndex: true
                    default_: 2
                    Layout.fillWidth: true
                }

                Choice {
                    destination: root.generalVars
                    id_: "bottomShiftableGauge"
                    name: qsTr("Bottom Shiftable Gauge")
                    choices: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
                    displayStrings: qsTr("ASSISTED EASY;EASY;NORMAL;HARD;EXHARD;FC").split(";")
                    default_: "AEASY"
                    Layout.fillWidth: true
                }
            }
        }
    }
}
}
