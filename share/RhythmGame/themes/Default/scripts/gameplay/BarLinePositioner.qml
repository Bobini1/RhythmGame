import QtQuick

Flickable {
    id: column

    required property real heightMultiplier
    required property real position
    property alias model: barlinesRepeater.model

    FrameAnimation {
        running: true
        onTriggered: {
            // Update the position of the bar lines based on the current position
            barlinesRepeater.model.bottomPosition = column.position;
            let top = column.height / column.heightMultiplier;
            barlinesRepeater.model.topPosition = column.position + top;
        }
    }

    Repeater {
        id: barlinesRepeater
        delegate: Item {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: display.time.position * column.heightMultiplier + column.height
            anchors.left: parent.left
            anchors.right: parent.right
            Component.onCompleted: {
                console.info("Bar line at position:", display.time.position, x, y, width, height);
            }
            Rectangle {
                color: "gray"
                height: 1
                y: -height / 2
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }
    }
}
