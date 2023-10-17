import QtQuick 2.0
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: column

    property double chartPosition: Math.floor(-chart.position * column.heightMultiplier)
    property string color
    property int erasedNoteIndex: 0
    property int heightMultiplier: 20
    property double judgeLineGlobalPos
    property var missedLnEnds: {
        return {};
    }
    property int noteHeight: 36
    property var notes
    property int visibleNoteIndex: 0

    function markLnEndAsMissed(index: int) {
        missedLnEnds[index] = true;
    }
    function removeNote(index: int) {
        if (column.notes[index].type === Note.Type.LongNoteBegin) {
            return;
        }
        let count = 1;
        let offset = index - column.erasedNoteIndex;
        if (column.notes[index].type === Note.Type.LongNoteEnd) {
            count = 2;
            offset--;
        }
        column.erasedNoteIndex += count;
        column.visibleNoteIndex -= count;
        column.visibleNoteIndex = Math.max(0, column.visibleNoteIndex);
        notesModel.remove(offset, count);
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
            function hideLnBody() {
                if (column.notes[note].type === Note.Type.LongNoteBegin) {
                    lnBodyLoader.children[0].visible = false;
                }
            }

            height: column.noteHeight
            source: root.iniImagesUrl + "default.png/" + getTypeString() + column.color
            visible: false
            width: parent.width
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
                            let flashing = Math.abs(chart.position % 1) > 0.5;
                            return root.iniImagesUrl + "default.png/ln_body_" + (flashing ? "flash" : "active") + "_" + column.color;
                        }
                        visible: noteImg.visible
                        width: sourceSize.width
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
                    notesModel.remove(count - 1, 2);
                    column.erasedNoteIndex += 2;
                    continue;
                }
                let item = noteRepeater.itemAt(count);
                item.y = Math.min(column.chartPosition, item.notePosition) - item.height / 2;
                count++;
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
