import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml

Row {
    id: numberWithSlider
    required property string prop
    property alias text: label.text
    property alias to: slider.to
    property alias from: slider.from

    height: slider.height
    spacing: 10

    Text {
        id: label
        anchors.verticalCenter: slider.verticalCenter
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "white"
        width: 100
    }
    Slider {
        id: slider

        value: root.vars[numberWithSlider.prop]
        width: 320

        onMoved: {
            root.vars[numberWithSlider.prop] = value;
            value = Qt.binding(() => root.vars[numberWithSlider.prop]);
        }
    }
    TextField {
        id: txt

        text: Qt.locale().toString(root.vars[numberWithSlider.prop], "f", numberWithSlider.to <= 1 ? 1 : 0)
        font.pixelSize: 20
        width: 50
        height: slider.height
        horizontalAlignment: Text.AlignHCenter

        validator: DoubleValidator {
        }

        onAccepted: {
            root.vars[numberWithSlider.prop] = Number.fromLocaleString(text);
            text = Qt.binding(() => Qt.locale().toString(root.vars[numberWithSlider.prop]));
        }
    }
}
