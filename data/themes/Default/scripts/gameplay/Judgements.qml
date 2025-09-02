import QtQuick
import RhythmGameQml

Item {
    id: judgement

    height: childrenRect.height
    width: childrenRect.width
    z: 3
    visible: false

    required property var score
    required property string judge
    required property string fastslow
    required property var columns

    // turn invisible after one second of no notes
    Timer {
        id: hidingTimer

        interval: 500
        onTriggered: {
            judgement.visible = false;
        }
    }

    Image {
        id: fastslow
        anchors.bottom: judgementRow.top
        anchors.horizontalCenter: judgementRow.horizontalCenter
        anchors.bottomMargin: 10
        asynchronous: true
        property bool fast
        source: root.iniImagesUrl + "fastslow/" + judgement.fastslow + (fast ? "/fast" : "/slow")
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

        AnimatedSprite {
            id: judgementAnimation

            frameDuration: 40
            frameHeight: 84
            frameWidth: 227
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
            let fast = tap.points.deviation < 0;
            fastslow.fast = fast;
            switch (tap.points.judgement) {
                case Judgement.Perfect:
                    judgementAnimation.frameCount = 3;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat";
                    fastslow.visible = false;
                    break;
                case Judgement.Great:
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/great";
                    fastslow.visible = true;
                    break;
                case Judgement.Good:
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/good";
                    fastslow.visible = true;
                    break;
                case Judgement.Bad:
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/bad";
                    fastslow.visible = true;
                    break;
                case Judgement.Poor:
                    judgementAnimation.frameCount = 1;
                    judgementAnimation.source = root.iniImagesUrl + "judge/" + judgement.judge + "/poor";
                    fastslow.visible = false;
                    break;
                default:
                    fastslow.visible = false;
            }
        }

        target: judgement.score
    }
    // preload images

    Image {
        width: 0
        height: 0
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat"
        opacity: 0
    }

    Image {
        width: 0
        height: 0
        source: root.iniImagesUrl + "judge/" + judgement.judge + "/pgreat_0"
        opacity: 0
    }
}
