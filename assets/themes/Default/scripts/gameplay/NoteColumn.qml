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

    readonly property url lnBodyInactive: root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_inactive_" + column.color;
    readonly property url lnBodyActive: root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_active_" + column.color;
    readonly property url lnBodyFlashing: root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_flash_" + column.color;
    readonly property bool flashing: Math.abs(column.position % 0.5) > 0.25;

    readonly property url normalNote: `${root.iniImagesUrl}notes/${column.noteImage}/note_${column.color}`;
    readonly property url lnBegin: `${root.iniImagesUrl}notes/${column.noteImage}/ln_start_${column.color}`;
    readonly property url lnEnd: `${root.iniImagesUrl}notes/${column.noteImage}/ln_end_${column.color}`;
    readonly property url mine: `${root.iniImagesUrl}mines/${column.noteImage}/mine_${column.color}`;

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

                    source: {
                        switch (noteObj.note.type) {
                            case Note.Type.Normal:
                                return column.normalNote;
                            case Note.Type.LongNoteBegin:
                                return column.lnBegin;
                            case Note.Type.LongNoteEnd:
                                return column.lnEnd;
                            case Note.Type.Landmine:
                                return column.mine;
                            case Note.Type.Invisible:
                                return "";
                            default:
                                console.info("Unknown note type: " + type);
                        }
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
                                    return column.lnBodyInactive;
                                }
                                return column.flashing ? column.lnBodyFlashing : column.lnBodyActive;
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
