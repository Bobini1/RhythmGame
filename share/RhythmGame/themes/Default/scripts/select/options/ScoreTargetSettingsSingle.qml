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

    function duplicate(arr) {
        return arr.concat(arr)
    }

    DarkLabel {
        id: scoreTargetTypeLabel
        width: 600
        height: 40
        anchors.top: parent.top
        anchors.topMargin: 60
        anchors.horizontalCenter: parent.horizontalCenter

        text: qsTr("Score Target Type")

        highlightedKeys: [1, 2]
    }

    OptionsTumbler {
        id: scoreTargetType
        anchors.top: scoreTargetTypeLabel.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: scoreTargetTypeLabel.horizontalCenter
        up: BmsKey.Col12
        down: BmsKey.Col11
        profile: Rg.profileList.mainProfile

        model: bg.duplicate([ScoreTarget.Fraction, ScoreTarget.BestScore, ScoreTarget.LastScore])
        strings: bg.duplicate(qsTr("GRADE;BEST SCORE;LAST SCORE").split(";"))
        prop: "scoreTarget"
    }

    DarkLabel {
        id: scoreTargetLabel
        width: 600
        height: 40
        anchors.top: scoreTargetType.bottom
        anchors.topMargin: 180
        anchors.horizontalCenter: scoreTargetType.horizontalCenter

        text: qsTr("Target Grade")

        highlightedKeys: [3, 4]
    }

    OptionsTumbler {
        id: scoreTarget
        anchors.top: scoreTargetLabel.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: scoreTargetLabel.horizontalCenter
        up: BmsKey.Col14
        down: BmsKey.Col13
        profile: Rg.profileList.mainProfile

        model: {
            let base = [1, 8.5 / 9, 8 / 9, 7.5 / 9, 7 / 9, 6.5 / 9, 6 / 9, 5.5 / 9, 5 / 9, 4 / 9, 3 / 9, 2 / 9, 0];
            // This will help when something else was set in the main menu
            let currentChoice = Rg.profileList.mainProfile.vars.generalVars.targetScoreFraction;
            if (!(base.includes(currentChoice))) {
                base.unshift(currentChoice);
            }
            return base;
        }
        strings: {
            let base = qsTr("MAX;MAX-;AAA;AAA-;AA;AA-;A;A-;B;C;D;E;F").split(";");
            if (base.length < scoreTarget.model.length) {
                base.unshift((Rg.profileList.mainProfile.vars.generalVars.targetScoreFraction * 100).toLocaleString(Qt.locale(Rg.languages.selectedLanguage)) + "%");
            }
            return base;
        }
        prop: "targetScoreFraction"
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