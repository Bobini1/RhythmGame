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

    OptionsTumbler {
        width: 360
        height: 220

        model: ["NORMAL", "MIRROR", "SPIRAL", "RANDOM", "S-RANDOM", "H-RANDOM", "R-RANDOM", "S-RANDOM+"]
        prop: "noteOrderAlgorithm"
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