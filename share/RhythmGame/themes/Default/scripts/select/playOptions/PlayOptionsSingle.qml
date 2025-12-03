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
    
    TapHandler {
        gesturePolicy: TapHandler.WithinBounds
    }

    DarkLabel {
        id: p1RandomLabel
        width: 600
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 60
        anchors.left: parent.left
        anchors.leftMargin: 20

        text: qsTr("Note Order (P1)")

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
        profile: Rg.profileList.mainProfile

        model: [NoteOrderAlgorithm.Normal, NoteOrderAlgorithm.Mirror, NoteOrderAlgorithm.Random, NoteOrderAlgorithm.RandomPlus, NoteOrderAlgorithm.SRandom, NoteOrderAlgorithm.SRandomPlus, NoteOrderAlgorithm.RRandom]
        strings: qsTr("NORMAL;MIRROR;RANDOM;RANDOM+;S-RANDOM;S-RANDOM+;R-RANDOM").split(";")
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

        text: qsTr("Note Order (P2)")

        highlightedKeys: [6, 7]
    }

    OptionsTumbler {
        anchors.top: p2RandomLabel.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: p2RandomLabel.horizontalCenter
        up: BmsKey.Col17
        down: BmsKey.Col16
        profile: Rg.profileList.mainProfile

        model: [NoteOrderAlgorithm.Normal, NoteOrderAlgorithm.Mirror, NoteOrderAlgorithm.Random, NoteOrderAlgorithm.RandomPlus, NoteOrderAlgorithm.SRandom, NoteOrderAlgorithm.SRandomPlus, NoteOrderAlgorithm.RRandom]
        strings: qsTr("NORMAL;MIRROR;RANDOM;RANDOM+;S-RANDOM;S-RANDOM+;R-RANDOM").split(";")
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

            text: qsTr("Gauge Type")

            highlightedKeys: [3]
        }

        DarkLabel {
            id: dpOptionsLabel
            Layout.fillWidth: true
            height: 40

            text: qsTr("DP Options")

            highlightedKeys: [4]
        }

        DarkLabel {
            id: hiSpeedFixLabel
            Layout.fillWidth: true
            height: 40

            text: qsTr("Hi-Speed Fix")

            highlightedKeys: [5]
        }
    }

    OptionsTumbler {
        id: gaugeTumbler
        anchors.top: labelRow.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 20 + (gaugeLabel.width - width) / 2
        down: BmsKey.Col13
        model: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"]
        strings: qsTr("ASSISTED EASY;EASY;NORMAL;HARD;EXHARD;FC").split(";")
        prop: "gaugeType"
        profile: Rg.profileList.mainProfile
    }

    function duplicate(arr) {
        return arr.concat(arr)
    }

    OptionsTumbler {
        id: dpOptionsTumbler
        anchors.top: labelRow.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        down: BmsKey.Col14
        model: bg.duplicate([DpOptions.Off, DpOptions.Flip, DpOptions.Battle])
        strings: bg.duplicate(qsTr("OFF;FLIP;BATTLE").split(";"))
        prop: "dpOptions"
        profile: Rg.profileList.mainProfile
    }

    OptionsTumbler {
        id: hiSpeedFixTumbler
        anchors.top: labelRow.bottom
        anchors.topMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20 + (hiSpeedFixLabel.width - width) / 2
        down: BmsKey.Col15
        model: bg.duplicate([HiSpeedFix.Main, HiSpeedFix.Start, HiSpeedFix.Max, HiSpeedFix.Min, HiSpeedFix.Avg, HiSpeedFix.Off])
        strings: bg.duplicate(qsTr("MAIN BPM;START BPM;MAX BPM;MIN BPM;AVG BPM;OFF").split(";"))
        prop: "hiSpeedFix"
        profile: Rg.profileList.mainProfile
    }

    states: State {
        name: "shown"; when: bg.enabled
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