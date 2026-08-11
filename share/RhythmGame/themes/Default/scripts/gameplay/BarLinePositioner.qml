import QtQuick
import RhythmGameQml

Item {
    id: column

    required property real heightMultiplier
    required property Player player
    property alias model: barlinesRepeater.model
    clip: true

    Item {
        anchors.fill: parent
        transform: Translate {
            y: column.player.position * column.heightMultiplier
        }

        Repeater {
            id: barlinesRepeater
            delegate: Item {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: display.time.position * column.heightMultiplier
                anchors.left: parent.left
                anchors.right: parent.right

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    antialiasing: true
                    color: "gray"
                    height: 1
                    y: -0.5
                }
            }
        }
    }
}
