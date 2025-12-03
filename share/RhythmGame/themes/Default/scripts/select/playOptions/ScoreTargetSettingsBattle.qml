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
        id: scoreTargetLabel
        width: 600
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 60
        anchors.left: parent.left
        anchors.leftMargin: 20

        text: qsTr("Score Target")

        highlightedKeys: [1, 2]
    }

    OptionsTumbler {
        id: scoreTarget
        width: 360
        height: 220
        anchors.top: p1RandomLabel.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: p1RandomLabel.horizontalCenter
        up: BmsKey.Col12
        down: BmsKey.Col11
        profile: Rg.profileList.mainProfile

        model: [ScoreTarget.Fraction, ScoreTarget.BestScore, ScoreTarget.LastScore]
        strings: qsTr("GRADE;BEST SCORE;LAST SCORE").split(";")
        prop: "scoreTarget"
    }

    DarkLabel {
        id: scoreTargetLabel
        width: 600
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 60
        anchors.left: parent.left
        anchors.leftMargin: 20

        text: qsTr("Score Target")

        highlightedKeys: [1, 2]
    }

    OptionsTumbler {
        id: scoreTarget
        width: 360
        height: 220
        anchors.top: p1RandomLabel.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: p1RandomLabel.horizontalCenter
        up: BmsKey.Col14
        down: BmsKey.Col13
        profile: Rg.profileList.mainProfile

        model: [1, 8.5 / 9, 8 / 9, 7.5 / 9, 7 / 9, 6.5 / 9, 6 / 9, 5.5 / 9, 5 / 9, 4 / 9, 3 / 9, 2 / 9, 1 / 9]
        strings: qsTr("MAX;MAX-;AAA;AAA-;AA;AA-;A;A-;B;C;D;E;F").split(";")
        prop: "scoreTarget"
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