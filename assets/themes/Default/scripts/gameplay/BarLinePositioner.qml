import QtQuick 2.0

Item {
    id: column

    property alias barLines: barLineRepeater.model
    required property real heightMultiplier

    Repeater {
        id: barLineRepeater

        Rectangle {
            visible: !display.belowJudgeline
            color: "gray"
            height: 1
            width: parent.width
            y: -display.time.position * column.heightMultiplier - height / 2
        }
    }
}
