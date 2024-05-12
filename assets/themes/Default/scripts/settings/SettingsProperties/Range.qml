import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick
import QtQuick.Layouts

RowLayout {
    height: 30
    Slider {
        Layout.fillWidth: true
        stepSize: props.max && props.max <= 1.0 ? 0.01 : 1
        from: props.min || 0
        to: props.max || 100
        Layout.fillHeight: true
        value: destination[props.id]

        onValueChanged: destination[props.id] = value
    }
    TextField {
        id: textField
        horizontalAlignment: Text.AlignHCenter
        Layout.preferredWidth: textMetrics.width + 20
        text: Qt.locale().toString(destination[props.id], "f", 3)
        Layout.fillHeight: true
        validator: DoubleValidator {
            bottom: props.min || 0; top: props.max || 100
        }
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onTextEdited: {
            if (acceptableInput) {
                destination[props.id] = Number.fromLocaleString(text)
            }
        }
        TextMetrics {
            id: textMetrics
            font: textField.font
            text: Qt.locale().toString(props.max || 1000, "f", 3)
        }
    }
}