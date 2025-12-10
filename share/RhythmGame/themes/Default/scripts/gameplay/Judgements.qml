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

        ShaderEffectSource {
            id: judgementAnimation

            sourceItem: {
                switch (judgement.lastJudgement) {
                    case Judgement.Perfect:
                        return pgreatImage;
                    case Judgement.Great:
                        return greatImage;
                    case Judgement.Good:
                        return goodImage;
                    case Judgement.Bad:
                        return goodImage;
                    case Judgement.Poor:
                    case Judgement.EmptyPoor:
                        return poorImage;
                    default:
                        return pgreatImage;
                }
            }
            width: sourceItem ? sourceItem.width : 0
            height: sourceItem ? sourceItem.height : 0
            opacity: judgement.lastJudgement !== null ? 1 : 0
        }

        Repeater {
            id: comboNumber

            model: judgement.combo > 0
                ? Array.from(judgement.combo.toString(), Number)
                : []

            delegate: ShaderEffectSource {
                sourceItem: lastJudgement === Judgement.Perfect ? pgreatDigits.itemAt(modelData) : greatDigits.itemAt(modelData)
                width: sourceItem.width
                height: sourceItem.height
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
        delegate: AnimatedSprite {
            interpolate: false
            frameDuration: 40
            frameCount: 3
            frameHeight: 84
            frameWidth: 55
            height: judgement.height
            width: frameWidth * (judgement.height / frameHeight)
            source: root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat_" + modelData
            opacity: 0
            currentFrame: pgreatImage.currentFrame
            running: false
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
    AnimatedSprite {
        id: pgreatImage

        interpolate: false
        frameDuration: 40
        frameHeight: 84
        frameWidth: 227
        frameCount: 3
        height: judgement.height
        width: frameWidth * (judgement.height / frameHeight)
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat"
        opacity: 0
    }
    Image {
        id: greatImage

        height: judgement.height
        width: sourceSize.width * (judgement.height / sourceSize.height)
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/great"
        opacity: 0
    }
    Image {
        id: goodImage

        height: judgement.height
        width: sourceSize.width * (judgement.height / sourceSize.height)
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/good"
        opacity: 0
    }
    Image {
        id: poorImage

        height: judgement.height
        width: sourceSize.width * (judgement.height / sourceSize.height)
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/poor"
        opacity: 0
    }
}
