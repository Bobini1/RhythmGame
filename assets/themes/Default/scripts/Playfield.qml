import QtQml
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: playfield

    property list<int> columns: []
    readonly property int totalWidthAbs: 3 * 48 + 4 * 60 + 108 + notesRow.spacing * 6

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

        property double blackWidth: 48.0 / playfield.totalWidthAbs * parent.width
        property double redWidth: 108.0 / playfield.totalWidthAbs * parent.width
        property double whiteWidth: 60.0 / playfield.totalWidthAbs * parent.width

        anchors.bottom: parent.bottom
        height: children.height
        spacing: 2
        width: parent.width

        Repeater {
            // take only the columns specified in the columns property
            model: playfield.columns.map(function (column) {
                    return chart.chartData.noteData.visibleNotes[column];
                })

            NoteColumn {
                id: noteColumn

                heightMultiplier: root.greenNumber
                image: {
                    let column = playfield.columns[index];
                    if (column === 7 || column === 15)
                        return root.imagesUrl + "default.png/note_red";
                    else if (column % 2 === 0)
                        return root.imagesUrl + "default.png/note_white";
                    else
                        return root.imagesUrl + "default.png/note_black";
                }
                noteHeight: 36
                notes: modelData
                width: {
                    let column = playfield.columns[index];
                    if (column === 7 || column === 15)
                        return notesRow.redWidth;
                    else if (column % 2 === 0)
                        return notesRow.whiteWidth;
                    else
                        return notesRow.blackWidth;
                }

                Component.onCompleted:
                //console.log(note);
                {
                }
            }
        }
    }
}
