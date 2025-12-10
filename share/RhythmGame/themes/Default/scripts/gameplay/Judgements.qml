import QtQuick
import RhythmGameQml

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
        height: parent.height

        AnimatedSprite {
            id: judgementAnimation

            frameDuration: 40
            frameHeight: 84
            frameWidth: 227
            height: judgementRow.height
            width: frameWidth * (judgement.height / frameHeight)
            interpolate: false
            onSourceChanged: {
                if (source != root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat") {
                    judgementAnimationFlashing.start();
                } else {
                    judgementAnimationFlashing.stop();
                    judgementRow.visible = true;
                }
            }
        }

        Repeater {
            id: comboNumber

            model: judgement.combo > 0 && judgementAnimation.source != root.iniImagesUrl + "judge/" + judgement.judge + "/poor" ? judgement.combo.toString().split("") : []

            AnimatedSprite {
                id: comboNumberAnimation

                currentFrame: judgementAnimation.currentFrame
                frameCount: judgementAnimation.source == root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat" ? 3 : 1
                frameHeight: 84
                frameWidth: 55
                height: judgementRow.height
                width: frameWidth * (judgement.height / frameHeight)
                paused: true
                source: root.iniImagesUrl + "judge/" + judgement.judge + (judgementAnimation.source == root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat" ? "/pgreat_" : "/great_") + modelData
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
            switch (tap.points.judgement) {
                case Judgement.Perfect:
                    judgementAnimation.frameCount = 3;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat";
                    break;
                case Judgement.Great:
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/great";
                    break;
                case Judgement.Good:
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/good";
                    break;
                case Judgement.Bad:
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/bad";
                    break;
                case Judgement.Poor:
                case Judgement.EmptyPoor:
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/poor";
                    break;
            }
        }

        target: judgement.score
    }
    // preload images

    Image {
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat"
        opacity: 0
    }
    Image {
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/great"
        opacity: 0
    }
    Image {
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/good"
        opacity: 0
    }
    Image {
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/poor"
        opacity: 0
    }

    Repeater {
        model: 10
        delegate: Item {
            Image {
                source: root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat_" + modelData
                opacity: 0
            }
            Image {
                source: root.iniImagesUrl + "judge/" + judgement.judge + "/great_" + modelData
                opacity: 0
            }
        }
    }
}
