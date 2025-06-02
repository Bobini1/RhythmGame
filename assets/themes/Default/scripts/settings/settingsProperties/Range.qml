import QtQuick.Dialogs
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import "../../common/helpers.js" as Helpers
import ".."

RowLayout {
    id: range
    property real min: -Infinity
    property real max: Infinity
    property real default_: 0
    property var destination
    property string id_
    property alias name: strLabel.text
    property alias description: strLabel.description

    SettingsLabel {
        id: strLabel
    }

    RowLayout {
        id: rangeRow
        Layout.fillWidth: true
        Layout.preferredWidth: 400
        Layout.minimumWidth: 200

        Loader {
            active: range.min !== -Infinity && range.max !== Infinity
            Layout.fillWidth: true
            Layout.minimumWidth: 120 - rangeRow.spacing
            Layout.preferredWidth: 120 - rangeRow.spacing
            sourceComponent: Component {
                Slider {
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
            text: Helpers.getFormattedNumber(range.destination[range.id_])
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.maximumWidth: 200
            Layout.minimumWidth: 80
            Layout.preferredWidth: 80
            Layout.alignment: Qt.AlignRight
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
            HoverHandler {
                id: hoverHandler
            }
            ToolTip.visible: hoverHandler.hovered
            ToolTip.text: "Limits: " + Qt.locale().toString(doubleValidator.bottom, "f", -128) + " - " + Qt.locale().toString(doubleValidator.top, "f", -128)
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


