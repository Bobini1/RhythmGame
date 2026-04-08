import QtQuick
import RhythmGameQml

/**
 * Hit timing distribution display.
 *
 * Shows where notes are being hit relative to perfect timing.
 *
 * Layout:
 *   • Horizontal (default) – timing axis runs left (early) → right (late).
 *   • Vertical             – timing axis runs top (early) → bottom (late).
 *     The same technique as DensityGraphContent is used: the inner Item is
 *     given swapped dimensions and rotated -90°.
 *
 * Two display modes:
 *   • Trail mode  – fading trail of the N most-recent hit positions.
 *   • EWMA mode   – single anti-aliased line tracking an exponential
 *                   weighted moving average.
 *
 * Lines (trail and EWMA) are drawn on a Canvas so they look crisp at any
 * display scale, just like the BPM overlay in DensityGraphContent.
 */
Item {
    id: hitDistribution

    // ── Required ─────────────────────────────────────────────────────────────
    required property BmsLiveScore score

    // ── Timing windows ───────────────────────────────────────────────────────
    // [{ judgement: string, windowNs: int }, …] sorted widest → narrowest.
    property var timingWindows: []

    // ── Layout ───────────────────────────────────────────────────────────────
    /** false = horizontal (timing axis left↔right), true = vertical (top↔bottom) */
    property bool vertical: false

    // ── Visibility ───────────────────────────────────────────────────────────
    property bool contentVisible: true

    // ── Display mode ─────────────────────────────────────────────────────────
    property bool ewmaMode: false
    property real ewmaAlpha: 0.1

    // ── Appearance ───────────────────────────────────────────────────────────
    property color lineColor: "white"
    property color centerLineColor: "gray"
    property real  lineWidth: 2
    property real  centerLineWidth: 1
    property real  backgroundOpacity: 1.0

    property int maxTrail: 30

    property var  hits: []
    property real ewmaValue: 0
    property bool hasAnyHit: false

    readonly property real maxWindowNs: {
        for (let w of timingWindows) {
            if (w.judgement === "Bad")
                return w.earlyNs; // Bad is symmetric, earlyNs == lateNs
        }
        return 200000000; // 200 ms fallback
    }

    // ── Inner item (rotated for vertical mode) ───────────────────────────────
    // In the natural (unrotated) coordinate system:
    //   X axis = timing deviation  (x=0 → fully early,  x=width → fully late)
    //   Y axis = neutral dimension (background bands span full height)
    Item {
        id: inner

        width:  hitDistribution.vertical ? hitDistribution.height : hitDistribution.width
        height: hitDistribution.vertical ? hitDistribution.width  : hitDistribution.height

        transformOrigin: Item.TopLeft
        rotation: hitDistribution.vertical ? -90 : 0
        transform: Translate { y: hitDistribution.vertical ? inner.width : 0 }

        // ── Background ───────────────────────────────────────────────────────
        Item {
            anchors.fill: parent
            visible: hitDistribution.contentVisible && hitDistribution.backgroundOpacity > 0
            opacity: hitDistribution.backgroundOpacity

            // Outermost band: Bad zone and beyond (dark red), fills entire area.
            Rectangle {
                anchors.fill: parent
                color: "#550000"
            }

            // Narrower coloured bands for Good / Great / Perfect,
            // centred on the timing axis.
            // EmptyPoor is excluded.
            Repeater {
                model: hitDistribution.timingWindows.filter(
                           w => w.judgement !== "EmptyPoor")

                delegate: Rectangle {
                    required property var modelData

                    height: parent.height
                    width:  parent.width * (modelData.earlyNs / hitDistribution.maxWindowNs)
                    x:      (parent.width  - width) / 2
                    y:      0

                    visible: modelData.judgement !== "Bad"
                    color: {
                        switch (modelData.judgement) {
                            case "Good":    return "#005500";
                            case "Great":   return "#555500";
                            case "Perfect": return "#000077";
                            default:        return "transparent";
                        }
                    }
                }
            }
        }

        // ── Lines canvas ─────────────────────────────────────────────────────
        // Draws: centre reference line · trail hit lines · EWMA line.
        // Using Canvas (with antialiasing) ensures the lines remain sharp
        // when the gameplay scene is scaled by a non-integer factor.
        Canvas {
            id: linesCanvas

            anchors.fill: parent
            antialiasing: true
            visible: hitDistribution.contentVisible

            onPaint: {
                let ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);

                if (!hitDistribution.contentVisible)
                    return;

                // Maps a nanosecond deviation to an x pixel position.
                function devToX(devNs) {
                    let r = Math.max(-1, Math.min(1, devNs / hitDistribution.maxWindowNs));
                    return width / 2 + r * (width / 2 - hitDistribution.lineWidth / 2);
                }

                // Centre reference line (drawn first so hit lines render on top).
                ctx.strokeStyle = hitDistribution.centerLineColor.toString();
                ctx.lineWidth   = hitDistribution.centerLineWidth;
                ctx.beginPath();
                ctx.moveTo(width / 2, 0);
                ctx.lineTo(width / 2, height);
                ctx.stroke();

                if (hitDistribution.ewmaMode) {
                    if (hitDistribution.hasAnyHit) {
                        let x = devToX(hitDistribution.ewmaValue);
                        ctx.globalAlpha  = 1.0;
                        ctx.strokeStyle  = hitDistribution.lineColor.toString();
                        ctx.lineWidth    = hitDistribution.lineWidth;
                        ctx.beginPath();
                        ctx.moveTo(x, 0);
                        ctx.lineTo(x, height);
                        ctx.stroke();
                    }
                } else {
                    // Draw trail oldest → newest so the freshest line is on top.
                    let hitList  = hitDistribution.hits;
                    let n        = hitList.length;
                    ctx.lineWidth = hitDistribution.lineWidth;
                    for (let i = n - 1; i >= 0; i--) {
                        ctx.globalAlpha = 1.0 - (i / Math.max(n, 1)) * 0.95;
                        ctx.strokeStyle = hitDistribution.lineColor.toString();
                        let x = devToX(hitList[i]);
                        ctx.beginPath();
                        ctx.moveTo(x, 0);
                        ctx.lineTo(x, height);
                        ctx.stroke();
                    }
                    ctx.globalAlpha = 1.0;
                }
            }

            // Repaint whenever anything that affects the drawn output changes.
            Connections {
                target: hitDistribution
                function onHitsChanged()            { linesCanvas.requestPaint(); }
                function onEwmaValueChanged()       { linesCanvas.requestPaint(); }
                function onEwmaModeChanged()        { linesCanvas.requestPaint(); }
                function onHasAnyHitChanged()       { linesCanvas.requestPaint(); }
                function onLineColorChanged()       { linesCanvas.requestPaint(); }
                function onCenterLineColorChanged() { linesCanvas.requestPaint(); }
                function onLineWidthChanged()       { linesCanvas.requestPaint(); }
                function onCenterLineWidthChanged() { linesCanvas.requestPaint(); }
                function onMaxWindowNsChanged()     { linesCanvas.requestPaint(); }
                function onContentVisibleChanged()  { linesCanvas.requestPaint(); }
            }

            onWidthChanged:      requestPaint()
            onHeightChanged:     requestPaint()
            Component.onCompleted: requestPaint()
        }
    }

    // ── Hit tracking ─────────────────────────────────────────────────────────

    Connections {
        target: hitDistribution.score
        enabled: hitDistribution.contentVisible

        function onHit(tap) {
            if (!tap.points)
                return;
            var j = tap.points.judgement;
            if (j === Judgement.Poor
                    || j === Judgement.EmptyPoor
                    || j === Judgement.MineHit
                    || j === Judgement.MineAvoided
                    || j === Judgement.LnEndSkip)
                return;

            var dev = tap.points.deviation;

            if (hitDistribution.ewmaMode) {
                if (!hitDistribution.hasAnyHit) {
                    hitDistribution.ewmaValue  = dev;
                    hitDistribution.hasAnyHit  = true;
                } else {
                    hitDistribution.ewmaValue =
                        hitDistribution.ewmaAlpha * dev
                        + (1.0 - hitDistribution.ewmaAlpha) * hitDistribution.ewmaValue;
                }
            } else {
                var arr = [dev].concat(
                    hitDistribution.hits.slice(0, hitDistribution.maxTrail - 1));
                hitDistribution.hits = arr;
            }
        }
    }
}
