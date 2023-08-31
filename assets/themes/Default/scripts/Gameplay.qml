import QtQuick
import QtQuick.Window

Rectangle {
    id: root

    property double bpm
    property double greenNumber: 400
    property double notePosition: 0

    anchors.fill: parent

    Component.onCompleted: {
        chart.start();
    }

    Text {
        anchors.centerIn: parent
        text: chart.elapsed
    }
    FpsCounter {
        anchors.right: parent.right
        anchors.top: parent.top
    }
    Item {
        id: playfield

        anchors.bottom: parent.bottom
        height: children.height

        BarLinePositioner {
            barLines: chart.chartData.noteData.barLines
            heightMultiplier: root.greenNumber
            width: notesRow.width
            // start from bottom
            y: chart.position * root.greenNumber - playfield.height
        }
        Row {
            id: notesRow

            anchors.bottom: parent.bottom
            spacing: 10

            Repeater {
                model: chart.chartData.noteData.visibleNotes
                width: 100

                NoteColumn {
                    heightMultiplier: root.greenNumber
                    notes: modelData
                    y: chart.position * root.greenNumber - playfield.height
                }
            }
        }
    }
    Connections {
        function onBpmChanged(bpmChange) {
            root.bpm = bpmChange.bpm;
        }

        target: chart
    }
}
