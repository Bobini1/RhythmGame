import QtQuick 2.0
import QtQuick.Layouts

Item {
    id: column

    property int heightMultiplier: 20
    property var notes: []

    Layout.alignment: Qt.AlignBottom
    Layout.bottomMargin: 8

    // position notes according to their timestamp
    Component.onCompleted: {
        if (notes.length === 0) {
            return;
        }
        column.height = notes[notes.length - 1].time.position + heightMultiplier;
        for (let i = 0; i < noteRepeater.count; i++) {
            let note = noteRepeater.itemAt(i);
            let notePosition = notes[i].time.position * heightMultiplier;
            // we need to start from the bottom
            note.y = column.height - notePosition;
        }
    }

    // create rectangle for each note
    Repeater {
        id: noteRepeater

        model: column.notes

        Rectangle {
            id: note

            border.width: 1
            color: "red"
            height: 20
            width: parent.width
        }
    }
}
