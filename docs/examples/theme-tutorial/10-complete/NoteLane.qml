pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml

Rectangle {
    id: lane
    required property Player player
    required property var columnState
    required property real visibleBeats
    required property bool scratch
    readonly property real pixelsPerBeat: lane.height / lane.visibleBeats
    color: lane.scratch ? "#30232d" : "#18232e"
    clip: true

    // [pressed]
    Rectangle {
        anchors.fill: parent
        color: "#35586a"
        visible: lane.columnState.pressed
    }
    // [pressed]

    // [notes]
    Repeater {
        model: lane.columnState
        delegate: Item {
            id: drawnNote
            required property int noteType
            required property real notePosition
            required property real longNoteEndPosition
            required property bool visibleNote
            required property bool heldLongNote
            required property bool staticLongNoteCandidate

            readonly property bool longNote: drawnNote.noteType === note.Type.LongNoteBegin
                && isFinite(drawnNote.longNoteEndPosition)
            readonly property real bottomPosition: drawnNote.staticLongNoteCandidate
                && drawnNote.longNoteEndPosition > lane.player.position
                    ? lane.player.position : drawnNote.notePosition
            readonly property real bodyLength: drawnNote.longNote
                ? (drawnNote.longNoteEndPosition - drawnNote.bottomPosition) * lane.pixelsPerBeat : 0

            width: lane.width
            height: 6
            y: lane.height - (drawnNote.bottomPosition - lane.player.position) * lane.pixelsPerBeat - height
            visible: drawnNote.visibleNote && drawnNote.noteType !== note.Type.Invisible

            Rectangle {
                width: drawnNote.width
                height: Math.abs(drawnNote.bodyLength)
                y: Math.min(0, -drawnNote.bodyLength)
                visible: drawnNote.longNote
                color: drawnNote.heldLongNote ? "#a8dcc9" : "#527266"
            }
            Rectangle {
                anchors.fill: parent
                color: drawnNote.noteType === note.Type.Landmine ? "#ef7777"
                    : lane.scratch ? "#e8bc89" : "#d9e5ee"
            }
        }
    }
    // [notes]

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 2
        color: "#ffffff"
    }
}
