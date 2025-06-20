import QtQuick

Flickable {
    id: column

    required property real heightMultiplier
    required property real position
    property alias model: barlinesRepeater.model
    interactive: false
    clip: true
    contentY: -(column.position * column.heightMultiplier + height * (1 - playArea.generalVars.liftOn * playArea.generalVars.liftRatio)) + 0.5
    contentWidth: width

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
            y: -display.time.position * column.heightMultiplier
            width: column.width
            Rectangle {
                color: "gray"
                height: 1
                y: -height / 2
                width: parent.width
            }
        }
    }
}
