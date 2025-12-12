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

    Timer {
        id: hidingTimer
        interval: 500
        onTriggered: {
            judgement.visible = false;
        }
    }

    property int combo: 0
    property var lastJudgement: null
    property int pgreatFrame: 0

    NumberAnimation on pgreatFrame {
        from: 0
        to: 2
        duration: 120
        loops: Animation.Infinite
    }

    Connections {
        function onHit(hit) {
            if (!hit.points) return;
            if (!judgement.columns.includes(hit.column)) return;
            if (hit.points.judgement > Judgement.Perfect) return;

            judgement.lastJudgement = hit.points.judgement;
            judgement.combo = judgement.score.combo;
            judgement.visible = true;
            hidingTimer.restart();
            console.info("Judgement:", hit.points.judgement, "Combo:", judgement.combo);
        }
        target: judgement.score
    }

    onLastJudgementChanged: {
        if (lastJudgement === Judgement.Perfect) {
            judgementAnimationFlashing.stop();
            judgementRow.visible = true;
        } else {
            judgementAnimationFlashing.restart();
        }
    }

    SequentialAnimation {
        id: judgementAnimationFlashing
        loops: Animation.Infinite
        running: false

        PropertyAction { property: "visible"; target: judgementRow; value: true }
        PauseAnimation { duration: 40 }
        PropertyAction { property: "visible"; target: judgementRow; value: false }
        PauseAnimation { duration: 40 }
        PropertyAction { property: "visible"; target: judgementRow; value: true }
    }

    Row {
        id: judgementRow
        spacing: 0
        height: parent.height

        Image {
            id: judgementAnimation
            source: {
                switch (judgement.lastJudgement) {
                    case Judgement.Perfect: return pgreatImage.source;
                    case Judgement.Great: return greatImage.source;
                    case Judgement.Good: return goodImage.source;
                    case Judgement.Bad: return badImage.source;
                    case Judgement.Poor:
                    case Judgement.EmptyPoor: return poorImage.source;
                    default: return pgreatImage.source;
                }
            }
            height: judgement.height
            width: sourceSize.width * (judgement.height / sourceSize.height)
            visible: judgement.lastJudgement !== null
        }

        Repeater {
            id: comboNumber

            model: judgement.combo > 0 && judgement.lastJudgement !== Judgement.EmptyPoor
                ? Array.from(judgement.combo.toString(), Number)
                : []

            delegate: Image {
                source: lastJudgement === Judgement.Perfect ? pgreatDigits.itemAt(modelData).source : greatDigits.itemAt(modelData).source
                height: judgement.height
                width: sourceSize.width * (judgement.height / sourceSize.height)
            }
        }
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
        id: badImage

        source: root.iniImagesUrl + "judge/" + judgement.judge + "/bad"
        opacity: 0
    }
    Image {
        id: poorImage

        source: root.iniImagesUrl + "judge/" + judgement.judge + "/poor"
        opacity: 0
    }
}
