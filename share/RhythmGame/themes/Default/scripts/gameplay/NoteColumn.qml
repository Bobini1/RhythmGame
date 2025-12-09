pragma ValueTypeBehavior: Addressable
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
    required property bool hideLnEnds
    required property real liftRatio

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
    readonly property url mine: `${root.iniImagesUrl}mine/${column.mineImage}/mine_${column.color}`;

    Item {
        id: flickable
        height: parent.height
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: (column.position * column.heightMultiplier + height * (1 - playArea.generalVars.liftOn * playArea.generalVars.liftRatio)) - column.noteHeight
        }

        Repeater {
            id: repeater
            model: column.columnState

            delegate: Item {
                id: noteObj

                required property var display
                required property int index
                readonly property var hitData: display.hitData
                readonly property bool held: display.note.type === note.Type.LongNoteBegin && hitData && !display.otherEndHitData
                readonly property real nextPosition: column.notes[display.index+1]?.time?.position || Infinity
                visible: display.note.type === note.Type.LongNoteBegin || display.note.type === note.Type.LongNoteEnd || !hitData
                readonly property bool shouldShowStatic: display.note.type === note.Type.LongNoteBegin && (wasHeld || display.belowBottom) && nextPosition > column.position
                property bool wasHeld: display.note.type === note.Type.LongNoteBegin && hitData && (!display.otherEndHitData || display.otherEndHitData.points.judgement !== Judgement.LnEndSkip)

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    bottom: parent.bottom
                    bottomMargin: (shouldShowStatic ? column.position : display.note.time.position) * column.heightMultiplier + flickable.height
                }
                width: column.width
                z: -index

                Image {
                    id: noteImg
                    height: column.noteHeight
                    width: column.width

                    source: {
                        switch (noteObj.display.note.type) {
                            case note.Type.Normal:
                                return column.normalNote;
                            case note.Type.LongNoteBegin:
                                return column.lnBegin;
                            case note.Type.LongNoteEnd:
                                if (column.hideLnEnds) {
                                    return "";
                                }
                                return column.lnEnd;
                            case note.Type.Landmine:
                                return column.mine;
                            case note.Type.Invisible:
                                return "";
                            default:
                                console.info("Unknown note type: " + type);
                        }
                    }
                }

                Loader {
                    id: lnBodyLoader

                    active: noteObj.display.note.type === note.Type.LongNoteBegin

                    sourceComponent: Component {
                        Image {
                            id: lnImg

                            fillMode: Image.TileVertically
                            height: {
                                if (!noteObj.shouldShowStatic) {
                                    return (noteObj.nextPosition - noteObj.display.note.time.position) * column.heightMultiplier - column.noteHeight
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
