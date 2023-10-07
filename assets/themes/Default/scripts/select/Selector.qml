import RhythmGameQml
import QtQuick

Image {
    id: selector

    property var currentItem: null
    property bool scrollingText: false

    source: iniImagesUrl + "folders.png/frame"

    Loader {
        id: loader

        active: currentItem instanceof ChartData
        anchors.fill: parent

        sourceComponent: Component {
            Item {
                anchors.bottomMargin: 10
                anchors.fill: parent
                anchors.leftMargin: 50

                Column {
                    id: labels

                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
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
                    id: valuesColumn

                    anchors.bottom: parent.bottom
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
                Image {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    anchors.left: valuesColumn.right
                    anchors.leftMargin: 20
                    source: {
                        let rank = songList.current.rank;
                        rank = Math.min(3, Math.max(0, rank));
                        switch (rank) {
                        case 0:
                            return root.iniImagesUrl + "parts.png/very_hard";
                        case 1:
                            return root.iniImagesUrl + "parts.png/hard";
                        case 2:
                            return root.iniImagesUrl + "parts.png/normal";
                        case 3:
                            return root.iniImagesUrl + "parts.png/easy";
                        }
                    }
                }
                Column {
                    id: artistInfo

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    anchors.right: parent.right
                    anchors.rightMargin: 60
                    spacing: 5

                    NameLabel {
                        id: artist

                        anchors.right: parent.right
                        font.pixelSize: 20
                        height: metrics.height
                        scrolling: selector.scrollingText
                        text: songList.current.artist
                        width: 330
                    }
                    NameLabel {
                        id: subartist

                        anchors.right: parent.right
                        font.pixelSize: 15
                        height: metricsSmall.height
                        scrolling: selector.scrollingText
                        text: songList.current.subartist
                        width: 330
                    }
                    TextMetrics {
                        id: metrics

                        font: artist.font
                        text: "z"
                    }
                    TextMetrics {
                        id: metricsSmall

                        font: subartist.font
                        text: "z"
                    }
                }
            }
        }
    }
}