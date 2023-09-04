import QtQml
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: playfield

    property list<int> columns: []

    color: "black"

    BarLinePositioner {
        anchors.bottom: parent.bottom
        barLines: chart.chartData.noteData.barLines
        heightMultiplier: root.greenNumber
        width: parent.width
    }
    RowLayout {
        id: notesRow

        property double columnWidth: (parent.width - notesRow.spacing * (playfield.columns.length - 1)) / playfield.columns.length

        anchors.bottom: parent.bottom
        height: children.height
        spacing: 20
        width: parent.width

        Repeater {
            // take only the columns specified in the columns property
            model: playfield.columns.map(function (column) {
                    return chart.chartData.noteData.visibleNotes[column];
                })

            NoteColumn {
                heightMultiplier: root.greenNumber
                notes: modelData
                width: notesRow.columnWidth
            }
        }
    }
}
