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
    property int spacing

    function removeNote(column: int, index: int) {
        var noteColumn = noteColumnRepeater.itemAt(columnsReversedMapping[column]);
        noteColumn.removeNoteAt(index);
    }

    color: "black"
    width: totalWidthAbs

    BarLinePositioner {
        anchors.bottom: parent.bottom
        barLines: chart.chartData.noteData.barLines
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
                    return chart.chartData.noteData.visibleNotes[column];
                })

            NoteColumn {
                id: noteColumn

                heightMultiplier: root.greenNumber
                image: root.noteImages[playfield.columns[index]]
                noteHeight: 36
                notes: modelData
                width: root.columnSizes[playfield.columns[index]]
            }
        }
    }
}
