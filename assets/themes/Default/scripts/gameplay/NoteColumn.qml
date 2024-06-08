import QtQuick 2.0
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: column

    property double chartPosition: Math.floor(-chart.position * column.heightMultiplier)
    property string color
    property int erasedNoteIndex: 0
    property int heightMultiplier: 20
    property var missedLnEnds: {
        return {};
    }
    property int noteHeight: 36
    property var notes
    property int visibleNoteIndex: 0

    function activateLn(index: int) {
        let item = noteRepeater.itemAt(index - column.erasedNoteIndex);
        if (item) {
            item.held = true;
        }
    }
    function deactivateLn(index: int) {
        let item = noteRepeater.itemAt(index - column.erasedNoteIndex);
        item.held = false;
    }
    function markLnEndAsMissed(index: int) {
        missedLnEnds[index] = true;
    }
    function removeNote(index: int) {
        if (column.notes[index].type === Note.Type.LongNoteBegin || column.notes[index].type === Note.Type.LongNoteEnd) {
            return;
        }
        let offset = index - column.erasedNoteIndex;
        let item = noteRepeater.itemAt(offset);
        if (item) {
            item.visible = false;
            item.width = 0;
            item.height = 0;
        }
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
            property double notePosition: Math.floor(-column.notes[note].time.position * column.heightMultiplier)

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

            source: root.iniImagesUrl + "default.png/" + getTypeString() + column.color
            visible: false
            y: notePosition - height / 2

            Loader {
                id: lnBodyLoader

                active: column.notes[note].type === Note.Type.LongNoteBegin

                sourceComponent: Component {
                    Image {
                        id: lnImg

                        fillMode: Image.TileVertically
                        height: Math.floor(column.notes[note + 1].time.position * column.heightMultiplier) + noteImg.y
                        source: {
                            if (!noteImg.held) {
                                return root.iniImagesUrl + "default.png/ln_body_inactive_" + column.color;
                            }
                            let flashing = Math.abs(chart.position % 0.5) > 0.25;
                            return root.iniImagesUrl + "default.png/ln_body_" + (flashing ? "flash" : "active") + "_" + column.color;
                        }
                        width: noteImg.width
                        y: -height
                    }
                }
            }
        }
    }
    Connections {
        function onPositionChanged(_) {
            let count = 0;
            let chartPosition = chart.position;
            while (column.erasedNoteIndex + count < column.notes.length) {
                let note = column.notes[column.erasedNoteIndex + count];
                if (note.time.position > chartPosition) {
                    break;
                }
                if (column.missedLnEnds[column.erasedNoteIndex + count]) {
                    notesModel.remove(count, 1);
                    column.erasedNoteIndex += 1;
                    column.visibleNoteIndex -= 1;
                    continue;
                }
                if (!noteRepeater.itemAt(count).visible) {
                    notesModel.remove(count, 1);
                    column.erasedNoteIndex += 1;
                    column.visibleNoteIndex -= 1;
                    continue;
                }
                if (note.type === Note.Type.Landmine) {
                    notesModel.remove(count, 1);
                    column.erasedNoteIndex += 1;
                    column.visibleNoteIndex -= 1;
                    continue;
                }
                let item = noteRepeater.itemAt(count);
                item.y = Math.min(column.chartPosition - item.height / 2, item.y);
                count++;
            }
            column.visibleNoteIndex = Math.max(0, column.visibleNoteIndex - count);
            let visibleNoteIndex = column.visibleNoteIndex;
            count = 0;
            while (visibleNoteIndex + count < noteRepeater.count) {
                let noteImage = noteRepeater.itemAt(visibleNoteIndex + count);
                let globalPos = noteImage.mapToGlobal(0, 0);
                globalPos.y += noteImage.height;
                if (globalPos.y > 0) {
                    noteImage.visible = true;
                    noteImage.width = Qt.binding(() => column.width);
                    noteImage.height = Qt.binding(() => column.noteHeight);
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
