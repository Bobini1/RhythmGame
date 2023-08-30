import QtQuick
import QtQuick.Window

Rectangle {
    anchors.fill: parent

    Component.onCompleted: {
        chart.start();
    }

    Text {
        anchors.centerIn: parent
        text: chart.elapsed
    }
    Item {
        Row {
            spacing: 10

            Repeater {
                model: chart.chartData.noteData.visibleNotes
                width: 100

                NoteColumn {
                    notes: modelData
                    y: chart.elapsed / 100
                }
            }
            Repeater {
                model: chart.chartData.noteData.barLines

                Rectangle {
                    border.width: 1
                    color: "black"
                    height: 4
                    width: 8
                }
            }
            Repeater {
                model: chart.chartData.noteData.bpmChanges

                Rectangle {
                    border.width: 1
                    color: "yellow"
                    height: 4
                    width: 4
                }
            }
        }
    }
}
