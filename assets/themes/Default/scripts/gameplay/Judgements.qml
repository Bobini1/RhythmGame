import QtQuick
import RhythmGameQml

Item {
    id: judgement

    height: childrenRect.height
    width: childrenRect.width
    z: 3

    // turn invisible after one second of no notes
    Timer {
        id: hidingTimer

        interval: 500
        onTriggered: {
            judgement.visible = false;
        }
    }

    Connections {
        function onComboChanged() {
            Qt.callLater(function() {
                judgement.visible = true;
                hidingTimer.restart();
            });
        }

        target: chart.score
    }

    SequentialAnimation {
        id: judgementAnimationFlashing

        loops: Animation.Infinite
        running: false

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

    Row {
        id: judgementRow

        spacing: 0

        AnimatedSprite {
            id: judgementAnimation

            frameDuration: 40
            frameHeight: 84
            frameWidth: 227
            interpolate: false
            onSourceChanged: {
                if (source != root.iniImagesUrl + "judge/" + root.vars.judge + "/pgreat") {
                    judgementAnimationFlashing.start();
                } else {
                    judgementAnimationFlashing.stop();
                    judgementRow.visible = true;
                }
            }
        }

        Repeater {
            id: comboNumber

            model: chart.score.combo > 0 && judgementAnimation.source != root.iniImagesUrl + "judge/" + root.vars.judge + "/poor" ? chart.score.combo.toString().split("") : []

            AnimatedSprite {
                id: comboNumberAnimation

                currentFrame: judgementAnimation.currentFrame
                frameCount: judgementAnimation.source == root.iniImagesUrl + "judge/" + root.vars.judge + "/pgreat" ? 3 : 1
                frameHeight: 84
                frameWidth: 55
                paused: true
                source: root.iniImagesUrl + "judge/" + root.vars.judge + (judgementAnimation.source == root.iniImagesUrl + "judge/" + root.vars.judge + "/pgreat" ? "/pgreat_" : "/great_") + modelData
            }

        }

    }

    Connections {
        function onLnEndMissed(_) {
            judgementAnimation.frameCount = 1;
            judgementAnimation.source = root.iniImagesUrl + "judge/" + root.vars.judge + "/poor";
        }

        function onMissed(misses) {
            judgementAnimation.frameCount = 1;
            judgementAnimation.source = root.iniImagesUrl + "judge/" + root.vars.judge + "/poor";
        }

        function onNoteHit(tap) {
            switch (tap.points.judgement) {
            case Judgement.Perfect:
                judgementAnimation.frameCount = 3;
                judgementAnimation.source = root.iniImagesUrl + "judge/" + root.vars.judge + "/pgreat";
                break;
            case Judgement.Great:
                judgementAnimation.frameCount = 1;
                judgementAnimation.source = root.iniImagesUrl + "judge/" + root.vars.judge + "/great";
                break;
            case Judgement.Good:
                judgementAnimation.frameCount = 1;
                judgementAnimation.source = root.iniImagesUrl + "judge/" + root.vars.judge + "/good";
                break;
            case Judgement.Bad:
                judgementAnimation.frameCount = 1;
                judgementAnimation.source = root.iniImagesUrl + "judge/" + root.vars.judge + "/bad";
                break;
            default:
                judgementAnimation.frameCount = 1;
                judgementAnimation.source = root.iniImagesUrl + "judge/" + root.vars.judge + "/poor";
                break;
            }
        }

        target: chart.score
    }
    // preload images

    Image {
        height: 0
        source: root.iniImagesUrl + "judge/" + root.vars.judge + "/pgreat"
        visible: false
        width: 0
    }

    Image {
        height: 0
        source: root.iniImagesUrl + "judge/" + root.vars.judge + "/pgreat_0"
        visible: false
        width: 0
    }

}
