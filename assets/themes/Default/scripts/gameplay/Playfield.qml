import QtQml
import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Rectangle {
    id: playfield

    required property var columnSizes
    property list<int> columns
    readonly property list<int> columnsReversedMapping: {
        var mapping = [];
        for (var i = 0; i < columns.length; i++) {
            mapping[columns[i]] = i;
        }
        return mapping;
    }
    required property real heightMultiplier
    required property real noteThickness
    required property real spacing
    required property var notes
    required property string noteImage
    required property string mineImage
    required property bool notesStay

    function activateLn(column: int, index: int) {
        let noteColumn = noteColumnRepeater.itemAt(columnsReversedMapping[column]);
        noteColumn.activateLn(index);
    }
    function deactivateLn(column: int, index: int) {
        let noteColumn = noteColumnRepeater.itemAt(columnsReversedMapping[column]);
        noteColumn.deactivateLn(index);
    }
    function markLnEndAsMissed(column: int, index: int) {
        let noteColumn = noteColumnRepeater.itemAt(columnsReversedMapping[column]);
        noteColumn.markLnEndAsMissed(index);
    }
    function removeNote(column: int, index: int) {
        let noteColumn = noteColumnRepeater.itemAt(columnsReversedMapping[column]);
        noteColumn.removeNote(index);
    }

    color: "black"
    width: notesRow.width

    Row {
        id: notesRow

        anchors.bottom: parent.bottom
        spacing: playfield.spacing

        Repeater {
            id: noteColumnRepeater

            // take only the columns specified in the columns property
            model: playfield.notes

            NoteColumn {
                id: noteColumn

                color: {
                    let idx = playfield.columns[index];
                    if (idx === 7 || idx === 15)
                        return "red";
                    else if (idx % 2 === 0)
                        return "white";
                    else
                        return "black";
                }
                height: 1
                heightMultiplier: playfield.heightMultiplier
                noteHeight: playfield.noteThickness * 3
                notes: modelData
                noteImage: playfield.noteImage
                mineImage: playfield.mineImage
                notesStay: playfield.notesStay
                width: playfield.columnSizes[playfield.columns[index]]
            }
        }
    }
}
