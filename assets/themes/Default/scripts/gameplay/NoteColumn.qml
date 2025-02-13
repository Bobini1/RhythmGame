import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls

ListView {
    id: column

    property string color
    property real heightMultiplier: 20
    property real noteHeight: 36
    property var notes
    property bool notesStay
    property string noteImage
    property string mineImage
    required property var columnState
    interactive: false
    spacing: 0

    Repeater {
        id: repeater
        model: column.columnState

        delegate: Item {
            required property var display
        }
    }

    verticalLayoutDirection: ListView.BottomToTop

    contentY: -(chart.position * playArea.heightMultiplier + height * (1 - playArea.globalVars.liftOn * playArea.globalVars.liftRatio)) + noteHeight / 3
    model: columnState
    reuseItems: true

    delegate: Item {
        id: noteObj

        required property var display
        required property int index
        readonly property var hitData: display.hitData
        readonly property bool held: note.type === Note.Type.LongNoteEnd && !hitData && repeater.itemAt(index-1).display.hitData
        readonly property var note: display.note
        readonly property var noteVisible: !display.belowJudgeline
        visible: note.type === Note.Type.LongNoteEnd || !hitData

        readonly property real previousPos: index > 0 ? notes[index-1].time.position : 0
        height: (note.time.position - previousPos) * column.heightMultiplier
        width: column.width
        z: index

        Image {
            id: noteImg
            height: column.noteHeight
            width: column.width
            mipmap: true

            function getTypeString() {
                let type = noteObj.note.type;
                switch (type) {
                    case Note.Type.Normal:
                        return "note_";
                    case Note.Type.LongNoteBegin:
                        return "ln_start_";
                    case Note.Type.LongNoteEnd:
                        return "ln_end_";
                    case Note.Type.Landmine:
                        return "mine_";
                    case Note.Type.Invisible:
                        return "invisible_";
                    default:
                        console.info("Unknown note type: " + type);
                }
            }
            source: {
                let type = getTypeString();
                if (type === "invisible_") {
                    return "";
                }

                const typePath = noteObj.note.type === Note.Type.Landmine
                    ? `mine/${column.mineImage}`
                    : `notes/${column.noteImage}`;
                const suffix = `${type}${column.color}`;

                return `${root.iniImagesUrl}${typePath}/${suffix}`;
            }
            y: -column.noteHeight / 3
        }

        Loader {
            id: lnBodyLoader

            active: noteObj.note.type === Note.Type.LongNoteEnd

            sourceComponent: Component {
                Image {
                    id: lnImg

                    fillMode: Image.TileVertically
                    height: noteObj.height - column.noteHeight
                    source: {
                        if (!noteObj.held) {
                            return root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_inactive_" + column.color;
                        }
                        let flashing = Math.abs(chart.position % 0.5) > 0.25;
                        return root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_" + (flashing ? "flash" : "active") + "_" + column.color;
                    }
                    width: noteObj.width
                    y: column.noteHeight * 2 / 3
                    z: -1
                }
            }
        }
    }
}
