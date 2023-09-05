import QtQml
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: playfield

    property list<int> columns
    property int spacing

    function removeNote(column: int, index: int) {
        notesRow.children[column].removeNoteAt(index);
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
