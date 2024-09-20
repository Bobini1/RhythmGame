import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: bg

    color: "white"
    radius: 32

    scale: 0

    width: 1600
    height: 900

    Component {
        id: delegateComponent

        Text {
            text: modelData
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 32

            color: {
                let c = Qt.color("black");
                let value = Math.max(0, 1 - Math.abs(Tumbler.displacement));
                c.hslLightness = value;
                return c;
            }
        }
    }

    OptionsTumbler {
        width: 360
        height: 220

        model: ["NORMAL", "MIRROR", "SPIRAL", "RANDOM", "S-RANDOM", "H-RANDOM", "R-RANDOM", "R-RANDOM"]
        prop: "NoteOrderAlgorithm"
    }

    states: State {
        name: "shown"; when: InputTranslator.start
        PropertyChanges {
            target: bg
            scale: 1
        }
    }

    transitions: Transition {
        NumberAnimation {
            properties: "scale"
            easing.type: Easing.InOutQuad
            duration: 150
        }
    }
}