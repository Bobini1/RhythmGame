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
                stepSize: props.max <= 1.0 ? 0.1 : 1
                from: props.min
                to: props.max
                Layout.fillHeight: true
                value: destination[props.id]

                onMoved: {
                    destination[props.id] = Math.round(value * 10) / 10
                    value = Qt.binding(() => destination[props.id])
                }
            }
        }
    }
    TextField {
        id: textField
        horizontalAlignment: Text.AlignHCenter
        Layout.preferredWidth: textMetrics.width + 20
        text: Qt.locale().toString(destination[props.id], "f", 3)
        Layout.fillHeight: true
        color: acceptableInput ? "black" : "red"
        validator: DoubleValidator {
            id: doubleValidator
            bottom: props.min !== undefined ? props.min : -Infinity; top: props.max !== undefined ? props.max : Infinity
        }
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onTextEdited: {
            if (acceptableInput) {
                destination[props.id] = Number.fromLocaleString(text)
                text = Qt.binding(() => Qt.locale().toString(destination[props.id], "f", 3))
            }
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