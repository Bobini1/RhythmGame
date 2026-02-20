import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import "../common/helpers.js" as Helpers

Image {
    id: graph

    required property var bpms
    required property var histogramData
    required property real mainBpm
    required property real maxBpm

    required property int normalCount
    required property int scratchCount
    required property int lnCount
    required property int bssCount

    readonly property var normalNotes: histogramData[0]
    readonly property var scratches: histogramData[1]
    readonly property var lns: histogramData[2]
    readonly property var bss: histogramData[3]
    readonly property real maxDensity: {
        let max = null;
        for (let i = 0; i < normalNotes.length; i++) {
            let total = normalNotes[i] + scratches[i] + lns[i] + bss[i];
            if (!max || total > max)
                max = total;
        }
        // If below 15 kps, scale rectangles down
        return Math.max(max || 1, 15);
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
        z: -2
    }
    RowLayout {
        anchors.fill: graphContent
        spacing: {
            let availableWidth = width;
            let requiredWidth = graph.normalNotes.length;
            return availableWidth > requiredWidth ? 1 : 0;
        }
        anchors.leftMargin: 1
        anchors.rightMargin: 1
        z: -1
        Repeater {
            model: graph.normalNotes.length
            ColumnLayout {
                id: column
                spacing: 0
                Layout.fillHeight: true
                Layout.fillWidth: true
                Rectangle {
                    Layout.preferredHeight: graphContent.height * (graph.bss[index] / graph.maxDensity)
                    color: "green"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Rectangle {
                    Layout.preferredHeight: graphContent.height * (graph.scratches[index] / graph.maxDensity)
                    color: "red"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Rectangle {
                    Layout.preferredHeight: graphContent.height * (graph.lns[index] / graph.maxDensity)
                    color: "blue"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Rectangle {
                    Layout.preferredHeight: graphContent.height * (graph.normalNotes[index] / graph.maxDensity)
                    color: "white"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Rectangle {
                    Layout.preferredHeight: graphContent.height * ((graph.maxDensity - graph.bss[index] - graph.scratches[index] - graph.lns[index] - graph.normalNotes[index]) / graph.maxDensity)
                    color: "transparent"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Layout.alignment: Qt.AlignTop
                transform: Scale {
                    yScale: -1
                    origin.y: graphContent.height / 2
                }
            }
        }
    }
    RowLayout {
        anchors {
            left: graph.left
            right: graph.right
            bottom: graph.bottom
            top: graphContent.bottom
            margins: 14
        }
        height: 42
        spacing: 2
        Image {
            source: root.iniImagesUrl + "parts.png/note_grey"
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: graph.normalCount
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 26
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_red"
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: graph.scratchCount
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 26
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_blue"
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: graph.lnCount
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 26
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_green"
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: graph.bssCount
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 26
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
    }
}