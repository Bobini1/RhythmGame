import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import "../common/helpers.js" as Helpers

Image {
    id: graph

    required property var bpms
    required property var histogramData
    required property var mainBpm
    required property var maxBpm
    readonly property var normalNotes: histogramData[0]
    readonly property var lns: histogramData[1]
    readonly property var maxDensity: {
        let max = null;
        for (let i = 0; i < normalNotes.length; i++) {
            let total = normalNotes[i] + lns[i];
            if (!max || total > max)
                max = total;
        }
        return max;
    }
    Component.onCompleted: {
        console.info(JSON.stringify(histogramData));
        console.info(JSON.stringify(bpms));
    }
    asynchronous: true
    source: root.iniImagesUrl + "parts.png/graph"

    Rectangle {
        id: graphContent
        anchors {
            fill: parent
            leftMargin: 14
            rightMargin: 14
            topMargin: 14
            bottomMargin: 54
        }
        opacity: 0.8
        color: "black"
        RowLayout {
            anchors.fill: parent
            Repeater {
                model: graph.normalNotes.length
                Column {
                    width: graphContent.width / graph.normalNotes.length - 2
                    spacing: 0
                    Rectangle {
                        height: graphContent.height * (graph.normalNotes[index] / graph.maxDensity)
                        color: "white"
                    }
                    Rectangle {
                        height: graphContent.height * (graph.lns[index] / graph.maxDensity)
                        color: "red"
                    }
                    Layout.alignment: Qt.AlignBottom
                }
            }
        }
    }
}