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
            let erasedNoteIndex = column.erasedNoteIndex;
            let chartPosition = chart.position;
            while (erasedNoteIndex + count < column.notes.length) {
                let note = column.notes[erasedNoteIndex + count];
                if (note.time.position > chartPosition) {
                    break;
                }
                count++;
            }
            column.erasedNoteIndex += count;
            if (count > 0) {
                notesModel.remove(0, count);
            }
        }

        target: chart
    }
}
