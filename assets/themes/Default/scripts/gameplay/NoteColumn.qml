import QtQuick 2.0
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: column

    property string color
    property int erasedNoteIndex: 0
    property int heightMultiplier: 20
    property int noteHeight: 36
    property var notes
    property int visibleNoteIndex: 0

    function removeNote(index: int) {
        noteRepeater.itemAt(index - column.erasedNoteIndex).visible = false;
    }

    Layout.alignment: Qt.AlignBottom

    ListModel {
        id: notesModel

        Component.onCompleted: {
            for (let i = 0; i < column.notes.length; i++) {
                notesModel.append({
                        "note": i
                    });
            }
        }
    }
    Repeater {
        id: noteRepeater

        model: notesModel

        Image {
            id: noteImg

            // for ln begin only
            property bool held: false

            function getTypeString() {
                let type = column.notes[note].type;
                switch (type) {
                case Note.Type.Normal:
                    return "note_";
                case Note.Type.LongNoteBegin:
                    return "ln_start_";
                case Note.Type.LongNoteEnd:
                    return "ln_end_";
                case Note.Type.Landmine:
                    return "note_";
                default:
                    console.info("Unknown note type: " + type);
                }
            }

            height: column.noteHeight
            source: root.iniImagesUrl + "default.png/" + getTypeString() + column.color
            visible: false
            width: parent.width
            y: Math.floor(-column.notes[note].time.position * column.heightMultiplier) - height / 2

            Loader {
                id: lnBodyLoader

                active: column.notes[note].type === Note.Type.LongNoteBegin

                sourceComponent: Component {
                    Image {
                        height: Math.floor((column.notes[note + 1].time.position - column.notes[note].time.position) * column.heightMultiplier)
                        source: {
                            if (!noteImg.held) {
                                return root.iniImagesUrl + "default.png/ln_body_inactive_" + column.color;
                            }
                            let flashing = Math.abs(chart.position % 1) > 0.5;
                            return root.iniImagesUrl + "default.png/ln_body_" + (flashing ? "flash" : "active") + "_" + column.color;
                        }
                        visible: noteImg.visible
                        width: sourceSize.width
                        y: -height

                        Component.onCompleted: {
                            console.info(y);
                        }
                    }
                }
            }
        }
    }
    Connections {
        function onPositionChanged(_) {
            let count = 0;
            let erasedNoteIndex = column.erasedNoteIndex;
            let chartPosition = chart.position;
            while (erasedNoteIndex + count < column.notes.length) {
                let note = column.notes[erasedNoteIndex + count];
                if (note.time.position > chartPosition) {
                    break;
                }
                count++;
            }
            if (count > 0 && column.notes[erasedNoteIndex + count - 1].type === Note.Type.LongNoteBegin) {
                count--;
            }
            column.erasedNoteIndex += count;
            column.visibleNoteIndex -= count;
            column.visibleNoteIndex = Math.max(0, column.visibleNoteIndex);
            if (count > 0) {
                notesModel.remove(0, count);
            }
            let visibleNoteIndex = column.visibleNoteIndex;
            count = 0;
            while (visibleNoteIndex + count < noteRepeater.count) {
                let noteImage = noteRepeater.itemAt(visibleNoteIndex + count);
                let globalPos = noteImage.mapToGlobal(0, 0);
                globalPos.y += noteImage.height;
                if (globalPos.y > 0) {
                    noteImage.visible = true;
                    count++;
                } else {
                    break;
                }
            }
            column.visibleNoteIndex += count;
        }

        target: chart
    }
}
