import QtQuick 2.0

Item {
    id: column

    property var barLines: []
    property int heightMultiplier: 20

    anchors.bottom: parent.bottom

    Repeater {
        id: barLineRepeater

        model: column.barLines

        Rectangle {
            id: note

            border.width: 1
            color: "darkslategray"
            height: 5
            width: column.width
            y: -column.barLines[index].position * column.heightMultiplier - height / 2
        }
    }
}
