import RhythmGameQml
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: bg

    color: "white"
    radius: 32

    scale: 0

    width: 1600
    height: 900

    Component {
        id: delegateComponent

        Text {
            text: modelData
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 32

            color: {
                let c = Qt.color("black");
                let value = Math.max(0, 1 - Math.abs(Tumbler.displacement));
                c.hslLightness = value;
                return c;
            }
        }
    }


    TumblerFrame {
        width: 360
        height: 220

        DarkHighlightLabel {
            width: parent.width
            height: 40

            anchors.centerIn: parent
        }
        Tumbler {
            id: randoms
            model: ["NORMAL", "MIRROR", "SPIRAL", "RANDOM", "S-RANDOM", "H-RANDOM", "R-RANDOM", "R-RANDOM"]
            delegate: delegateComponent
            wrap: true
            visibleItemCount: 5

            anchors.fill: parent

            function findView(parent) {
                for (let i = 0; i < parent.children.length; ++i) {
                    let child = parent.children[i];
                    if (child.hasOwnProperty("currentIndex")) {
                        return child;
                    }

                    let grandChild = findView(child);
                    if (grandChild)
                        return grandChild;
                }

                return null;
            }

            Component.onCompleted: {
            }

            WheelHandler {
                target: randoms
                onWheel: (wheel) => {
                    let view = randoms.findView(randoms);
                    print(view.highlightMoveDuration);
                    view.highlightMoveDuration = 200;
                    if (wheel.angleDelta.y > 0) {
                        randoms.findView(randoms).incrementCurrentIndex();
                    } else {
                        randoms.findView(randoms).decrementCurrentIndex();
                    }
                }
            }
        }
    }

    states: State {
        name: "shown"; when: InputTranslator.start
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