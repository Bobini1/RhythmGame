import QtQuick
import RhythmGameQml

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var skinClock: null
    property int skinClockMode: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property var chart
    property var score: null
    property var screenRoot: null
    property real scaleOverride: 1.0

    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState && timelineState.staticState.valid
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        skinClock: root.skinClock
        clockMode: root.skinClockMode
        dsts: root.dsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }
    readonly property var currentState: staticTimelineState
        || (timelineState.hasState ? timelineState.state : null)
    readonly property int fieldW: Math.max(1, srcData ? (srcData.fieldW || 1) : 1)
    readonly property int fieldH: Math.max(1, srcData ? (srcData.fieldH || 1) : 1)
    readonly property int center: Math.max(1, srcData ? (srcData.judgeWidthMillis || 250) : 250)
    readonly property int sourceW: center * 2 + 1
    readonly property real stateW: currentState ? (currentState.w || 0) : 0
    readonly property real stateH: currentState ? (currentState.h || 0) : 0
    readonly property real drawW: chartDstExtent(fieldW, stateW)
    readonly property real drawH: chartDstExtent(fieldH, stateH)
    readonly property real drawX: currentState
        ? (currentState.x + (stateW < 0 ? -drawW : 0)) * scaleOverride
        : 0
    readonly property real drawY: currentState
        ? (currentState.y - drawH) * scaleOverride
        : 0
    readonly property var chartData: {
        if (!chart) {
            return null;
        }
        return chart.chartData !== undefined ? chart.chartData : chart;
    }
    readonly property var hitEvents: score && score.replayData
        ? (score.replayData.hitEvents || [])
        : []
    readonly property var recentEvents: recentTimingEvents()

    function chartDstExtent(fieldSize: var, stateSize: var) : var {
        let size = Math.abs(Number(stateSize || 0));
        if (size <= 0) {
            return fieldSize;
        }
        return size <= 4 ? fieldSize * size : size;
    }

    function clamp(value: var, low: var, high: var) : var {
        return Math.max(low, Math.min(high, value));
    }

    function colorObject(value: var, fallback: var) : var {
        let raw = value === undefined || value === null ? "" : String(value);
        raw = raw.replace(/[^0-9a-fA-F]/g, "");
        if (raw.length >= 8) {
            return {
                r: parseInt(raw.substring(0, 2), 16),
                g: parseInt(raw.substring(2, 4), 16),
                b: parseInt(raw.substring(4, 6), 16),
                a: parseInt(raw.substring(6, 8), 16) / 255.0
            };
        }
        if (raw.length >= 6) {
            return {
                r: parseInt(raw.substring(0, 2), 16),
                g: parseInt(raw.substring(2, 4), 16),
                b: parseInt(raw.substring(4, 6), 16),
                a: 1.0
            };
        }
        return colorObject(fallback, "ff0000ff");
    }

    function colorString(color: var, alphaMultiplier: var) : var {
        let alpha = root.clamp((color.a === undefined ? 1 : color.a) * alphaMultiplier, 0, 1);
        return "rgba(" + color.r + "," + color.g + "," + color.b + "," + alpha + ")";
    }

    function timingWindowFor(judgementName: var, fallbackMs: var) : var {
        let windows = root.chartData && root.chartData.timingWindows
            ? root.chartData.timingWindows
            : [];
        for (let i = 0; i < windows.length; ++i) {
            let win = windows[i];
            if (String(win && win.judgement ? win.judgement : "") !== judgementName) {
                continue;
            }
            let early = Math.round(Number(win.earlyNs || 0) / 1000000);
            let late = Math.round(Number(win.lateNs || 0) / 1000000);
            return { low: -late, high: early };
        }
        return { low: -fallbackMs, high: fallbackMs };
    }

    function judgeAreas() : var {
        return [
            Object.assign({ color: root.colorObject(root.srcData ? root.srcData.pgColor : "", "0088ffcc") },
                timingWindowFor("Perfect", 21)),
            Object.assign({ color: root.colorObject(root.srcData ? root.srcData.grColor : "", "00ff88cc") },
                timingWindowFor("Great", 60)),
            Object.assign({ color: root.colorObject(root.srcData ? root.srcData.gdColor : "", "ffff00cc") },
                timingWindowFor("Good", 120)),
            Object.assign({ color: root.colorObject(root.srcData ? root.srcData.bdColor : "", "ff8800cc") },
                timingWindowFor("Bad", 200)),
            {
                low: -root.center,
                high: root.center,
                color: (root.srcData && (root.srcData.transparent || 0) === 1)
                    ? { r: 0, g: 0, b: 0, a: 0 }
                    : root.colorObject(root.srcData ? root.srcData.prColor : "", "ff0000cc")
            }
        ];
    }

    function judgementForHit(hit: var) : var {
        return root.screenRoot && root.screenRoot.gameplayJudgementFromHit
            ? root.screenRoot.gameplayJudgementFromHit(hit)
            : (hit && hit.points && hit.points.judgement !== undefined ? hit.points.judgement : -1);
    }

    function hitDeviationNanos(hit: var) : var {
        if (hit && hit.points && hit.points.deviation !== undefined) {
            return Number(hit.points.deviation || 0);
        }
        if (hit && hit.hitOffset !== undefined) {
            return Number(hit.hitOffset || 0);
        }
        return 0;
    }

    function timingMillis(hit: var) : var {
        return Math.round(-root.hitDeviationNanos(hit) / 1000000);
    }

    function visualizerJudgement(judgement: var) : var {
        return judgement >= Judgement.Bad && judgement <= Judgement.Perfect;
    }

    function recentTimingEvents() : var {
        let result = [];
        let events = root.hitEvents || [];
        for (let i = 0; i < events.length; ++i) {
            let hit = events[i];
            if (!hit || !hit.noteRemoved) {
                continue;
            }
            if (!root.visualizerJudgement(root.judgementForHit(hit))) {
                continue;
            }
            let offset = root.timingMillis(hit);
            if (offset < -root.center || offset > root.center) {
                continue;
            }
            result.push(offset);
            if (result.length > 100) {
                result.shift();
            }
        }
        return result;
    }

    function sourceToCanvasX(sourceX: var, width: var) : var {
        return width * sourceX / Math.max(1, root.sourceW);
    }

    function drawJudgeBackground(ctx: var, width: var, height: var) : void {
        let areas = root.judgeAreas();
        let centerX = root.sourceToCanvasX(root.center, width);
        let centerLineW = Math.max(1, width / root.sourceW);

        ctx.fillStyle = root.colorString(root.colorObject(root.srcData ? root.srcData.centerColor : "", "ffffffff"), 1);
        ctx.fillRect(centerX, 0, centerLineW, height);

        let beforeLeft = root.center;
        let beforeRight = root.center + 1;
        let lowBound = -root.center;
        let highBound = root.center;
        for (let i = 0; i < areas.length; ++i) {
            let area = areas[i];
            let x1 = root.center + root.clamp(Math.round(area.low), lowBound, highBound);
            let x2 = root.center + root.clamp(Math.round(area.high), lowBound, highBound) + 1;
            x1 = root.clamp(x1, 0, root.sourceW);
            x2 = root.clamp(x2, 0, root.sourceW);
            ctx.fillStyle = root.colorString(area.color, 1);
            if (beforeLeft > x1) {
                let left = root.sourceToCanvasX(x1, width);
                let right = root.sourceToCanvasX(beforeLeft, width);
                ctx.fillRect(left, 0, Math.max(1, right - left), height);
                beforeLeft = x1;
            }
            if (x2 > beforeRight) {
                let left = root.sourceToCanvasX(beforeRight, width);
                let right = root.sourceToCanvasX(x2, width);
                ctx.fillRect(left, 0, Math.max(1, right - left), height);
                beforeRight = x2;
            }
        }

        ctx.fillStyle = "rgba(0,0,0,0.25)";
        for (let x = root.center % 10; x < root.sourceW; x += 10) {
            let canvasX = root.sourceToCanvasX(x, width);
            ctx.fillRect(canvasX, 0, Math.max(1, width / root.sourceW), height);
        }
    }

    function drawRecentLines(ctx: var, width: var, height: var) : void {
        let events = root.recentEvents || [];
        let lineColor = root.colorObject(root.srcData ? root.srcData.lineColor : "", "00ff00ff");
        let lineWidth = Math.max(1, Math.min(4, root.srcData ? (root.srcData.lineWidth || 1) : 1))
            * root.scaleOverride;
        let drawDecay = root.srcData && (root.srcData.drawDecay || 0) === 1;
        for (let i = 0; i < events.length; ++i) {
            let x = root.sourceToCanvasX(root.center + events[i], width) - lineWidth / 2;
            let alpha = drawDecay ? (i + 1) / Math.max(1, events.length) : 1;
            let lineHeight = drawDecay ? height * (i + 1) / Math.max(1, events.length) : height;
            let y = drawDecay ? (height - lineHeight) / 2 : 0;
            ctx.fillStyle = root.colorString(lineColor, alpha);
            ctx.fillRect(x, y, lineWidth, lineHeight);
        }
    }

    visible: !!currentState
        && (currentState.a === undefined ? 255 : currentState.a) > 0
        && !!srcData
        && !!score

    Canvas {
        id: chartCanvas
        x: root.drawX
        y: root.drawY
        width: Math.max(1, root.drawW * root.scaleOverride)
        height: Math.max(1, root.drawH * root.scaleOverride)
        visible: root.visible
        opacity: root.currentState ? Math.max(0, Math.min(1, (root.currentState.a || 255) / 255.0)) : 0
        renderTarget: Canvas.Image
        renderStrategy: Canvas.Threaded
        antialiasing: false

        onAvailableChanged: root.requestChartPaint()
        onWidthChanged: root.requestChartPaint()
        onHeightChanged: root.requestChartPaint()

        onPaint: {
            let ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            if (!root.srcData || !root.score || width <= 0 || height <= 0) {
                return;
            }
            root.drawJudgeBackground(ctx, width, height);
            root.drawRecentLines(ctx, width, height);
        }
    }

    function requestChartPaint() : void {
        if (root.visible && chartCanvas.available) {
            chartCanvas.requestPaint();
        }
    }

    onChartChanged: requestChartPaint()
    onChartDataChanged: requestChartPaint()
    onScoreChanged: requestChartPaint()
    onHitEventsChanged: requestChartPaint()
    onRecentEventsChanged: requestChartPaint()
    onSrcDataChanged: requestChartPaint()
    onCurrentStateChanged: requestChartPaint()
    onSourceWChanged: requestChartPaint()
    onVisibleChanged: requestChartPaint()
    Component.onCompleted: requestChartPaint()
}
