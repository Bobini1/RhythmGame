import QtQml
import QtQuick
import QtQuick.Layouts

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
    property double judgeLineGlobalPos
    property int spacing

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
    width: totalWidthAbs

    BarLinePositioner {
        anchors.bottom: parent.bottom
        barLines: chart.notes.barLines
        heightMultiplier: root.greenNumber
        width: parent.width
    }
    RowLayout {
        id: notesRow

        anchors.bottom: parent.bottom
        height: children.height
        spacing: playfield.spacing
        width: parent.width

        Repeater {
            id: noteColumnRepeater

            // take only the columns specified in the columns property
            model: playfield.columns.map(function (column) {
                    return chart.notes.visibleNotes[column];
                })

            NoteColumn {
                id: noteColumn

                color: root.noteColors[playfield.columns[index]]
                heightMultiplier: root.greenNumber
                judgeLineGlobalPos: playfield.judgeLineGlobalPos
                noteHeight: 36
                notes: modelData
                width: root.columnSizes[playfield.columns[index]]
            }
        }
    }
}
