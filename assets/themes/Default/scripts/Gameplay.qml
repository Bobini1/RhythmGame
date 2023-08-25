import QtQuick
import QtQuick.Window

Rectangle {
    anchors.fill: parent

    Text {
        anchors.centerIn: parent
        text: chart.start()
    }
}
