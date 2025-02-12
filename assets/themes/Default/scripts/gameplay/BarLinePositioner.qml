import QtQuick 2.0

ListView {
    id: column

    required property real heightMultiplier
    required property var barlinesArray
    interactive: false
    contentY: (chart.position * heightMultiplier + height * (1 - playArea.globalVars.liftOn * playArea.globalVars.liftRatio))

    transform: Scale {
        origin.y: column.height / 2
        yScale: -1
    }
    delegate: Item {
        readonly property real previousPos: index > 0 ? barlinesArray[index-1].position : 0
        height: (display.time.position - previousPos) * column.heightMultiplier
        width: column.width
        Rectangle {
            color: "gray"
            height: 1
            y: -height / 2
            width: parent.width
        }
    }
}
