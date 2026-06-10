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
    readonly property int lineWidthUnits: Math.max(1, Math.min(fieldW, srcData ? (srcData.lineWidth || 1) : 1))
    readonly property int sourceW: Math.max(1, Math.floor(fieldW / lineWidthUnits))
    readonly property int center: Math.floor(sourceW / 2)
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

    function chartDstExtent(fieldSize: var, stateSize: var) : var {
        let size = Math.abs(Number(stateSize || 0));
        if (size <= 0) {
            return fieldSize;
        }
        return size <= 4 ? fieldSize * size : size;
    }

    function chartWithTimingData(value: var) : var {
        return value && value.timingWindows !== undefined ? value : null;
    }

    readonly property var chartData: {
        if (!chart) {
            return null;
        }
        return chartWithTimingData(chart.chartData) || chartWithTimingData(chart);
    }
    readonly property var resultEvents: score && score.replayData
        ? (score.replayData.hitEvents || [])
        : []
    readonly property var timingData: buildTimingData()
    readonly property int sourceH: Math.max(10, timingData.maxCount || 10)

    function skinColor(value: var, fallback: var) : var {
        let raw = value === undefined || value === null ? "" : String(value);
        raw = raw.replace(/[^0-9a-fA-F]/g, "");
        if (raw.length >= 8) {
            let r = parseInt(raw.substring(0, 2), 16);
            let g = parseInt(raw.substring(2, 4), 16);
            let b = parseInt(raw.substring(4, 6), 16);
            let a = parseInt(raw.substring(6, 8), 16) / 255.0;
            return "rgba(" + r + "," + g + "," + b + "," + a + ")";
        }
        if (raw.length >= 6) {
            return "#" + raw.substring(0, 6);
        }
        return fallback;
    }

    function clamp(value: var, low: var, high: var) : var {
        return Math.max(low, Math.min(high, value));
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
            Object.assign({ color: root.skinColor(root.srcData ? root.srcData.pgColor : "", "#000088") },
                timingWindowFor("Perfect", 21)),
            Object.assign({ color: root.skinColor(root.srcData ? root.srcData.grColor : "", "#008800") },
                timingWindowFor("Great", 60)),
            Object.assign({ color: root.skinColor(root.srcData ? root.srcData.gdColor : "", "#888800") },
                timingWindowFor("Good", 120)),
            Object.assign({ color: root.skinColor(root.srcData ? root.srcData.bdColor : "", "#880000") },
                timingWindowFor("Bad", 200)),
            { low: -root.center, high: root.sourceW - root.center - 1,
                color: root.skinColor(root.srcData ? root.srcData.prColor : "", "#000000") }
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

    function timingDistributionJudgement(judgement: var) : var {
        return judgement === Judgement.Poor
            || (judgement >= Judgement.Bad && judgement <= Judgement.Perfect);
    }

    function emptyDistribution(size: var) : var {
        let dist = new Array(size);
        for (let i = 0; i < size; ++i) {
            dist[i] = 0;
        }
        return dist;
    }

    function buildTimingData() : var {
        let displayDist = root.emptyDistribution(root.sourceW);
        let fullDist = root.emptyDistribution(301);
        let count = 0;
        let sum = 0;
        let sumSq = 0;
        let events = root.resultEvents || [];
        for (let i = 0; i < events.length; ++i) {
            let hit = events[i];
            if (!hit || !hit.noteRemoved) {
                continue;
            }
            let judgement = root.judgementForHit(hit);
            if (!root.timingDistributionJudgement(judgement)) {
                continue;
            }
            let ms = root.timingMillis(hit);
            if (ms < -150 || ms > 150) {
                continue;
            }
            ++count;
            sum += ms;
            sumSq += ms * ms;
            fullDist[ms + 150] = (fullDist[ms + 150] || 0) + 1;

            let displayIndex = root.center + ms;
            if (displayIndex >= 0 && displayIndex < displayDist.length) {
                displayDist[displayIndex] = (displayDist[displayIndex] || 0) + 1;
            }
        }

        let maxCount = 10;
        for (let j = 0; j < fullDist.length; ++j) {
            if (fullDist[j] > maxCount) {
                maxCount = Math.floor(fullDist[j] / 10) * 10 + 10;
            }
        }

        let average = 0;
        let stddev = -1;
        if (count > 0) {
            average = sum / count;
            stddev = Math.sqrt(Math.max(0, sumSq / count - average * average));
        }

        return {
            dist: displayDist,
            maxCount: maxCount,
            average: average,
            stddev: stddev,
            hasAverage: count > 0,
            hasStddev: count > 0
        };
    }

    function drawJudgeBackground(ctx: var, width: var, height: var) : void {
        let areas = root.judgeAreas();
        ctx.fillStyle = areas[0].color;
        ctx.fillRect(root.center, 0, 1, height);

        let beforeLeft = root.center;
        let beforeRight = root.center + 1;
        let lowBound = -root.center;
        let highBound = root.sourceW - root.center - 1;
        for (let i = 0; i < areas.length; ++i) {
            let area = areas[i];
            let x1 = root.center + root.clamp(Math.round(area.low), lowBound, highBound);
            let x2 = root.center + root.clamp(Math.round(area.high), lowBound, highBound) + 1;
            x1 = root.clamp(x1, 0, width);
            x2 = root.clamp(x2, 0, width);
            ctx.fillStyle = area.color;
            if (beforeLeft > x1) {
                ctx.fillRect(x1, 0, beforeLeft - x1, height);
                beforeLeft = x1;
            }
            if (x2 > beforeRight) {
                ctx.fillRect(beforeRight, 0, x2 - beforeRight, height);
                beforeRight = x2;
            }
        }

        ctx.fillStyle = "rgba(0,0,0,0.25)";
        for (let x = root.center % 10; x < width; x += 10) {
            ctx.fillRect(x, 0, 1, 1);
        }
    }

    function drawVerticalMarker(ctx: var, offsetMs: var, color: var, height: var) : void {
        let x = root.center + Math.round(offsetMs);
        if (x < 0 || x >= root.sourceW) {
            return;
        }
        ctx.fillStyle = color;
        ctx.fillRect(x, 0, 1, height);
    }

    function drawHistogram(ctx: var, data: var, height: var) : void {
        let dist = data.dist || [];
        ctx.fillStyle = root.skinColor(root.srcData ? root.srcData.graphColor : "", "#00ff00");
        for (let x = 0; x < dist.length; ++x) {
            let value = Math.max(0, dist[x] || 0);
            if (value <= 0) {
                continue;
            }
            ctx.fillRect(x, Math.max(0, height - value), 1, value);
        }
    }

    visible: !!currentState
        && (currentState.a === undefined ? 255 : currentState.a) > 0
        && !!srcData
        && !!chartData
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
            if (!root.srcData || !root.chartData || !root.score || width <= 0 || height <= 0) {
                return;
            }

            let sourceW = root.sourceW;
            let sourceH = root.sourceH;
            let data = root.timingData;

            ctx.save();
            ctx.scale(width / sourceW, height / sourceH);
            root.drawJudgeBackground(ctx, sourceW, sourceH);

            if ((root.srcData.drawAverage || 0) === 1 && data.hasAverage) {
                root.drawVerticalMarker(
                    ctx,
                    data.average,
                    root.skinColor(root.srcData.averageColor, "#ffffff"),
                    sourceH);
            }

            if ((root.srcData.drawDev || 0) === 1 && data.hasStddev) {
                let devColor = root.skinColor(root.srcData.devColor, "#ffffff");
                root.drawVerticalMarker(ctx, data.average + data.stddev, devColor, sourceH);
                root.drawVerticalMarker(ctx, data.average - data.stddev, devColor, sourceH);
            }

            root.drawHistogram(ctx, data, sourceH);
            ctx.restore();
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
    onResultEventsChanged: requestChartPaint()
    onTimingDataChanged: requestChartPaint()
    onSrcDataChanged: requestChartPaint()
    onCurrentStateChanged: requestChartPaint()
    onSourceWChanged: requestChartPaint()
    onSourceHChanged: requestChartPaint()
    onVisibleChanged: requestChartPaint()
    Component.onCompleted: requestChartPaint()
}
