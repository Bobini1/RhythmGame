import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import "../common/helpers.js" as Helpers
import "settingsProperties"


GroupBox {
    id: axisSettings
    property int player: 1
    title: qsTr(`Player %1 Turntable`).arg(player)
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

        Choice {
            destination: Rg.inputTranslator[`analogAxisConfig${player}`]
            id_: "scratchAlgorithm"
            name: qsTr("Algorithm")
            choices: ["Analog", "Classic"]
            assignIndex: true
            default_: 0
        }


        Range {
            id_: "triggerThreshold"
            destination: Rg.inputTranslator[`analogAxisConfig${player}`]
            name: qsTr("Trigger Threshold")
            sliderMax: 0.1
            sliderMin: 0
            min: 0
            max: 1
            default_: 0.008
        }
        Range {
            id_: "releaseThreshold"
            destination: Rg.inputTranslator[`analogAxisConfig${player}`]
            name: qsTr("Release Threshold")
            sliderMax: 0.1
            sliderMin: 0
            min: 0
            max: 1
            default_: 0.004
        }

        Range {
            id_: "timeout"
            destination: Rg.inputTranslator[`analogAxisConfig${player}`]
            name: qsTr("Timeout")
            sliderMax: 500
            sliderMin: 0
            min: 0
            default_: 100
        }
    }
}