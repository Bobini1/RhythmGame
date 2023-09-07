import QtQuick 2.0
import QtQuick.Layouts

Item {
    id: column

    property int heightMultiplier: 20
    property string image
    property int noteHeight: 36
    property var notes: []

    function removeNote(index: int) {
        noteRepeater.itemAt(index).visible = false;
    }

    Layout.alignment: Qt.AlignBottom
    Layout.bottomMargin: 16

    // position notes according to their timestamp
    Component.onCompleted: {
        if (notes.length === 0) {
            return;
        }
        column.height = notes[notes.length - 1].time.position * heightMultiplier;
        for (let i = 0; i < noteRepeater.count; i++) {
            let note = noteRepeater.itemAt(i);
            let notePosition = notes[i].time.position * heightMultiplier;
            // we need to start from the bottom
            note.y = Math.floor(column.height - notePosition);
        }
    }

    // create rectangle for each note
    Repeater {
        id: noteRepeater

        model: column.notes

        Image {
            id: note

            height: column.noteHeight
            source: column.image
            width: parent.width
        }
    }
}
