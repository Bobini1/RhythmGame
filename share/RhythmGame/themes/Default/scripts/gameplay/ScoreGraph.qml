import QtQuick

Item {
    id: scoreGraph

    // Live data
    property real currentPoints: 0
    required property real maxPoints

    property string graphBackground: ""

    // Bar width as a fraction of the total available bar-area width (1.0 = fill all)
    property real barWidthRatio: 1.0

    property real targetPoints: 0
    property real targetFinalPoints: 0
    property real bestPoints: 0
    property real bestFinalPoints: 0
    property real bestMaxPoints: 0

    // Reactive fractions — drive Rectangle sizes directly
    readonly property real mp: maxPoints > 0 ? maxPoints : 1
    readonly property real tgtFinal: targetFinalPoints / mp
    readonly property real tgtNow: targetPoints / mp
    readonly property real bestFinal: bestFinalPoints / mp
    readonly property real bestNow: bestPoints / mp
    readonly property bool hasTgt: targetFinalPoints > 0 || targetPoints > 0
    readonly property real curFrac: currentPoints / mp

    readonly property var grades: [
        { label: "MAX", frac: 1.0,        color: "#4488FF" },
        { label: "AAA", frac: 8.0 / 9.0,  color: "#4488FF" },
        { label: "AA",  frac: 7.0 / 9.0,  color: "#4488FF" },
        { label: "A",   frac: 6.0 / 9.0,  color: "#4488FF" },
    ]

    readonly property real fontSize: Math.max(7, scoreGraph.height / 27)
    readonly property real topPad: Math.round(fontSize * 1.6 + 2)
    readonly property real labelColWidth: Math.ceil(fontSize * 3.4)
    readonly property int lineInnerH: 1
    readonly property int lineOuterH: 3

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
        readonly property real barW: Math.floor((width - barGap * 4) / 3 * scoreGraph.barWidthRatio)
        readonly property real totalBarGroupW: barGap * 4 + barW * 3
        readonly property real barOffset: width - totalBarGroupW

        // ── Current bar (blue) ──────────────────────────────────────
        Item {
            x: barArea.barOffset + barArea.barGap;  y: 0
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
            x: barArea.barOffset + barArea.barGap * 2 + barArea.barW;  y: 0
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
            x: barArea.barOffset + barArea.barGap * 3 + barArea.barW * 2;  y: 0
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
                readonly property real delta: scoreGraph.currentPoints - scoreGraph.bestPoints
                readonly property string num: String(Math.abs(Math.round(delta))).padStart(4, "0")
                text: (deltaDisplay.showFull ? "MYBEST: " : "B: ") + (delta >= 0 ? "+" : "-") + num
                color: "white"; font.pixelSize: scoreGraph.fontSize; font.bold: true
                style: Text.Outline; styleColor: "black"
                font.family: "monospace"
            }
        }
    }
}

