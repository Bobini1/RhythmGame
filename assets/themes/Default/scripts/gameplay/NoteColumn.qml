import QtQuick 2.0
import QtQuick.Layouts

Item {
    id: column

    property int heightMultiplier: 20
    property string image
    property int noteHeight: 36
    property var notes

    function removeNote(index: int) {
        noteRepeater.itemAt(index).visible = false;
    }

    Layout.alignment: Qt.AlignBottom

    Repeater {
        id: noteRepeater

        model: column.notes

        Image {
            id: noteImg

            height: column.noteHeight
            source: column.image
            width: parent.width
            y: Math.floor(-column.notes[index].time.position * column.heightMultiplier) - height / 2
        }
    }
    Connections {
        function onPositionChanged(_) {
            if (column.notes.length === 0) {
                return;
            }
            let index = 0;
            while (index < column.notes.length) {
                let note = column.notes[index];
                let noteImage = noteRepeater.itemAt(index);
                if (note.time.position > chart.position) {
                    return;
                }
                noteImage.visible = false;
                index++;
            }
        }

        target: chart
    }
}
