import QtQml
import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Rectangle {
    id: playfield

    property list<int> columns
    readonly property list<int> columnsReversedMapping: {
        var mapping = [];
        for (var i = 0; i < columns.length; i++) {
            mapping[columns[i]] = i;
        }
        return mapping;
    }
    property real spacing

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
            model: playfield.columns.map(function (column) {
                    return root.visibleNotes[column];
                })

            NoteColumn {
                id: noteColumn

                color: root.noteColors[playfield.columns[index]]
                heightMultiplier: root.greenNumber
                noteHeight: ProfileList.currentProfile.vars.themeVars.gameplay.thickness
                notes: modelData
                width: root.columnSizes[playfield.columns[index]]
                height: 1
            }
        }
    }
}
