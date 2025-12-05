import QtQuick
import QtQuick.Controls
import RhythmGameQml

Row {
    id: numberWithSlider
    required property string prop
    property alias text: label.text
    property alias to: slider.to
    property alias from: slider.from
    required property var src
    property real decimals: to <= 1 ? 1 : 0
    height: 40
    spacing: 10

    Text {
        id: label
        anchors.verticalCenter: slider.verticalCenter
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "white"
        width: 160
        fontSizeMode: Text.Fit
    }
    Slider {
        id: slider

        value: src[numberWithSlider.prop]
        width: 180

        onMoved: {
            src[numberWithSlider.prop] = value;
        }
    }
    SpinBox {
        id: txt

        value: src[numberWithSlider.prop] * 10 ** numberWithSlider.decimals
        font.pixelSize: 18
        width: 140
        height: parent.height
        anchors.verticalCenter: slider.verticalCenter

        validator: DoubleValidator {
        }

        IntValidator {
            id: intRange
        }
        from: (numberWithSlider.bottom === -Infinity ? intRange.bottom : numberWithSlider.from) * 10 ** numberWithSlider.decimals
        to: (numberWithSlider.to === Infinity ? intRange.top : numberWithSlider.to) * 10 ** numberWithSlider.decimals
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
