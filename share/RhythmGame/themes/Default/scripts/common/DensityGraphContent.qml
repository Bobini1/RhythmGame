import QtQuick
import QtQuick.Layouts

Item {
    id: root

    // --- chart data ---
    required property var  histogramData   // [[normal], [scratch], [ln], [bss]]
    required property var  bpms            // array of { bpm, time: { timestamp } }
    required property real mainBpm
    required property real maxBpm
    required property real minBpm
    required property real length          // song length (same time unit as timestamps)
    required property bool gapsEnabled

    // --- orientation ---
    property bool vertical: false

    // --- visibility toggles ---
    property bool showNotes: true   // show the note-density histogram bars
    property bool showBpm:   true   // show the BPM overlay lines

    // --- background ---
    property real backgroundOpacity: 0.8

    property real positionRatio: -1

    readonly property var  normalNotes: histogramData[0]
    readonly property var  scratches:   histogramData[1]
    readonly property var  lns:         histogramData[2]
    readonly property var  bss:         histogramData[3]
    readonly property real maxDensity: {
        let max = null;
        for (let i = 0; i < normalNotes.length; i++) {
            let total = normalNotes[i] + scratches[i] + lns[i] + bss[i];
            if (max === null || total > max)
                max = total;
        }
        return Math.max(max || 1, 15);
    }

    Item {
        id: inner

        width:  root.vertical ? root.height : root.width
        height: root.vertical ? root.width  : root.height

        transformOrigin: Item.TopLeft
        rotation: root.vertical ? -90 : 0
        transform: Translate { y: root.vertical ? inner.width : 0 }

        // Semi-transparent black background
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: root.backgroundOpacity
            z: -2
        }

        // Histogram bars
        RowLayout {
            visible: root.showNotes
            anchors {
                fill: parent
                leftMargin: 1
                rightMargin: 1
            }
            spacing: {
                let requiredWidth = root.normalNotes.length * 2;
                return width > requiredWidth && root.gapsEnabled ? 1 : 0;
            }
            z: -1

            Repeater {
                model: root.normalNotes.length

                ColumnLayout {
                    spacing: 0
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop

                    Rectangle {
                        Layout.preferredHeight: inner.height * (root.bss[index] / root.maxDensity)
                        Layout.fillWidth: true
                        color: "#3BDC3B"
                    }
                    Rectangle {
                        Layout.preferredHeight: inner.height * (root.scratches[index] / root.maxDensity)
                        Layout.fillWidth: true
                        color: "#FF4444"
                    }
                    Rectangle {
                        Layout.preferredHeight: inner.height * (root.lns[index] / root.maxDensity)
                        Layout.fillWidth: true
                        color: "#4444FF"
                    }
                    Rectangle {
                        Layout.preferredHeight: inner.height * (root.normalNotes[index] / root.maxDensity)
                        Layout.fillWidth: true
                        color: "#CCCCCC"
                    }
                    // Transparent spacer so bars grow from the bottom
                    Rectangle {
                        Layout.preferredHeight: inner.height * (
                            (root.maxDensity - root.bss[index] - root.scratches[index]
                             - root.lns[index] - root.normalNotes[index]) / root.maxDensity)
                        Layout.fillWidth: true
                        color: "transparent"
                    }

                    transform: Scale { yScale: -1; origin.y: inner.height / 2 }
                }
            }
        }

        // BPM overlay (drawn in canvas coordinates of the horizontal layout)
        Canvas {
            id: bpmCanvas
            visible: root.showBpm
            anchors.fill: parent
            antialiasing: false

            onPaint: {
                let ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);

                let bpmList = root.bpms;
                if (!bpmList || bpmList.length === 0)
                    return;

                let totalDuration = root.length > 0 ? root.length : 1;
                ctx.lineWidth = 2;

                function bpmColor(bpm) {
                    if (bpm === root.mainBpm) return "#00FF00";
                    if (bpm === root.minBpm)  return "#0000FF";
                    return "#FFFF00";
                }

                function bpmY(bpm) {
                    let maxDisplayBpm = 2 * root.mainBpm;
                    let clamped = Math.min(bpm, maxDisplayBpm);
                    return 1 + (height - 2) * (1 - clamped / maxDisplayBpm);
                }

                for (let i = 0; i < bpmList.length; i++) {
                    let bpmChange = bpmList[i];
                    let bpm = bpmChange.bpm;
                    let startTs = bpmChange.time.timestamp;
                    let endTs = (i + 1 < bpmList.length)
                                ? bpmList[i + 1].time.timestamp
                                : totalDuration;

                    let x1 = startTs / totalDuration * width;
                    let x2 = endTs   / totalDuration * width;
                    let y  = bpmY(bpm);

                    ctx.strokeStyle = bpmColor(bpm);
                    ctx.beginPath();
                    ctx.moveTo(x1 - 1, y);
                    ctx.lineTo(x2 + 1, y);
                    ctx.stroke();

                    if (i + 1 < bpmList.length) {
                        let nextY = bpmY(bpmList[i + 1].bpm);
                        if (nextY !== y) {
                            let yStart = nextY < y ? y - 1 : y + 1;
                            let yEnd   = nextY < y ? nextY + 1 : nextY - 1;
                            ctx.strokeStyle = "#7F7F7F";
                            ctx.beginPath();
                            ctx.moveTo(x2, yStart);
                            ctx.lineTo(x2, yEnd);
                            ctx.stroke();
                        }
                    }
                }
            }

            Connections {
                target: root
                function onBpmsChanged()   { bpmCanvas.requestPaint(); }
                function onMainBpmChanged(){ bpmCanvas.requestPaint(); }
                function onMaxBpmChanged() { bpmCanvas.requestPaint(); }
                function onMinBpmChanged() { bpmCanvas.requestPaint(); }
                function onLengthChanged() { bpmCanvas.requestPaint(); }
            }
            Component.onCompleted: requestPaint()
        }

        // Red position line – drawn on a Canvas so it sits at the exact
        // sub-pixel x coordinate.  With antialiasing=true the 1.5 px stroke
        // is blended across adjacent pixels, giving a constant visual weight
        // even when the gameplay scene is scaled by a non-integer factor.
        Canvas {
            id: positionCanvas
            anchors.fill: parent
            antialiasing: true
            visible: root.positionRatio >= 0
            z: 1

            onPaint: {
                let ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                if (root.positionRatio < 0)
                    return;
                let x = root.positionRatio * width;
                ctx.strokeStyle = "#FF0000";
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, height);
                ctx.stroke();
            }

            Connections {
                target: root
                function onPositionRatioChanged() { positionCanvas.requestPaint(); }
            }
        }
    }
}








