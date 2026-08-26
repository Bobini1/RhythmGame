import QtQml
import QtQuick
import QtQuick.Layouts

Item {
    id: wrapper

    property int columnIndex
    property url image
    property bool active: false
    property bool hideAfterFinished: false
    property real beamSize: 1
    onActiveChanged: {
        if (active) {
            hideAnimation.stop();
            appearAnimation.start();
            hideAfterFinished = false;
        } else if (!appearAnimation.running) {
            hideAnimation.start();
        } else {
            hideAfterFinished = true;
        }
    }
    Connections {
        target: appearAnimation
        function onFinished() {
            if (wrapper.hideAfterFinished) {
                hideAnimation.start();
            }
        }
    }

    anchors.bottom: parent.bottom

    Item {
        id: laserBeam

        property int duration: 83
        readonly property real clampedSize: Math.max(0, Math.min(1, wrapper.beamSize))

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        clip: true
        height: beamImage.implicitHeight * clampedSize
        transformOrigin: Item.Bottom
        visible: clampedSize > 0
        width: 0

        Image {
            id: beamImage

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            source: wrapper.image
            width: parent.width
        }

        ParallelAnimation {
            id: appearAnimation
            PropertyAction {
                target: laserBeam
                property: "opacity"
                value: 1
            }
            PropertyAction {
                target: laserBeam
                property: "width"
                value: wrapper.width
            }
            PropertyAction {
                target: laserBeam
                property: "scale"
                value: 0
            }
            SequentialAnimation {
                NumberAnimation {
                    duration: laserBeam.duration * 3 / 5
                    property: "scale"
                    target: laserBeam
                    to: 1
                }
                PauseAnimation {
                    duration: laserBeam.duration * 2 / 5
                }
            }
        }

        ParallelAnimation {
            id: hideAnimation
            NumberAnimation {
                duration: laserBeam.duration
                target: laserBeam
                property: "opacity"
                to: 0
            }
            NumberAnimation {
                duration: laserBeam.duration
                property: "width"
                target: laserBeam
                to: 0
            }
        }
    }
}
