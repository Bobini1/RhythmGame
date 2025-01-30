import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic


Item {
    id: playOptionsBattle
    width: 1600
    height: 900

    scale: 0

    function duplicate(arr) {
        return arr.concat(arr)
    }

    Side {
        profile: ProfileList.battleProfiles.player1Profile

        TapHandler {
            gesturePolicy: TapHandler.WithinBounds
        }

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
    }

    Side {
        profile: ProfileList.battleProfiles.player2Profile

        TapHandler {
            gesturePolicy: TapHandler.WithinBounds
        }

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
    }

    component Side: Rectangle {
        id: bg

        color: "white"
        radius: 32

        width: parent.width / 2 - 25

        required property Profile profile

        DarkLabel {
            id: p1RandomLabel
            height: 40
            anchors.top: parent.top
            anchors.topMargin: 60
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 20

            text: "Note Order"

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
            profile: bg.profile

            model: [NoteOrderAlgorithm.Normal, NoteOrderAlgorithm.Mirror, NoteOrderAlgorithm.Random, NoteOrderAlgorithm.RandomPlus, NoteOrderAlgorithm.SRandom, NoteOrderAlgorithm.SRandomPlus, NoteOrderAlgorithm.RRandom]
            strings: ["NORMAL", "MIRROR", "RANDOM", "RANDOM+", "S-RANDOM", "S-RANDOM+", "R-RANDOM"]
            prop: "noteOrderAlgorithm"
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
            anchors.leftMargin: 20
            width: 360
            height: 220
            down: BmsKey.Col13
            model: ["AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "HAZARD"]
            strings: ["ASSISTED EASY", "EASY", "NORMAL", "HARD", "EXHARD", "HAZARD"]
            prop: "gaugeType"
            profile: bg.profile
        }

        OptionsTumbler {
            id: hiSpeedFixTumbler
            anchors.top: labelRow.bottom
            anchors.topMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 20
            width: 360
            height: 220
            down: BmsKey.Col15
            model: playOptionsBattle.duplicate([HiSpeedFix.Main, HiSpeedFix.Start, HiSpeedFix.Max, HiSpeedFix.Min, HiSpeedFix.Off])
            strings: playOptionsBattle.duplicate(["MAIN BPM", "START BPM", "MAX BPM", "MIN BPM", "OFF"])
            prop: "hiSpeedFix"
            profile: bg.profile
        }
    }

    states: State {
        name: "shown"; when: playOptionsBattle.enabled
        PropertyChanges {
            target: playOptionsBattle
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
