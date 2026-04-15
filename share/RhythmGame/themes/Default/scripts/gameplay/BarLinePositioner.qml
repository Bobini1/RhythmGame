import QtQuick

Item {
    id: column

    required property real heightMultiplier
    required property real position
    property alias model: barlinesRepeater.model

    FrameAnimation {
        running: true
        onTriggered: {
            let top = column.height / column.heightMultiplier;
            barlinesRepeater.model.topPosition = column.position + top;
            barlinesRepeater.model.bottomPosition = column.position;
        }
    }
    clip: true

    Item {
        anchors.fill: parent
        anchors.bottomMargin: -column.position * column.heightMultiplier

        Repeater {
            id: barlinesRepeater
            delegate: Item {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: display.time.position * column.heightMultiplier
                anchors.left: parent.left
                anchors.right: parent.right

                Canvas {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 2
                    y: -1
                    antialiasing: true

                    onPaint: {
                        let ctx = getContext("2d");
                        ctx.strokeStyle = "gray";
                        ctx.lineWidth = 1;
                        ctx.beginPath();
                        ctx.moveTo(0, 1);      // horizontal line through the canvas centre
                        ctx.lineTo(width, 1);
                        ctx.stroke();
                    }
                }
            }
        }
    }
}
