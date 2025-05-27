import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import "../common/helpers.js" as Helpers


GroupBox {
    id: axisSettings
    property int player: 1
    title: qsTr(`Configure player ${player} turntable`)
    property var axisConfig: Rg.inputTranslator[`analogAxisConfig${player}`]

    component NumberField: TextField {
        id: textField

        property string variable
        property real max
        property real min

        horizontalAlignment: contentWidth >= width ? TextField.AlignLeft : TextField.AlignHCenter
        autoScroll: false
        text: Qt.locale().toString(Rg.inputTranslator[`analogAxisConfig${player}`][textField.variable], "f", 3)
        color: acceptableInput ? "black" : "red"
        validator: DoubleValidator {
            id: doubleValidator
            bottom: textField.min; top: textField.max
        }
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onTextEdited: {
            // noinspection SillyAssignmentJS
            text = text
            if (acceptableInput) {
                Rg.inputTranslator[`analogAxisConfig${player}`][textField.variable] = Number.fromLocaleString(text)
            }
        }

        onEditingFinished: {
            text = Qt.binding(() => Qt.locale().toString(Rg.inputTranslator[`analogAxisConfig${player}`][textField.variable], "f", 3))
            ensureVisible(0);
        }
        onActiveFocusChanged: {
            autoScroll = true;
        }

        Component.onCompleted: {
            ensureVisible(0);
        }
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            spacing: 0
            TextEdit {
                text: qsTr("Algorithm")
                font.pixelSize: 20
                readOnly: true
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 3
            }
            ComboBox {
                id: choiceComboBox
                model: ["Analog", "Classic"]
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 3 * 2

                Binding {
                    delayed: true
                    choiceComboBox.currentIndex: Rg.inputTranslator[`analogAxisConfig${player}`].scratchAlgorithm;
                }

                onActivated: () => {
                    Rg.inputTranslator[`analogAxisConfig${player}`].scratchAlgorithm = currentIndex
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0
            TextEdit {
                text: qsTr("Trigger Threshold")
                font.pixelSize: 20
                readOnly: true
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 3
            }
            NumberField {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 6
                variable: "triggerThreshold"
                max: 1
                min: 0
            }
            Slider {
                id: triggerThreshold
                from: 0
                to: 0.10
                stepSize: 0.001
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 2

                Binding {
                    delayed: true
                    triggerThreshold.value: Rg.inputTranslator[`analogAxisConfig${player}`].triggerThreshold;
                }

                onMoved: () => {
                    Rg.inputTranslator[`analogAxisConfig${player}`].triggerThreshold = value;
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 0
            TextEdit {
                text: qsTr("Release Threshold")
                font.pixelSize: 20
                readOnly: true
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 3
            }
            NumberField {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 6
                variable: "releaseThreshold"
                max: 1
                min: 0
            }
            Slider {
                id: releaseThreshold
                from: 0
                to: 0.10
                stepSize: 0.001
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 2

                Binding {
                    delayed: true
                    releaseThreshold.value: Rg.inputTranslator[`analogAxisConfig${player}`].releaseThreshold;
                }

                onMoved: () => {
                    Rg.inputTranslator[`analogAxisConfig${player}`].releaseThreshold = value;
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0
            TextEdit {
                text: qsTr("Timeout")
                font.pixelSize: 20
                readOnly: true
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 3
            }
            NumberField {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 6
                variable: "timeout"
                max: 500
                min: 0
            }
            Slider {
                id: timeout
                from: 0
                to: 500
                stepSize: 1
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 2
                Binding {
                    delayed: true
                    timeout.value: Rg.inputTranslator[`analogAxisConfig${player}`].timeout;
                }
                onMoved: () => {
                    Rg.inputTranslator[`analogAxisConfig${player}`].timeout = value;
                }
            }
        }
    }
}