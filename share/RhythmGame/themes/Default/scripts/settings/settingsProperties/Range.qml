import QtQuick.Dialogs
import QtQuick.Controls
import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import "../../common/helpers.js" as Helpers
import ".."

RowLayout {
    id: range
    property real min: -Infinity
    property real max: Infinity
    property real sliderMax: max
    property real sliderMin: min
    property real default_: 0
    property var destination
    property string id_
    property alias name: strLabel.text
    property alias description: strLabel.description
    property int decimals: min < -1 || max > 1 ? 0 : 2

    SettingsLabel {
        id: strLabel
    }

    RowLayout {
        id: rangeRow
        Layout.fillWidth: true
        Layout.preferredWidth: 400
        Layout.minimumWidth: 200

        Loader {
            active: range.sliderMin !== -Infinity && range.sliderMax !== Infinity
            Layout.fillWidth: true
            Layout.minimumWidth: 120 - rangeRow.spacing
            Layout.preferredWidth: 120 - rangeRow.spacing
            sourceComponent: Component {
                Slider {
                    from: range.sliderMin
                    to: range.sliderMax
                    Layout.fillHeight: true
                    value: range.destination[range.id_]

                    onMoved: {
                        range.destination[range.id_] = Math.round(value * 10 ** range.decimals) / 10 ** range.decimals
                    }
                }
            }
        }
        SpinBox {
            id: textField

            value: range.destination[range.id_] * 10 ** range.decimals
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.maximumWidth: 200
            Layout.minimumWidth: 120
            Layout.preferredWidth: 120
            Layout.alignment: Qt.AlignRight
            IntValidator {
                id: intRange
            }
            from: range.min == -Infinity ? intRange.bottom : range.min * 10 ** range.decimals
            to: range.max == Infinity ? intRange.top : range.max * 10 ** range.decimals
            stepSize: 1
            editable: true
            validator: DoubleValidator {
                id: doubleValidator
                bottom: range.min
                top: range.max
                locale: Rg.languages.selectedLanguage
            }
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            onValueModified: {
                range.destination[range.id_] = value * 10 ** -range.decimals;
            }
            valueFromText: function(text, locale) {
                return Number.fromLocaleString(locale, text) * 10 ** range.decimals;
            }
            textFromValue: function(value, locale) {
                return Helpers.getFormattedNumber(locale, value * 10 ** -range.decimals, range.decimals);
            }
            HoverHandler {
                id: hoverHandler
            }
            ToolTip.visible: hoverHandler.hovered
            ToolTip.text: "Limits: " + Qt.locale(Rg.languages.selectedLanguage).toString(doubleValidator.bottom, "f", -128) + " - " + Qt.locale(Rg.languages.selectedLanguage).toString(doubleValidator.top, "f", -128)
        }

    }
    ResetButton {
        destination: range.destination
        id_: range.id_
        default_: range.default_

        onClicked: {
            range.destination[range.id_] = range.default_
        }
    }
}