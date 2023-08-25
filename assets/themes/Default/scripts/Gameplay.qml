import QtQuick
import QtQuick.Window

Rectangle {
    anchors.fill: parent

    Component.onCompleted: {
        chart.start();
    }

    Text {
        anchors.centerIn: parent
        text: chart.chartData
    }
}
