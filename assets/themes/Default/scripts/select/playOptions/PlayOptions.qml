import RhythmGameQml
import QtQuick
import QtQuick.Layouts
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
        width: 600
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 60
        anchors.left: parent.left
        anchors.leftMargin: 20

        text: "Note Order (P1)"

        highlightedKeys: [1, 2]
    }

    OptionsTumbler {
        id: p1RandomTumbler
        width: 360
        height: 220
        anchors.top: p1RandomLabel.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: p1RandomLabel.horizontalCenter
        up: BmsKey.Col12
        down: BmsKey.Col11

        model: [NoteOrderAlgorithm.Normal, NoteOrderAlgorithm.Mirror, NoteOrderAlgorithm.Spiral, NoteOrderAlgorithm.Random, NoteOrderAlgorithm.SRandom, NoteOrderAlgorithm.HRandom, NoteOrderAlgorithm.RRandom, NoteOrderAlgorithm.SRandomPlus]
        strings: ["NORMAL", "MIRROR", "SPIRAL", "RANDOM", "S-RANDOM", "H-RANDOM", "R-RANDOM", "S-RANDOM+"]
        prop: "noteOrderAlgorithm"
    }

    DarkLabel {
        id: p2RandomLabel
        width: 600
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 60
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
        anchors.horizontalCenter: p2RandomLabel.horizontalCenter
        up: BmsKey.Col17
        down: BmsKey.Col16

        model: [NoteOrderAlgorithm.Normal, NoteOrderAlgorithm.Mirror, NoteOrderAlgorithm.Spiral, NoteOrderAlgorithm.Random, NoteOrderAlgorithm.SRandom, NoteOrderAlgorithm.HRandom, NoteOrderAlgorithm.RRandom, NoteOrderAlgorithm.SRandomPlus]
        strings: ["NORMAL", "MIRROR", "SPIRAL", "RANDOM", "S-RANDOM", "H-RANDOM", "R-RANDOM", "S-RANDOM+"]
        prop: "noteOrderAlgorithmP2"
    }

    RowLayout {
        id: labelRow
        anchors.top: p1RandomTumbler.bottom
        anchors.topMargin: 180
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20
        spacing: 60
        height: 40

        DarkLabel {
            id: gaugeLabel
            Layout.fillWidth: true
            height: 40

            text: "Gauge Type"

            highlightedKeys: [3]
        }

        DarkLabel {
            id: dpOptionsLabel
            Layout.fillWidth: true
            height: 40

            text: "DP Options"

            highlightedKeys: [4]
        }

        DarkLabel {
            id: hiSpeedFixLabel
            Layout.fillWidth: true
            height: 40

            text: "Hi-Speed Fix"

            highlightedKeys: [5]
        }
    }

    OptionsTumbler {
        id: gaugeTumbler
        anchors.top: labelRow.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 20 + (gaugeLabel.width - width) / 2
        width: 360
        height: 220
        down: BmsKey.Col13
        model: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
        strings: ["ASSISTED EASY", "EASY", "NORMAL", "HARD", "EXHARD", "HAZARD"]
        prop: "gaugeType"
    }

    function duplicate(arr) {
        return arr.concat(arr)
    }

    OptionsTumbler {
        id: dpOptionsTumbler
        width: 360
        height: 220
        anchors.top: labelRow.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        down: BmsKey.Col14
        model: bg.duplicate([DpOptions.Off, DpOptions.Flip, DpOptions.Battle, DpOptions.BattleAs])
        strings: bg.duplicate(["OFF", "FLIP", "BATTLE", "BATTLE AS"])
        prop: "dpOptions"
    }

    OptionsTumbler {
        id: hiSpeedFixTumbler
        anchors.top: labelRow.bottom
        anchors.topMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20 + (hiSpeedFixLabel.width - width) / 2
        width: 360
        height: 220
        down: BmsKey.Col15
        model: bg.duplicate([HiSpeedFix.Main, HiSpeedFix.Start, HiSpeedFix.Max, HiSpeedFix.Min, HiSpeedFix.Off])
        strings: bg.duplicate(["MAIN BPM", "START BPM", "MAX BPM", "MIN BPM", "OFF"])
        prop: "hiSpeedFix"
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