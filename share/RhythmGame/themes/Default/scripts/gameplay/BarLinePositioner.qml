import QtQuick

Item {
    id: column

    required property real heightMultiplier
    required property real position
    property alias model: barlinesRepeater.model

    FrameAnimation {
        running: true
        onTriggered: {
            let top = column.height / column.heightMultiplier;
            barlinesRepeater.model.topPosition = column.position + top;
            barlinesRepeater.model.bottomPosition = column.position;
        }
    }

    Item {
        anchors.fill: parent
        anchors.bottomMargin: -column.position * column.heightMultiplier

        Rectangle {
            color: "transparent"
            anchors.fill: parent
            border.color: "blue"
            border.width: 5
        }

        Repeater {
            id: barlinesRepeater
            delegate: Item {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: display.time.position * column.heightMultiplier
                anchors.left: parent.left
                anchors.right: parent.right
                Rectangle {
                    color: "gray"
                    height: 1
                    y: -height / 2
                    anchors.left: parent.left
                    anchors.right: parent.right
                }
            }
        }
    }
}
