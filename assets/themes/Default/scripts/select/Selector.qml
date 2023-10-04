import RhythmGameQml
import QtQuick

Image {
    id: selector

    property var currentItem: null

    source: iniImagesUrl + "folders.png/frame"

    Loader {
        id: loader

        active: currentItem instanceof ChartData
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        sourceComponent: Component {
            Item {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                anchors.left: parent.left
                anchors.leftMargin: 50
                height: childrenRect.height
                width: childrenRect.width

                Column {
                    id: labels

                    spacing: 0

                    Text {
                        id: bpmText

                        font.pixelSize: 20
                        text: "BPM"
                    }
                    Text {
                        id: keysText

                        font.pixelSize: 20
                        text: "KEYS"
                    }
                }
                Column {
                    anchors.left: labels.right
                    anchors.leftMargin: 10
                    spacing: 0
                    width: 50

                    Text {
                        id: bpmValue

                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: 20
                        text: songList.current.initialBpm
                    }
                    Text {
                        id: keysValue

                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: 20
                        text: songList.current.keymode
                    }
                }
            }
        }
    }
}
