import QtQuick
import QtQuick.Controls.Basic
import "settingsProperties"
import QtQml.Models

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

        ListView {
            id: list
            width: 600

            spacing: 10
            bottomMargin: 10
            topMargin: 10
            leftMargin: 10
            rightMargin: 10

            Frame {
                anchors.fill: list
                z: -1
            }

            model: ObjectModel {
                GlobalSettingsProperty {
                    text: "Green Number"
                    props: {"id": "noteScreenTimeMillis", "min": 0, "default": 1000}
                    sourceComponent: Component {
                        Range {
                        }
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Lane Cover"
                    props: {"id": "laneCoverOn", "default": false}
                    sourceComponent: Component {
                        Boolean {}
                    }
                }
                GlobalSettingsProperty {
                    text: "Lane Cover Ratio"
                    props: {"id": "laneCoverRatio", "default": 0.1, "min": 0, "max": 1}
                    sourceComponent: Component {
                        Range {}
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Lift"
                    props: {"id": "liftOn", "default": false}
                    sourceComponent: Component {
                        Boolean {}
                    }
                }
                GlobalSettingsProperty {
                    text: "Lift Ratio"
                    props: {"id": "liftRatio", "default": 0.1, "min": 0, "max": 1}
                    sourceComponent: Component {
                        Range {}
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Hidden"
                    props: {"id": "hiddenOn", "default": false}
                    sourceComponent: Component {
                        Boolean {}
                    }
                }
                GlobalSettingsProperty {
                    text: "Hidden Ratio"
                    props: {"id": "hiddenRatio", "default": 0.1, "min": 0, "max": 1}
                    sourceComponent: Component {
                        Range {}
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "BGA Enabled"
                    props: {"id": "bgaOn", "default": true}
                    sourceComponent: Component {
                        Boolean {}
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Note Order Algorithm"
                    props: {"id": "noteOrderAlgorithm", "default": 0, "choices": ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]}
                    sourceComponent: Component {
                        Choice {
                            assignIndex: true
                        }
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Note Order Algorithm P2"
                    props: {"id": "noteOrderAlgorithmP2", "default": 0, "choices": ["Normal", "Mirror", "Random", "S-Random", "R-Random", "Random+", "S-Random+"]}
                    sourceComponent: Component {
                        Choice {
                            assignIndex: true
                        }
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Hi Speed Fix"
                    props: {"id": "hiSpeedFix", "default": 1, "choices": ["Off", "Main", "Start", "Min", "Max"]}
                    sourceComponent: Component {
                        Choice {
                            assignIndex: true
                        }
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "DP Options"
                    props: {"id": "dpOptions", "default": 0, "choices": ["Off", "Flip", "Battle", "BattleAs"]}
                    sourceComponent: Component {
                        Choice {
                            assignIndex: true
                        }
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Gauge Type"
                    props: {"id": "gaugeType", "default": "FC", "choices": ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]}
                    sourceComponent: Component {
                        Choice {
                            assignIndex: true
                        }
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Gauge Mode"
                    props: {"id": "gaugeMode", "default": 2, "choices": ["Exclusive", "Best", "Select to Under"]}
                    sourceComponent: Component {
                        Choice {
                            assignIndex: true
                        }
                    }
                }
                Separator {}
                GlobalSettingsProperty {
                    text: "Bottom Shiftable Gauge"
                    props: {"id": "bottomShiftableGauge", "default": "AEASY", "choices": ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]}
                    sourceComponent: Component {
                        Choice {
                            assignIndex: true
                        }
                    }
                }
            }
        }
    }
}