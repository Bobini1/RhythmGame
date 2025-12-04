import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic


Item {
    id: scoreTargetSettings
    width: 1600
    height: 900

    scale: 0

    function duplicate(arr) {
        return arr.concat(arr)
    }

    Side {
        profile: Rg.profileList.battleProfiles.player1Profile

        TapHandler {
            gesturePolicy: TapHandler.WithinBounds
        }

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
    }

    Side {
        profile: Rg.profileList.battleProfiles.player2Profile
        player: 2

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
        property int player: 1

        function duplicate(arr) {
            return arr.concat(arr)
        }

        DarkLabel {
            id: scoreTargetTypeLabel
            height: 40
            anchors.top: parent.top
            anchors.topMargin: 60
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 20

            text: qsTr("Score Target Type")

            highlightedKeys: [1, 2]
        }

        OptionsTumbler {
            id: scoreTargetTypeTumbler
            anchors.top: scoreTargetTypeLabel.bottom
            anchors.topMargin: 20
            anchors.horizontalCenter: scoreTargetTypeLabel.horizontalCenter
            up: BmsKey[`Col${bg.player}2`]
            down: BmsKey[`Col${bg.player}1`]
            profile: bg.profile

            model: bg.duplicate([ScoreTarget.Fraction, ScoreTarget.BestScore, ScoreTarget.LastScore])
            strings: bg.duplicate(qsTr("GRADE;BEST SCORE;LAST SCORE").split(";"))
            prop: "noteOrderAlgorithm"
        }

        DarkLabel {
            id: scoreTargetLabel
            height: 40

            anchors.top: scoreTargetTypeTumbler.bottom
            anchors.topMargin: 180
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 20

            text: qsTr("Target Grade")

            highlightedKeys: [3, 4]
        }

        OptionsTumbler {
            id: scoreTarget
            anchors.top: scoreTargetLabel.bottom
            anchors.topMargin: 20
            anchors.horizontalCenter: scoreTargetTypeLabel.horizontalCenter

            up: BmsKey[`Col${bg.player}4`]
            down: BmsKey[`Col${bg.player}3`]
            profile: bg.profile

            model: {
                let base = [1, 8.5 / 9, 8 / 9, 7.5 / 9, 7 / 9, 6.5 / 9, 6 / 9, 5.5 / 9, 5 / 9, 4 / 9, 3 / 9, 2 / 9, 0];
                // This will help when something else was set in the main menu
                let currentChoice = bg.profile.vars.generalVars.targetScoreFraction;
                if (!(base.includes(currentChoice))) {
                    base.unshift(currentChoice);
                }
                return base;
            }
            strings: {
                let base = qsTr("MAX;MAX-;AAA;AAA-;AA;AA-;A;A-;B;C;D;E;F").split(";");
                if (base.length < scoreTarget.model.length) {
                    base.unshift((bg.profile.vars.generalVars.targetScoreFraction * 100).toLocaleString(Qt.locale(Rg.languages.selectedLanguage)) + "%");
                }
                return base;
            }
            prop: "targetScoreFraction"
        }
    }

    states: State {
        name: "shown"; when: scoreTargetSettings.enabled
        PropertyChanges {
            target: scoreTargetSettings
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
