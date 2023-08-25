import QtQuick
import QtQuick.Window

Rectangle {
    anchors.fill: parent

    Component.onCompleted: {
        chart.start();
    }

    Text {
        anchors.centerIn: parent
        text: {
            let text = "";
            for (let column in chart.chartData.noteData.visibleNotes) {
                for (let note of chart.chartData.noteData.visibleNotes[column]) {
                    text += note + ", ";
                }
                text += "\n";
            }
            return text;
        }
    }
}
