import QtQuick

Item {
    id: judgement

    height: childrenRect.height
    width: childrenRect.width

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
            Qt.callLater(function () {
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
                if (source != root.iniImagesUrl + "judge.png/pgreat") {
                    judgementAnimationFlashing.start();
                } else {
                    judgementAnimationFlashing.stop();
                    judgementRow.visible = true;
                }
            }
        }
        Repeater {
            id: comboNumber

            model: chart.score.combo > 1 && judgementAnimation.source != root.iniImagesUrl + "judge.png/poor" ? chart.score.combo.toString().split("") : []

            AnimatedSprite {
                id: comboNumberAnimation

                currentFrame: judgementAnimation.currentFrame
                frameCount: judgementAnimation.source == root.iniImagesUrl + "judge.png/pgreat" ? 3 : 1
                frameHeight: 84
                frameWidth: 55
                paused: true
                source: root.iniImagesUrl + (judgementAnimation.source == root.iniImagesUrl + "judge.png/pgreat" ? "judge.png/pgreat_" : "judge.png/great_") + modelData
            }
        }
    }
    Connections {
        function onHit(tap) {
            if (tap.noteIndex === -1) {
                return;
            }
            Qt.callLater(function () {
                    switch (tap.points.judgement) {
                    case "Perfect":
                        judgementAnimation.frameCount = 3;
                        judgementAnimation.source = root.iniImagesUrl + "judge.png/pgreat";
                        break;
                    case "Great":
                        judgementAnimation.frameCount = 1;
                        judgementAnimation.source = root.iniImagesUrl + "judge.png/great";
                        break;
                    case "Good":
                        judgementAnimation.frameCount = 1;
                        judgementAnimation.source = root.iniImagesUrl + "judge.png/good";
                        break;
                    case "Bad":
                        judgementAnimation.frameCount = 1;
                        judgementAnimation.source = root.iniImagesUrl + "judge.png/bad";
                        break;
                    default:
                        judgementAnimation.frameCount = 1;
                        judgementAnimation.source = root.iniImagesUrl + "judge.png/poor";
                        break;
                    }
                });
        }
        function onMissed(misses) {
            Qt.callLater(function () {
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge.png/poor";
                });
        }

        target: chart.score
    }
}
