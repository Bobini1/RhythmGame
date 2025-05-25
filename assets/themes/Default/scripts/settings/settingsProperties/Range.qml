import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import "../../common/helpers.js" as Helpers

RowLayout {
    height: 30
    Loader {
        active: props.min !== undefined && props.max !== undefined
        Layout.fillWidth: active
        sourceComponent: Component {
            Slider {
                function order(num) {
                    if (!num) {
                        return -1;
                    }
                    return Math.floor(Math.log(Math.abs(num)) / Math.LN10 + 0.000000001);
                }
                readonly property real mult: Math.pow(10,Math.max(order(props.default), 2))
                readonly property real minmult: Math.pow(10,Math.max(order(props.min), 2))
                readonly property real maxmult: 0.500 + 1 / Math.pow(10,Math.max(order(props.max) + 1, 3))

                property var f: {
                    if (props.min !== undefined && props.max !== undefined) {
                        return num => num;
                    }
                    else if (props.min === undefined && props.max === undefined) {
                        return num => {
                            if (num === 1) {
                                return Infinity;
                            }
                            if (num === 0) {
                                return -Infinity;
                            }
                            return mult / 2 * (-2*num+1)/((2*num-2)*num);
                        }
                    } else if (props.min !== undefined) {
                        return num => {
                            if (num === 1) {
                                return Infinity
                            }
                            return props.min + minmult * num / (1 - num);
                        }
                    } else if (props.max !== undefined) {
                        return num => {
                            if (num === 0) {
                                return -Infinity
                            }
                            return (-Math.log(num) / Math.log(0.5 / maxmult)) + props.max
                        }
                    }
                }
                property var inverseF: {
                    if (props.min !== undefined && props.max !== undefined) {
                        return num => num;
                    }
                    else if (props.min === undefined && props.max === undefined) {
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
                    } else if (props.min !== undefined) {
                        return num => {
                            if (num === Infinity) {
                                return 1
                            }
                            if (num === props.min) {
                                return 0
                            }
                            return (num - props.min) / (minmult + num - props.min + 1);
                        }
                    } else if (props.max !== undefined) {
                        return num => {
                            if (num === -Infinity) {
                                return 0
                            }
                            if (num === props.max) {
                                return 1
                            }
                            return Math.pow(0.5 / maxmult, -num + props.max)
                        }
                    }
                }
                from: props.max === undefined || props.min === undefined ? 0 : props.min
                to: props.max === undefined || props.min === undefined ? 1 : props.max
                Layout.fillHeight: true
                value: inverseF(destination[props.id])

                onMoved: {
                    destination[props.id] = Math.round(f(value) * 1000) / 1000
                    value = Qt.binding(() => inverseF(destination[props.id]))
                }
            }
        }
    }
    TextField {
        id: textField
        
        horizontalAlignment: contentWidth >= width ? TextField.AlignLeft : TextField.AlignHCenter
        autoScroll: false
        Layout.preferredWidth: textMetrics.width + 20
        text: Helpers.getFormattedNumber(destination[props.id])
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
            text = Qt.binding(() => Helpers.getFormattedNumber(destination[props.id]))
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
                if (props.max !== undefined) {
                    str = Helpers.getFormattedNumber(props.max * 10);
                }
                if (props.min !== undefined) {
                    let minStr = Helpers.getFormattedNumber(props.min * 10);
                    if (minStr.length > str.length) {
                        str = minStr;
                    }
                }
                let defStr = Helpers.getFormattedNumber(props.default * 10);
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