import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml

Row {
    id: numberWithSlider
    required property string prop
    property alias text: label.text
    property alias to: slider.to
    property alias from: slider.from
    required property var src

    height: slider.height
    spacing: 10

    Text {
        id: label
        anchors.verticalCenter: slider.verticalCenter
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "white"
        width: 110
    }
    Slider {
        id: slider

        value: src[numberWithSlider.prop]
        width: 300

        onMoved: {
            src[numberWithSlider.prop] = value;
            value = Qt.binding(() => src[numberWithSlider.prop]);
        }
    }
    TextField {
        id: txt

        text: Qt.locale().toString(src[numberWithSlider.prop], "f", numberWithSlider.to <= 1 ? 1 : 0)
        font.pixelSize: 20
        width: 70
        height: slider.height
        horizontalAlignment: Text.AlignHCenter

        validator: DoubleValidator {
        }

        onAccepted: {
            src[numberWithSlider.prop] = Number.fromLocaleString(text);
            text = Qt.binding(() => Qt.locale().toString(src[numberWithSlider.prop]));
        }
    }
}
