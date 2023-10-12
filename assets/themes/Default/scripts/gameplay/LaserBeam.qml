import QtQml
import QtQuick
import QtQuick.Layouts

Item {
    id: wrapper

    property int columnIndex
    property url image

    function start() {
        laserBeamAnimation.complete();
        laserBeamAnimation.start();
    }

    anchors.bottom: parent.bottom
    height: 576
    width: root.columnSizes[wrapper.columnIndex]

    Image {
        id: laserBeam

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        height: 0
        source: wrapper.image
        width: 0

        SequentialAnimation {
            id: laserBeamAnimation

            PropertyAction {
                property: "width"
                target: laserBeam
                value: wrapper.width
            }
            PropertyAction {
                property: "height"
                target: laserBeam
                value: wrapper.height
            }
            PauseAnimation {
                duration: 100
            }
            ParallelAnimation {
                NumberAnimation {
                    duration: 100
                    from: wrapper.width
                    property: "width"
                    target: laserBeam
                    to: 0
                }
                NumberAnimation {
                    duration: 100
                    from: wrapper.height
                    property: "height"
                    target: laserBeam
                    to: 0
                }
            }
        }
    }
}
