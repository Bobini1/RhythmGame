import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import "../../common/helpers.js" as Helpers

RowLayout {
    id: range
    height: 30
    property real min
    property real max
    property real default_: 0
    property var destination
    property string id_

    Loader {
        active: range.min !== undefined && range.max !== undefined
        Layout.fillWidth: active
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

                property var f: {
                    if (range.min !== undefined && range.max !== undefined) {
                        return num => num;
                    }
                    else if (range.min === undefined && range.max === undefined) {
                        return num => {
                            if (num === 1) {
                                return Infinity;
                            }
                            if (num === 0) {
                                return -Infinity;
                            }
                            return mult / 2 * (-2*num+1)/((2*num-2)*num);
                        }
                    } else if (range.min !== undefined) {
                        return num => {
                            if (num === 1) {
                                return Infinity
                            }
                            return range.min + minmult * num / (1 - num);
                        }
                    } else if (range.max !== undefined) {
                        return num => {
                            if (num === 0) {
                                return -Infinity
                            }
                            return (-Math.log(num) / Math.log(0.5 / maxmult)) + range.max
                        }
                    }
                }
                property var inverseF: {
                    if (range.min !== undefined && range.max !== undefined) {
                        return num => num;
                    }
                    else if (range.min === undefined && range.max === undefined) {
                        return num => {
                            if (num === 0) {
                                return 0.5
                            }
                            if (num === Infinity) {
                                return 1
                            }
                            if (num === -Infinity) {
                                return 0
                            }
                            return (2 * num - mult + Math.sqrt(Math.pow(mult, 2) + 4 * Math.pow(num, 2))) / (4 * num);
                        }
                    } else if (range.min !== undefined) {
                        return num => {
                            if (num === Infinity) {
                                return 1
                            }
                            if (num === range.min) {
                                return 0
                            }
                            return (num - range.min) / (minmult + num - range.min + 1);
                        }
                    } else if (range.max !== undefined) {
                        return num => {
                            if (num === -Infinity) {
                                return 0
                            }
                            if (num === range.max) {
                                return 1
                            }
                            return Math.pow(0.5 / maxmult, -num + range.max)
                        }
                    }
                }
                from: range.max === undefined || range.min === undefined ? 0 : range.min
                to: range.max === undefined || range.min === undefined ? 1 : range.max
                Layout.fillHeight: true
                value: inverseF(range.destination[range.id_])

                onMoved: {
                    range.destination[range.id_] = Math.round(f(value) * 1000) / 1000
                    value = Qt.binding(() => inverseF(range.destination[range.id_]))
                }
            }
        }
    }
    TextField {
        id: textField
        
        horizontalAlignment: contentWidth >= width ? TextField.AlignLeft : TextField.AlignHCenter
        autoScroll: false
        Layout.preferredWidth: textMetrics.width + 20
        text: Helpers.getFormattedNumber(range.destination[range.id_])
        Layout.fillHeight: true
        color: acceptableInput ? "black" : "red"
        validator: DoubleValidator {
            id: doubleValidator
            bottom: range.min !== undefined ? range.min : -Infinity; top: range.max !== undefined ? range.max : Infinity
        }
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onTextEdited: {
            // noinspection SillyAssignmentJS
            text = text
            if (acceptableInput) {
                range.destination[range.id_] = Number.fromLocaleString(text)
            }
        }

        onEditingFinished: {
            text = Qt.binding(() => Helpers.getFormattedNumber(range.destination[range.id_]))
            ensureVisible(0);
        }
        onActiveFocusChanged: {
            autoScroll = true;
        }

        Component.onCompleted: {
            ensureVisible(0);
        }
        TextMetrics {
            id: textMetrics
            font: textField.font

            text: {
                let length = 0;
                let str = ""
                if (range.max !== undefined) {
                    str = Helpers.getFormattedNumber(range.max * 10);
                }
                if (range.min !== undefined) {
                    let minStr = Helpers.getFormattedNumber(range.min * 10);
                    if (minStr.length > str.length) {
                        str = minStr;
                    }
                }
                let defStr = Helpers.getFormattedNumber(range.default * 10);
                if (defStr.length > str.length) {
                    str = defStr;
                }
                let thousandStr = Helpers.getFormattedNumber(1000);
                if (str.length < thousandStr.length) {
                    str = thousandStr;
                }
                return str;
            }
        }
        Layout.alignment: Qt.AlignRight
        HoverHandler {
            id: hoverHandler
        }
        ToolTip.visible: hoverHandler.hovered
        ToolTip.text: "Limits: " + Qt.locale().toString(doubleValidator.bottom, "f", -128) + " - " + Qt.locale().toString(doubleValidator.top, "f", -128)
    }
}