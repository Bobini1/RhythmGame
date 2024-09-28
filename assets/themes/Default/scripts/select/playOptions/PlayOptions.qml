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

    DarkLabel {
        id: p1RandomLabel
        width: 480
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 20

        text: "Note Order (P1)"

        highlightedKeys: [1, 2]
    }

    OptionsTumbler {
        width: 360
        height: 220
        anchors.top: p1RandomLabel.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 20
        up: BmsKey.Col12
        down: BmsKey.Col11

        model: ["NORMAL", "MIRROR", "SPIRAL", "RANDOM", "S-RANDOM", "H-RANDOM", "R-RANDOM", "S-RANDOM+"]
        prop: "noteOrderAlgorithm"
    }

    DarkLabel {
        id: p2RandomLabel
        width: 480
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20

        text: "Note Order (P2)"

        highlightedKeys: [6, 7]
    }

    OptionsTumbler {
        width: 360
        height: 220
        anchors.top: p2RandomLabel.bottom
        anchors.topMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20
        up: BmsKey.Col17
        down: BmsKey.Col16

        model: ["NORMAL", "MIRROR", "SPIRAL", "RANDOM", "S-RANDOM", "H-RANDOM", "R-RANDOM", "S-RANDOM+"]
        prop: "noteOrderAlgorithmP2"
    }

    property var open: false

    Input.onStartPressed: {
        open = true;
    }

    Input.onStartReleased: {
        open = false;
    }

    states: State {
        name: "shown"; when: bg.open
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