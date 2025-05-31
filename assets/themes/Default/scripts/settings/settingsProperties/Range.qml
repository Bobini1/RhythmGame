import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import "../../common/helpers.js" as Helpers

RowLayout {
    id: range
    height: 30
    property real min: -Infinity
    property real max: Infinity
    property real default_: 0
    property var destination
    property string id_

    Loader {
        active: range.min !== -Infinity && range.max !== Infinity
        Layout.fillWidth: true
        sourceComponent: Component {
            Slider {
                function order(num) {
                    if (!num) {
                        return -1;
                    }
                    return Math.floor(Math.log(Math.abs(num)) / Math.LN10 + 0.000000001);
                }
                readonly property real mult: Math.pow(10,Math.max(order(range.default), 2))
                readonly property real minmult: Math.pow(10,Math.max(order(range.min), 2))
                readonly property real maxmult: 0.500 + 1 / Math.pow(10,Math.max(order(range.max) + 1, 3))

                from: range.min
                to: range.max
                Layout.fillHeight: true
                value: range.destination[range.id_]

                onMoved: {
                    range.destination[range.id_] = Math.round(value * 1000) / 1000
                }
            }
        }
    }
    TextField {
        id: textField
        
        horizontalAlignment: contentWidth >= width ? TextField.AlignLeft : TextField.AlignHCenter
        autoScroll: false
        Layout.preferredWidth: 100
        text: Helpers.getFormattedNumber(range.destination[range.id_])
        Layout.fillHeight: true
        color: acceptableInput ? "black" : "red"
        validator: DoubleValidator {
            id: doubleValidator
            bottom: range.min
            top: range.max
        }
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onTextEdited: {
            if (acceptableInput) {
                range.destination[range.id_] = Number.fromLocaleString(text)
            }
        }

        onEditingFinished: {
            ensureVisible(0);
        }
        onActiveFocusChanged: {
            autoScroll = true;
        }

        Component.onCompleted: {
            ensureVisible(0);
        }
        Layout.alignment: Qt.AlignRight
        HoverHandler {
            id: hoverHandler
        }
        ToolTip.visible: hoverHandler.hovered
        ToolTip.text: "Limits: " + Qt.locale().toString(doubleValidator.bottom, "f", -128) + " - " + Qt.locale().toString(doubleValidator.top, "f", -128)
    }
}

