import QtQuick
import QtQuick.Controls.Fusion
import RhythmGameQml
import QtQuick.Layouts
import "settingsProperties"
import QtQml.Models

Frame {
    Layout.fillHeight: true
    Layout.fillWidth: true
    Layout.preferredWidth: parent.width / 2
    Layout.alignment: Qt.AlignHCenter
    ScrollView {
        anchors.fill:parent
        id: scrollView
        clip: true
        contentWidth: Math.max(width, 450)
        ListView {
            model: ObjectModel {
                Row {
                    height: 30
                    TextEdit {
                        font.pixelSize: 16
                        font.bold: true
                        readOnly: true
                        text: "Green Number"
                    }
                    Loader {
                        active: true
                        property var props: {"id": "noteScreenTimeMillis", "min": 0}
                        property var destination: ProfileList.currentProfile.vars.globalVars
                        width: 150
                        anchors.verticalCenter: parent.verticalCenter
                        sourceComponent: Component {
                            Range {
                            }
                        }
                    }
                }
                Row {
                    height: 30
                    TextEdit {
                        font.pixelSize: 16
                        font.bold: true
                        readOnly: true
                        text: "Lane Cover"
                    }
                    Loader {
                        active: true
                        property var props: {"id":"laneCoverOn"}
                        property var destination: ProfileList.currentProfile.vars.globalVars
                        width: 150
                        anchors.verticalCenter: parent.verticalCenter
                        sourceComponent: Component {
                            Boolean {}
                        }
                    }
                }
            }
        }
    }
}
