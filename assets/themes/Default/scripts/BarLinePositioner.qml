import QtQuick 2.0

Item {
    id: column

    property var barLines: []
    property int heightMultiplier: 20

    // position barLines according to their timestamp
    Component.onCompleted: {
        if (barLines.length === 0) {
            return;
        }
        column.height = barLines[barLines.length - 1].position + heightMultiplier;
        for (let i = 0; i < barLineRepeater.count; i++) {
            let note = barLineRepeater.itemAt(i);
            let notePosition = barLines[i].position * heightMultiplier;
            // we need to start from the bottom
            note.y = column.height - notePosition;
        }
    }

    // create rectangle for each note
    Repeater {
        id: barLineRepeater

        model: column.barLines

        Rectangle {
            id: note

            border.width: 1
            color: "black"
            height: 5
            width: column.width
        }
    }
}
