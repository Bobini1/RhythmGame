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
    required property real minBpm

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
    Canvas {
        id: bpmCanvas

        anchors.fill: graphContent
        z: 0
        antialiasing: false

        onPaint: {
            let ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            let bpmList = graph.bpms;
            if (!bpmList || bpmList.length === 0)
                return;

            // Last BPM change timestamp determines the total time span
            let lastTimestamp = bpmList[bpmList.length - 1].time.timestamp;
            // Use the histogram length to determine total duration if possible
            let totalDuration = lastTimestamp > 0 ? lastTimestamp : 1;

            ctx.lineWidth = 2;

            function bpmColor(bpm) {
                if (bpm === graph.mainBpm)
                    return "#44ff44"; // green for main
                if (bpm === graph.minBpm)
                    return "#4488ff"; // blue for min
                return "#ffff44"; // yellow for others
            }

            function bpmY(bpm) {
                // Main BPM is always at the center, 2*mainBpm is the top
                let maxDisplayBpm = 2 * graph.mainBpm;
                let clamped = Math.min(bpm, maxDisplayBpm);
                // Inset by 1px so the 2px stroke is never clipped at edges
                return 1 + (height - 2) * (1 - clamped / maxDisplayBpm);
            }

            for (let i = 0; i < bpmList.length; i++) {
                let bpmChange = bpmList[i];
                let bpm = bpmChange.bpm;
                let startTimestamp = bpmChange.time.timestamp;
                let endTimestamp = (i + 1 < bpmList.length) ? bpmList[i + 1].time.timestamp : totalDuration;

                let x1 = startTimestamp / totalDuration * width;
                let x2 = endTimestamp / totalDuration * width;
                let y = bpmY(bpm);

                ctx.strokeStyle = bpmColor(bpm);
                ctx.beginPath();
                ctx.moveTo(x1 - 1, y);
                ctx.lineTo(x2 + 1, y);
                ctx.stroke();

                if (i + 1 < bpmList.length) {
                    let nextY = bpmY(bpmList[i + 1].bpm);
                    if (nextY !== y) {
                        let yStart = nextY < y ? y - 1 : y + 1;
                        let yEnd = nextY < y ? nextY + 1 : nextY - 1;
                        ctx.strokeStyle = "#888888"; // gray
                        ctx.beginPath();
                        ctx.moveTo(x2, yStart);
                        ctx.lineTo(x2, yEnd);
                        ctx.stroke();
                    }
                }
            }
        }

        Connections {
            target: graph
            function onBpmsChanged() { bpmCanvas.requestPaint(); }
            function onMainBpmChanged() { bpmCanvas.requestPaint(); }
            function onMaxBpmChanged() { bpmCanvas.requestPaint(); }
            function onMinBpmChanged() { bpmCanvas.requestPaint(); }
        }
        Component.onCompleted: requestPaint()
    }
    RowLayout {
        anchors {
            left: graph.left
            right: graph.right
            bottom: graph.bottom
            top: graphContent.bottom
            margins: 14
            topMargin: 7
        }
        height: 42
        spacing: 2
        Image {
            source: root.iniImagesUrl + "parts.png/note_grey"
        }
        Text {
            text: graph.normalCount
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 26
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_red"
        }
        Text {
            text: graph.scratchCount
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 26
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_blue"
        }
        Text {
            text: graph.lnCount
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 26
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_green"
        }
        Text {
            text: graph.bssCount
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 26
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
    }
}