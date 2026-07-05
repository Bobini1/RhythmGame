import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: numberWithSlider

    required property string prop
    property alias text: label.text
    property alias to: slider.to
    property alias from: slider.from
    required property var src
    property real decimals: to <= 1 ? 1 : 0
    property bool enforceRange: true
    readonly property real spinBoxWidth: 116
    height: Math.max(48, row.implicitHeight + 10)
    width: ListView.view ? ListView.view.width : 414

    PopupEditorColors {
        id: popupColors
    }

    RowLayout {
        id: row

        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 10

        Text {
            id: label

            Layout.fillWidth: true
            color: popupColors.text
            elide: Text.ElideRight
            font.bold: true
            font.pixelSize: 14
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Slider {
            id: slider

            Layout.preferredWidth: 112
            Layout.minimumWidth: 78
            value: src[numberWithSlider.prop]

            onMoved: {
                src[numberWithSlider.prop] = value;
            }
        }

        SpinBox {
            id: txt

            Layout.preferredHeight: 34
            Layout.preferredWidth: numberWithSlider.spinBoxWidth
            font.pixelSize: 14
            value: src[numberWithSlider.prop] * 10 ** numberWithSlider.decimals

            validator: DoubleValidator {
            }

            IntValidator {
                id: intRange
            }
            from: (numberWithSlider.bottom === -Infinity || !numberWithSlider.enforceRange) ? intRange.bottom : numberWithSlider.from * 10 ** numberWithSlider.decimals
            to: (numberWithSlider.to === Infinity || !numberWithSlider.enforceRange) ? intRange.top : numberWithSlider.to * 10 ** numberWithSlider.decimals
            stepSize: 1
            onValueModified: {
                src[numberWithSlider.prop] = value * 10 ** -numberWithSlider.decimals;
            }
            valueFromText: function(text, locale) {
                return Number.fromLocaleString(locale, text) * 10 ** numberWithSlider.decimals;
            }
            textFromValue: function(value, locale) {
                return Qt.locale().toString(value * 10 ** -numberWithSlider.decimals, "f", numberWithSlider.decimals)
            }
        }
    }
}
