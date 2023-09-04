import QtQuick
import QtQuick.Window

Rectangle {
    id: root

    property double bpm
    property double greenNumber: 400
    property double notePosition: 0

    anchors.fill: parent
    border.color: "black"
    border.width: 10

    Component.onCompleted: {
        chart.start();
    }

    // FpsCounter {
    //     anchors.right: parent.right
    //     anchors.top: parent.top
    // }
    Playfield {
        anchors.left: parent.left
        border.color: "black"
        border.width: 10
        columns: [7, 0, 1, 2, 3, 4, 5, 6]
        width: 400
        y: chart.position * root.greenNumber
    }
    // Playfield {
    //     anchors.right: parent.right
    //     border.color: "black"
    //     border.width: 10
    //     columns: [8, 9, 10, 11, 12, 13, 14, 15]
    //     width: 400
    //     y: chart.position * root.greenNumber
    // }
}
