import QtQml
import QtQuick
import QtQuick.Layouts

Item {
    id: wrapper

    property int columnIndex
    property url image
    property bool active: false
    property bool hideAfterFinished: false
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

    Image {
        id: laserBeam

        property int duration: 83

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        height: 0
        source: wrapper.image
        width: 0

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
                property: "height"
                value: 0
            }
            SequentialAnimation {
                NumberAnimation {
                    duration: laserBeam.duration * 3 / 5
                    property: "height"
                    target: laserBeam
                    to: laserBeam.implicitHeight
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
