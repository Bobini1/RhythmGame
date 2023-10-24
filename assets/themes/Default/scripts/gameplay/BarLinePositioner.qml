import QtQuick 2.0

Item {
    id: column

    property var barLines: []
    property int heightMultiplier: 20

    Repeater {
        id: barLineRepeater

        model: column.barLines

        Rectangle {
            id: note

            border.width: 1
            color: "gray"
            height: 3
            width: parent.width
            y: -column.barLines[index].position * column.heightMultiplier - height / 2
        }
    }
}
