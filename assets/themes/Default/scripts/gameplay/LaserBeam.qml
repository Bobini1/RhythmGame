import QtQml
import QtQuick
import QtQuick.Layouts

Item {
    id: wrapper

    property int columnIndex
    property url image
    required property list<real> columnSizes
    function start() {
        laserBeamAnimation.stop();
        laserBeam.width = wrapper.width;
        laserBeam.height = wrapper.height;
    }
    function stop() {
        laserBeam.duration = (laserBeam.width / wrapper.width) * 100;
        laserBeamAnimation.start();
    }

    anchors.bottom: parent.bottom
    height: 576
    width: wrapper.columnSizes[wrapper.columnIndex]

    Image {
        id: laserBeam

        property int duration: 100

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        height: 0
        source: wrapper.image
        width: 0

        SequentialAnimation {
            id: laserBeamAnimation

            ParallelAnimation {
                NumberAnimation {
                    duration: laserBeam.duration
                    property: "width"
                    target: laserBeam
                    to: 0
                }
                NumberAnimation {
                    duration: laserBeam.duration
                    property: "height"
                    target: laserBeam
                    to: 0
                }
            }
        }
    }
}
