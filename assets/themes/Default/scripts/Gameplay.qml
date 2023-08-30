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

                Column {
                    id: column

                    property int columnIndex: index
                    property var notes: modelData

                    Repeater {
                        model: column.notes

                        Rectangle {
                            border.width: 1
                            color: "red"
                            height: 4
                            width: 8
                        }
                        Text {
                            text: modelData.snap.numerator + "/" + modelData.snap.denominator
                        }
                    }
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
