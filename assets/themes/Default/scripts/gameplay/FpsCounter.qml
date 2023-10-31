import QtQuick 2.0
import QtQuick.Window 2.2

Rectangle {
    id: root

    property int counter: 0
    readonly property real dp: Screen.pixelDensity * 25.4 / 160
    property int fps: 0
    property int fpsAvg: 0
    property int frameCounter: 0
    property int frameCounterAvg: 0

    color: "black"
    height: (36 + 10) * dp
    width: childrenRect.width + 10 * dp

    Image {
        id: spinnerImage

        anchors.verticalCenter: parent.verticalCenter
        height: 36 * dp
        width: 36 * dp
        x: 4 * dp

        NumberAnimation on rotation {
            duration: 800
            loops: Animation.Infinite
            to: 360
        }

        onRotationChanged: frameCounter++
    }
    Text {
        anchors.left: spinnerImage.right
        anchors.leftMargin: 8 * dp
        anchors.verticalCenter: spinnerImage.verticalCenter
        color: "#c0c0c0"
        font.pixelSize: 18 * dp
        text: "Ø " + root.fpsAvg + " | " + root.fps + " fps"
    }
    Timer {
        interval: 2000
        repeat: true
        running: true

        onTriggered: {
            frameCounterAvg += frameCounter;
            root.fps = frameCounter / 2;
            counter++;
            frameCounter = 0;
            if (counter >= 3) {
                root.fpsAvg = frameCounterAvg / (2 * counter);
                frameCounterAvg = 0;
                counter = 0;
            }
        }
    }
}