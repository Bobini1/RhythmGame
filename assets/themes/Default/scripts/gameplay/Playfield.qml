import QtQml
import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: playfield

    required property var columnSizes
    property list<int> columns
    readonly property list<int> columnsReversedMapping: {
        let mapping = [];
        for (let i = 0; i < columns.length; i++) {
            mapping[columns[i]] = i;
        }
        return mapping;
    }
    required property real heightMultiplier
    required property real noteThickness
    required property real spacing
    required property var notes
    required property var columnStates
    required property string noteImage
    required property string mineImage
    required property bool notesStay
    required property real position

    width: notesRow.width

    Row {
        id: notesRow

        spacing: playfield.spacing
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        Repeater {
            id: noteColumnRepeater

            // take only the columns specified in the columns property
            model: playfield.notes
            height: notesRow.height

            delegate: NoteColumn {
                id: noteColumn

                required property int index
                required property var modelData

                color: {
                    let idx = playfield.columns[index];
                    if (idx === 7 || idx === 15)
                        return "red";
                    else if (idx % 2 === 0)
                        return "white";
                    else
                        return "black";
                }
                height: notesRow.height
                heightMultiplier: playfield.heightMultiplier
                noteHeight: playfield.noteThickness * 3
                notes: modelData
                columnState: playfield.columnStates[index]
                noteImage: playfield.noteImage
                mineImage: playfield.mineImage
                notesStay: playfield.notesStay
                width: playfield.columnSizes[playfield.columns[index]]
                position: playfield.position
            }
        }
    }
}
