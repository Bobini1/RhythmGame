import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: column

    property string color
    property real heightMultiplier: 20
    property real noteHeight: 36
    property var notes
    property bool notesStay
    property string noteImage
    property string mineImage
    required property var columnState
    Layout.alignment: Qt.AlignBottom

    Repeater {
        id: noteRepeater

        model: column.columnState

        Image {
            id: noteImg

            // for ln begin only
            required property var display
            required property int index
            readonly property var hitData: display.hitData
            readonly property bool held: note.type === Note.Type.LongNoteBegin && hitData && !nextHitData
            readonly property var note: display.note
            readonly property var noteVisible: !display.belowJudgeline
            height: column.noteHeight
            visible: (noteVisible && !hitData) || (note.type === Note.Type.LongNoteBegin && nextVisible) || (note.type === Note.Type.LongNoteEnd && noteVisible)

            onNoteVisibleChanged: {
                if (!noteVisible && note.type === Note.Type.LongNoteBegin && nextVisible) {
                    state = "reparented";
                }
            }

            // repeater instantiates items one by one so we set those properties in onCompleted of the next note
            property var nextHitData: null
            property real nextPosition: 0
            property var nextVisible: false

            Component.onCompleted: {
                if (note.type === Note.Type.LongNoteEnd) {
                    noteRepeater.itemAt(index-1).nextHitData = Qt.binding(() => hitData);
                    noteRepeater.itemAt(index-1).nextPosition = note.time.position;
                    noteRepeater.itemAt(index-1).nextVisible = Qt.binding(() => noteVisible);
                }
            }

            function getTypeString() {
                let type = noteImg.note.type;
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

            states: State {
                name: "reparented"
                ParentChange {
                    target: noteImg; parent: noteAnchor;
                }
                PropertyChanges {
                    target: noteImg; y: -noteImg.height / 2;
                }
            }

            mipmap: true
            source: getTypeString() === "invisible_" ? "" : root.iniImagesUrl + (noteImg.note.type === Note.Type.Landmine ? ("mine/" + column.mineImage) : ("notes/" + column.noteImage)) + "/" + getTypeString() + column.color
            y: -note.time.position * column.heightMultiplier - height / 2

            Loader {
                id: lnBodyLoader

                active: noteImg.note.type === Note.Type.LongNoteBegin

                sourceComponent: Component {
                    Image {
                        id: lnImg

                        fillMode: Image.TileVertically
                        height: {
                            let pos;
                            if (noteImg.state !== "reparented") {
                                pos = noteImg.note.time.position;
                            } else {
                                pos = chart.position;
                            }
                            return (noteImg.nextPosition - pos) * column.heightMultiplier - noteImg.height;
                        }
                        source: {
                            if (!noteImg.held) {
                                return root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_inactive_" + column.color;
                            }
                            let flashing = Math.abs(chart.position % 0.5) > 0.25;
                            return root.iniImagesUrl + "notes/" + column.noteImage + "/ln_body_" + (flashing ? "flash" : "active") + "_" + column.color;
                        }
                        width: noteImg.width
                        y: -height
                    }
                }
            }
        }
    }
}
