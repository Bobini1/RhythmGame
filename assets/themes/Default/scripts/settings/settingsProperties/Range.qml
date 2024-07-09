import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick
import QtQuick.Layouts

RowLayout {
    height: 30
    Loader {
        active: props.min !== undefined && props.max !== undefined
        Layout.fillWidth: active
        sourceComponent: Component {
            Slider {
                stepSize: 0.001
                from: props.min
                to: props.max
                Layout.fillHeight: true
                value: destination[props.id]

                onMoved: {
                    destination[props.id] = Math.round(value * 1000) / 1000
                    value = Qt.binding(() => destination[props.id])
                }
            }
        }
    }
    TextField {
        id: textField

        function getFormattedNumber(num) {
            let longNum = Qt.locale().toString(num, "f", -128);
            let shortNum = Qt.locale().toString(num, "f", 3);
            if (longNum.length > shortNum.length) {
                return longNum;
            }
            return shortNum;
        }
        horizontalAlignment: Text.AlignHCenter
        Layout.preferredWidth: textMetrics.width + 20
        text: getFormattedNumber(destination[props.id])
        Layout.fillHeight: true
        color: acceptableInput ? "black" : "red"
        validator: DoubleValidator {
            id: doubleValidator
            bottom: props.min !== undefined ? props.min : -Infinity; top: props.max !== undefined ? props.max : Infinity
        }
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onTextEdited: {
            // noinspection SillyAssignmentJS
            text = text
            if (acceptableInput) {
                destination[props.id] = Number.fromLocaleString(text)
            }
        }

        onEditingFinished: {
            text = Qt.binding(() => getFormattedNumber(destination[props.id]))
        }
        TextMetrics {
            id: textMetrics
            font: textField.font
            text: Qt.locale().toString(props.max !== undefined ? props.max : 1000, "f", 3)
        }
        Layout.alignment: Qt.AlignRight
        HoverHandler {
            id: hoverHandler
        }
        ToolTip.visible: hoverHandler.hovered
        ToolTip.text: "Limits: " + Qt.locale().toString(doubleValidator.bottom, "f", -128) + " - " + Qt.locale().toString(doubleValidator.top, "f", -128)
    }
}