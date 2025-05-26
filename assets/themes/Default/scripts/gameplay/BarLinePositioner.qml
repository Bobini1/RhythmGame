import QtQuick 2.0

Flickable {
    id: column

    required property real heightMultiplier
    required property real position
    property alias model: barlinesRepeater.model
    interactive: false
    clip: true
    contentY: -(column.position * column.heightMultiplier + height * (1 - playArea.globalVars.liftOn * playArea.globalVars.liftRatio)) + 0.5
    contentWidth: width

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
