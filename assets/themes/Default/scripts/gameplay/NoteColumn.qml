import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import QtQuick.Controls


Item {
    id: column

    required property var columnState
    required property real position
    required property string color
    required property string noteImage
    required property string mineImage
    required property var notes
    required property real heightMultiplier
    required property real noteHeight

    onPositionChanged: {
        let top = column.height / column.heightMultiplier;
        columnState.topPosition = column.position + top;
        columnState.bottomPosition = column.position;
    }

    Flickable {
        id: flickable
        anchors.fill: parent

        contentWidth: column.width
        clip: true
        interactive: false

        contentY: -(column.position * column.heightMultiplier + height * (1 - playArea.generalVars.liftOn * playArea.generalVars.liftRatio)) + column.noteHeight / 3

        Repeater {
            id: repeater
            model: column.columnState

            delegate: Item {
                id: noteObj

                required property var display
                required property int index
                readonly property var hitData: display.hitData
                readonly property bool held: note.type === Note.Type.LongNoteBegin && hitData && !display.otherEndHitData
                readonly property var note: display.note
                readonly property real nextPosition: column.notes[realIndex+1]?.time?.position || Infinity
                visible: note.type === Note.Type.LongNoteBegin || note.type === Note.Type.LongNoteEnd || !hitData
                readonly property bool shouldShowStatic: note.type === Note.Type.LongNoteBegin && (wasHeld || note.time.position < column.position) && nextPosition > column.position
                property bool wasHeld: note.type === Note.Type.LongNoteBegin && hitData && (!display.otherEndHitData || display.otherEndHitData.points.judgement !== Judgement.LnEndSkip)
                property int realIndex: columnState.getRealIndex(index)

                y: (shouldShowStatic ? -column.position : -note.time.position) * column.heightMultiplier -column.noteHeight / 3
                width: column.width
                z: index

                Image {
                    id: noteImg
                    height: column.noteHeight
                    width: column.width

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
                }

                Loader {
                    id: lnBodyLoader

                    active: noteObj.note.type === Note.Type.LongNoteBegin

                    sourceComponent: Component {
                        Image {
                            id: lnImg

                            fillMode: Image.TileVertically
                            height: {
                                if (!noteObj.shouldShowStatic) {
                                    return (noteObj.nextPosition - noteObj.note.time.position) * column.heightMultiplier - column.noteHeight
                                } else {
                                    return (noteObj.nextPosition - column.position) * column.heightMultiplier - column.noteHeight
                                }
                            }
                            source: {
                                if (!noteObj.held) {
                                    return root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_inactive_" + column.color;
                                }
                                let flashing = Math.abs(column.position % 0.5) > 0.25;
                                return root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_" + (flashing ? "flash" : "active") + "_" + column.color;
                            }
                            width: noteObj.width
                            y: -height
                            z: -1
                        }
                    }
                }
            }
        }
    }
}
