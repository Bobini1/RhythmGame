import QtQuick

Item {
    id: scoreGraph

    // Live data
    property real currentPoints: 0
    property real maxPoints: 1
    property real elapsed: 0       // nanoseconds
    property real chartLength: 1   // nanoseconds

    property var bestHitEvents: null
    property var targetHitEvents: null  // kept for API compatibility

    property string graphBackground: ""

    // Live target (from scoreReplayer / fraction / battle)
    property real targetPoints: 0
    // End-of-song target (from saved score / fraction / battle)
    property real targetFinalPoints: 0
    // Note-by-note best score replay points (from ScoreReplayer, same logic as ghost score)
    property real bestReplayPoints: 0

    property var bestHistory: []

    function buildScoreHistory(hitEvents) {
        if (!hitEvents || hitEvents.length === 0)
            return [];
        let points = 0;
        let history = [[0, 0]];
        let seenNotes = {};
        for (let i = 0; i < hitEvents.length; i++) {
            let ev = hitEvents[i];
            if (!ev.noteRemoved || !ev.points)
                continue;
            if (ev.noteIndex < 0)
                continue;
            let key = String(ev.column) + "_" + String(ev.noteIndex);
            if (seenNotes[key])
                continue;
            seenNotes[key] = true;
            points += ev.points.value;
            history.push([ev.offsetFromStart, points]);
        }
        return history;
    }

    // Interpolate score value at a given elapsed time from a history array.
    function scoreAtElapsed(history, elapsedNs) {
        if (!history || history.length === 0)
            return 0;
        if (elapsedNs <= history[0][0])
            return history[0][1];
        let last = history[history.length - 1];
        if (elapsedNs >= last[0])
            return last[1];
        let lo = 0, hi = history.length - 1;
        while (lo + 1 < hi) {
            let mid = Math.floor((lo + hi) / 2);
            if (history[mid][0] <= elapsedNs)
                lo = mid;
            else
                hi = mid;
        }
        let t0 = history[lo][0], t1 = history[hi][0];
        let s0 = history[lo][1], s1 = history[hi][1];
        if (t1 === t0)
            return s1;
        return s0 + (s1 - s0) * (elapsedNs - t0) / (t1 - t0);
    }

    function reset() {
        bestHistory = buildScoreHistory(bestHitEvents);
    }

    onBestHitEventsChanged: bestHistory = buildScoreHistory(bestHitEvents)

    // Reactive fractions — drive Rectangle sizes directly
    readonly property real mp: maxPoints > 0 ? maxPoints : 1
    readonly property real bestFinal: (bestHistory && bestHistory.length > 0)
        ? bestHistory[bestHistory.length - 1][1] / mp : 0
    readonly property real bestNow: (bestHistory && bestHistory.length > 0)
        ? scoreAtElapsed(bestHistory, elapsed) / mp : 0
    readonly property real tgtFinal: targetFinalPoints / mp
    readonly property real tgtNow: targetPoints / mp
    readonly property bool hasTgt: targetFinalPoints > 0 || targetPoints > 0
    readonly property real curFrac: currentPoints / mp

    readonly property var grades: [
        { label: "MAX", frac: 1.0,        color: "#4488FF" },
        { label: "AAA", frac: 8.0 / 9.0,  color: "#4488FF" },
        { label: "AA",  frac: 7.0 / 9.0,  color: "#4488FF" },
        { label: "A",   frac: 6.0 / 9.0,  color: "#4488FF" },
    ]

    readonly property real fontSize: Math.max(7, scoreGraph.height / 27)
    // Vertical space reserved above the bar area so MAX label fits above the MAX line
    readonly property real topPad: Math.round(fontSize * 1.6 + 2)
    // Label column width — just wide enough for the longest label ("MAX" / "AAA")
    readonly property real labelColWidth: Math.ceil(fontSize * 3.4)
    // Grade reference lines — always thin, not scaled
    readonly property int lineInnerH: 1
    readonly property int lineOuterH: 3

    // -----------------------------------------------------------------
    // Visuals
    // -----------------------------------------------------------------

    // Wraps all content so the drag border (added as a child in Gameplay.qml)
    // remains visible even when contentVisible is false.
    property bool contentVisible: true
    Rectangle {
        id: content
        anchors.fill: parent
        visible: scoreGraph.contentVisible && scoreGraph.graphBackground === ""
        color: "#111111"
        opacity: 0.8
    }

    Image {
        anchors.fill: parent
        source: scoreGraph.graphBackground !== ""
            ? root.imagesUrl + "scoregraph/" + scoreGraph.graphBackground
            : ""
        fillMode: Image.Stretch
        visible: scoreGraph.contentVisible && scoreGraph.graphBackground !== ""
    }

    // Grade reference lines — full width, offset by topPad
    Repeater {
        model: scoreGraph.grades
        Item {
            required property var modelData
            anchors.left: parent.left
            anchors.right: parent.right
            height: scoreGraph.lineOuterH
            visible: scoreGraph.contentVisible
            y: scoreGraph.topPad + Math.round((1.0 - modelData.frac) * barArea.height) - Math.floor(scoreGraph.lineOuterH / 2)

            Rectangle { anchors.fill: parent; color: "black" }
            Rectangle {
                anchors.centerIn: parent
                width: parent.width; height: scoreGraph.lineInnerH
                color: modelData.color; opacity: 0.9
            }
        }
    }

    // Left label column
    Item {
        id: labelCol
        width: scoreGraph.labelColWidth
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: scoreGraph.contentVisible

        Repeater {
            model: scoreGraph.grades
            Text {
                required property var modelData
                text: modelData.label
                color: modelData.color
                font.bold: true
                font.pixelSize: scoreGraph.fontSize
                style: Text.Outline
                renderType: Text.QtRendering
                styleColor: "black"
                width: labelCol.width - 2
                horizontalAlignment: Text.AlignLeft
                anchors.left: parent.left
                anchors.leftMargin: 2
                y: Math.max(0,
                    scoreGraph.topPad
                    + Math.round((1.0 - modelData.frac) * barArea.height)
                    - implicitHeight - 2)
            }
        }
    }

    // Bar area (right of label column)
    Item {
        id: barArea
        anchors.left: labelCol.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: scoreGraph.topPad
        anchors.bottom: parent.bottom
        anchors.rightMargin: 1
        visible: scoreGraph.contentVisible

        readonly property real barGap: 3
        readonly property real barW: Math.floor((width - barGap * 4) / 3)
        
        // ── Current bar (blue) ──────────────────────────────────────
        Item {
            x: barArea.barGap;  y: 0
            width: barArea.barW;  height: barArea.height

            Rectangle {
                color: "blue";  width: parent.width
                height: Math.max(0, scoreGraph.curFrac * parent.height)
                anchors.bottom: parent.bottom
                visible: scoreGraph.curFrac > 0
            }
        }

        // ── Best bar (green) ────────────────────────────────────────
        Item {
            x: barArea.barGap * 2 + barArea.barW;  y: 0
            width: barArea.barW;  height: barArea.height

            Rectangle {  // ghost
                color: "#282828";  opacity: 0.625;  width: parent.width
                height: Math.max(0, (scoreGraph.bestFinal - scoreGraph.bestNow) * parent.height)
                y: Math.round((1.0 - scoreGraph.bestFinal) * parent.height)
                visible: scoreGraph.bestFinal > scoreGraph.bestNow
            }
            Rectangle {  // fill
                color: "green";  width: parent.width
                height: Math.max(0, scoreGraph.bestNow * parent.height)
                anchors.bottom: parent.bottom
                visible: scoreGraph.bestNow > 0
            }
        }

        // ── Target bar (red) ────────────────────────────────────────
        Item {
            x: barArea.barGap * 3 + barArea.barW * 2;  y: 0
            width: barArea.barW;  height: barArea.height

            Rectangle {  // ghost
                color: "#282828";  opacity: 0.625;  width: parent.width
                height: Math.max(0, (scoreGraph.tgtFinal - scoreGraph.tgtNow) * parent.height)
                y: Math.round((1.0 - scoreGraph.tgtFinal) * parent.height)
                visible: scoreGraph.hasTgt && scoreGraph.tgtFinal > scoreGraph.tgtNow
            }
            Rectangle {  // fill
                color: "red";  width: parent.width
                height: Math.max(0, scoreGraph.tgtNow * parent.height)
                anchors.bottom: parent.bottom
                visible: scoreGraph.hasTgt && scoreGraph.tgtNow > 0
            }
        }
    }
    // ── Bottom-right score delta display ────────────────────────
    Item {
        id: deltaDisplay
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: deltaCol.implicitHeight
        visible: scoreGraph.contentVisible

        // Invisible width measurer texts
        Text {
            id: mFull
            visible: false
            font.pixelSize: scoreGraph.fontSize
            font.bold: true
            text: "TARGET: +0000"
            font.family: "monospace"
        }
        Text {
            id: mShort
            visible: false
            font.pixelSize: scoreGraph.fontSize
            font.bold: true
            text: "T: +0000"
            font.family: "monospace"
        }

        readonly property bool showFull:  mFull.implicitWidth  <= deltaDisplay.width
        readonly property bool showShort: !showFull && (mShort.implicitWidth <= deltaDisplay.width)
        readonly property bool showAny:   showFull || showShort

        Column {
            id: deltaCol
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: 1

            Text {
                anchors.right: parent.right
                visible: deltaDisplay.showAny && scoreGraph.hasTgt
                readonly property real delta: scoreGraph.currentPoints - scoreGraph.targetPoints
                readonly property string num: String(Math.abs(Math.round(delta))).padStart(4, "0")
                text: (deltaDisplay.showFull ? "TARGET: " : "T: ") + (delta >= 0 ? "+" : "-") + num
                color: "white"; font.pixelSize: scoreGraph.fontSize; font.bold: true
                style: Text.Outline; styleColor: "black"
                font.family: "monospace"
            }
            Text {
                anchors.right: parent.right
                visible: deltaDisplay.showAny && scoreGraph.bestFinal > 0
                readonly property real delta: scoreGraph.currentPoints - scoreGraph.bestReplayPoints
                readonly property string num: String(Math.abs(Math.round(delta))).padStart(4, "0")
                text: (deltaDisplay.showFull ? "MYBEST: " : "B: ") + (delta >= 0 ? "+" : "-") + num
                color: "white"; font.pixelSize: scoreGraph.fontSize; font.bold: true
                style: Text.Outline; styleColor: "black"
                font.family: "monospace"
            }
        }
    }
}

