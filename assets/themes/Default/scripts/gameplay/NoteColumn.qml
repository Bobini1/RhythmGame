import QtQuick 2.0
import QtQuick.Layouts

Item {
    id: column

    property int erasedNoteIndex: 0
    property int heightMultiplier: 20
    property string image
    property int noteHeight: 36
    property var notes
    property int visibleNoteIndex: 0

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
            visible: false
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
            column.visibleNoteIndex -= count;
            column.visibleNoteIndex = Math.max(0, column.visibleNoteIndex);

            if (count > 0) {
                notesModel.remove(0, count);
            }
            let visibleNoteIndex = column.visibleNoteIndex;
            count = 0;
            while (visibleNoteIndex + count < noteRepeater.count) {
                let noteImage = noteRepeater.itemAt(visibleNoteIndex + count);
                // console.info(column.parent.parent.parent)
                // show note if it is visible
                let globalPos = noteImage.mapToItem(column.parent.parent.parent, 0, 0)
                globalPos.y += noteImage.height
                if (globalPos.y > 0) {
                    noteImage.visible = true;
                    count++;
                } else {
                    break;
                }
            }
            column.visibleNoteIndex += count;
        }

        target: chart
    }
}
