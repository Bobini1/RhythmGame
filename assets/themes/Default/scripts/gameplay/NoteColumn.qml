import QtQuick 2.0
import QtQuick.Layouts

Item {
    id: column

    property int erasedNoteIndex: 0
    property int heightMultiplier: 20
    property string image
    property int noteHeight: 36
    property var notes

    function removeNote(index: int) {
        noteRepeater.itemAt(index - column.erasedNoteIndex).visible = false;
    }

    Layout.alignment: Qt.AlignBottom

    ListModel {
        id: notesModel

        Component.onCompleted: {
            for (let i = 0; i < column.notes.length; i++) {
                notesModel.append({
                        "note": i
                    });
            }
        }
    }
    Repeater {
        id: noteRepeater

        model: notesModel

        Image {
            id: noteImg

            height: column.noteHeight
            source: column.image
            width: parent.width
            y: Math.floor(-column.notes[note].time.position * column.heightMultiplier) - height / 2
        }
    }
    Connections {
        function onPositionChanged(_) {
            let count = 0;
            while (column.erasedNoteIndex < column.notes.length) {
                let note = column.notes[column.erasedNoteIndex];
                if (note.time.position > chart.position) {
                    break;
                }
                count++;
                column.erasedNoteIndex++;
            }
            if (count > 0) {
                notesModel.remove(0, count);
            }
        }

        target: chart
    }
}
