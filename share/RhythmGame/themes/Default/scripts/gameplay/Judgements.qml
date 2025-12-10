import QtQuick
import RhythmGameQml
import QtQuick.Effects

Item {
    id: judgement

    width: judgementRow.width
    z: 3
    visible: false

    required property var score
    required property string judge
    required property var columns

    // turn invisible after one second of no notes
    Timer {
        id: hidingTimer

        interval: 500
        onTriggered: {
            judgement.visible = false;
        }
    }

    property int pgreatFrame: 0
    NumberAnimation on pgreatFrame {
        from: 0
        to: 2
        duration: 120
        loops: Animation.Infinite
    }

    property int combo: 0

    Connections {
        function onHit(hit) {
            if (!hit.points) {
                return;
            }
            if (!judgement.columns.includes(hit.column)) {
                return;
            }

            Qt.callLater(function() {
                judgement.combo = judgement.score.combo;
                judgement.visible = true;
                hidingTimer.restart();
            });
        }

        target: judgement.score
    }

    SequentialAnimation {
        id: judgementAnimationFlashing

        loops: Animation.Infinite
        running: lastJudgement !== Judgement.Perfect && lastJudgement !== null

        PropertyAction {
            property: "visible"
            target: judgementRow
            value: true
        }

        PauseAnimation {
            duration: 40
        }

        PropertyAction {
            property: "visible"
            target: judgementRow
            value: false
        }

        PauseAnimation {
            duration: 40
        }

        PropertyAction {
            property: "visible"
            target: judgementRow
            value: true
        }
    }

    property var lastJudgement: null

    Row {
        id: judgementRow

        spacing: 0
        height: parent.height

        Image {
            id: judgementAnimation
            function sourceForJudgement(judgementType) {
                switch (judgementType) {
                case Judgement.Perfect:
                    return pgreatImage.source;
                case Judgement.Great:
                    return greatImage.source;
                case Judgement.Good:
                    return goodImage.source;
                case Judgement.Poor:
                    return poorImage.source;
                default:
                    return pgreatImage.source;
                }
            }
            source: sourceForJudgement(judgement.lastJudgement)
            height: judgement.height
            width: sourceSize.width * (judgement.height / sourceSize.height)
            opacity: judgement.lastJudgement !== null ? 1 : 0
        }

        Repeater {
            id: comboNumber

            model: judgement.combo > 0
                ? Array.from(judgement.combo.toString(), Number)
                : []

            delegate: Image {
                source: lastJudgement === Judgement.Perfect ? pgreatDigits.itemAt(modelData).source : greatDigits.itemAt(modelData).source
                height: judgement.height
                width: sourceSize.width * (judgement.height / sourceSize.height)
            }
        }
    }

    Connections {
        function onHit(tap) {
            if (!tap.points) {
                return;
            }
            if (!judgement.columns.includes(tap.column)) {
                return;
            }
            // ignore mine hits etc.
            if (tap.points.judgement > Judgement.Perfect) {
                return;
            }
            judgement.lastJudgement = tap.points.judgement;
        }

        target: judgement.score
    }
    Repeater {
        id: pgreatDigits
        model: 10
        delegate: Image {
            source: root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat_" + modelData + "_f" + judgement.pgreatFrame
            opacity: 0
        }
    }
    Repeater {
        id: greatDigits
        model: 10
        delegate: Image {
            source: root.iniImagesUrl + "judge/" + judgement.judge + "/great_" + modelData
            opacity: 0
        }
    }
    Image {
        id: pgreatImage

        source: root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat_f" + judgement.pgreatFrame
        opacity: 0
    }
    Image {
        id: greatImage

        source: root.iniImagesUrl + "judge/" + judgement.judge + "/great"
        opacity: 0
    }
    Image {
        id: goodImage

        source: root.iniImagesUrl + "judge/" + judgement.judge + "/good"
        opacity: 0
    }
    Image {
        id: poorImage

        source: root.iniImagesUrl + "judge/" + judgement.judge + "/poor"
        opacity: 0
    }
}
