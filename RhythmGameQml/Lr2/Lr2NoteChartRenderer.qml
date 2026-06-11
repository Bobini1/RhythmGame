import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

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
    property Lr2ChartDataSnapshot chartSnapshot: Lr2ChartDataSnapshot {
        chart: root.chart && root.chart.chartData !== undefined ? root.chart.chartData : root.chart
    }

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
    readonly property real reveal: {
        if (!srcData || (srcData.delay || 0) <= 0) {
            return 1;
        }
        return Math.max(0, Math.min(1, effectiveSkinTime / Math.max(1, srcData.delay || 1)));
    }
    readonly property int chartType: srcData ? Math.max(0, Math.min(2, srcData.chartType || 0)) : 0
    readonly property var chartData: root.chart && root.chart.chartData !== undefined ? root.chart.chartData : root.chart
    readonly property bool hasChartData: chartType === 0
        ? chartSnapshot.hasHistogram
        : (chartSnapshot.hasHistogram || resultEvents.length > 0)
    readonly property var resultEvents: score && score.replayData ? (score.replayData.hitEvents || []) : []
    readonly property var densityData: buildGraphData()
    readonly property int bucketCount: Math.max(1, densityData.length)
    readonly property int maxDensity: chartType === 0
        ? chartSnapshot.normalDensityMax
        : graphMax(densityData)
    readonly property int sourceW: bucketCount * 5
    readonly property int sourceH: maxDensity * 5
    readonly property int effectiveSkinTime: Lr2SkinUtils.skinTimeForClock(skinClock, skinClockMode, skinTime)

    function chartDstExtent(fieldSize: var, stateSize: var) : var {
        let size = Math.abs(Number(stateSize || 0));
        if (size <= 0) {
            return fieldSize;
        }
        return size <= 4 ? fieldSize * size : size;
    }

    function graphColors() : var {
        if (root.chartType === 1) {
            return [
                "#555555", "#0088ff", "#00ff88", "#ffff00",
                "#ff8800", "#ff0000"
            ];
        }
        if (root.chartType === 2) {
            return [
                "#555555", "#44ff44", "#0088ff", "#0066cc",
                "#004488", "#002244", "#ff8800", "#cc6600",
                "#884400", "#442200"
            ];
        }
        return [
            "#44ff44", "#228822", "#ff4444", "#4444ff",
            "#222288", "#cccccc", "#880000"
        ];
    }

    function drawBackground(ctx: var, sourceW: var, sourceH: var, maxDensity: var, bucketCount: var) : var {
        if (srcData && (srcData.backTexOff || 0) === 1) {
            return;
        }

        ctx.fillStyle = "rgba(0,0,0,0.8)";
        ctx.fillRect(0, 0, sourceW, sourceH);

        for (let i = 10; i < maxDensity; i += 10) {
            let c = Math.max(0, Math.min(255, Math.round(255 * 0.007 * i)));
            ctx.fillStyle = "rgb(" + c + "," + c + ",0)";
            let bandTop = Math.max(0, sourceH - (i + 10) * 5);
            let bandBottom = Math.max(0, sourceH - i * 5);
            ctx.fillRect(0, bandTop, sourceW, Math.max(1, bandBottom - bandTop));
        }

        for (let i = 0; i < bucketCount; ++i) {
            if (i % 60 === 0) {
                ctx.fillStyle = "rgb(64,64,64)";
                ctx.fillRect(i * 5, 0, 1, sourceH);
            } else if (i % 10 === 0) {
                ctx.fillStyle = "rgb(32,32,32)";
                ctx.fillRect(i * 5, 0, 1, sourceH);
            }
        }
    }

    function chartLengthNanos() : var {
        let result = root.score && root.score.result ? root.score.result : null;
        let length = Math.max(0,
            Number(root.chartData && root.chartData.length !== undefined ? root.chartData.length : 0),
            Number(result && result.length !== undefined ? result.length : 0));
        let maxOffset = length > 0 ? length + 1000000000 : 0;
        let events = root.resultEvents || [];
        for (let i = 0; i < events.length; ++i) {
            let offset = root.replayEventOffset(events[i], maxOffset);
            if (offset > length) {
                length = offset;
            }
        }
        return length;
    }

    function replayEventOffset(hit: var, maxOffset: var) : var {
        if (!hit || !hit.noteRemoved) {
            return -1;
        }
        let offset = Number(hit.offsetFromStart !== undefined ? hit.offsetFromStart : -1);
        if (!isFinite(offset) || offset < 0) {
            return -1;
        }
        if (maxOffset > 0 && offset > maxOffset) {
            return -1;
        }
        return offset;
    }

    function emptyReplayBuckets(bucketSize: var) : var {
        let seconds = Math.max(1, Math.floor(root.chartLengthNanos() / 1000000000) + 1);
        let result = new Array(seconds);
        for (let i = 0; i < seconds; ++i) {
            result[i] = new Array(bucketSize);
            for (let j = 0; j < bucketSize; ++j) {
                result[i][j] = 0;
            }
        }
        return result;
    }

    function histogramBucketCount(histogram: var) : var {
        let count = 0;
        for (let i = 0; i < 4; ++i) {
            count = Math.max(count, histogram && histogram[i] ? histogram[i].length : 0);
        }
        return count;
    }

    function histogramPlayableCountAt(histogram: var, index: var) : var {
        let normal = histogram && histogram[0] ? histogram[0] : null;
        let scratch = histogram && histogram[1] ? histogram[1] : null;
        let ln = histogram && histogram[2] ? histogram[2] : null;
        let bss = histogram && histogram[3] ? histogram[3] : null;
        return (normal && index < normal.length ? (Number(normal[index]) || 0) : 0)
            + (scratch && index < scratch.length ? (Number(scratch[index]) || 0) : 0)
            + (ln && index < ln.length ? (Number(ln[index]) || 0) : 0)
            + (bss && index < bss.length ? (Number(bss[index]) || 0) : 0);
    }

    function baseReplayBuckets(bucketSize: var) : var {
        let histogram = root.chartSnapshot.hasHistogram ? root.chartSnapshot.histogramData : [];
        let histogramCount = root.histogramBucketCount(histogram);
        let seconds = Math.max(histogramCount, Math.floor(root.chartLengthNanos() / 1000000000) + 1, 1);
        let result = new Array(seconds);
        for (let i = 0; i < seconds; ++i) {
            result[i] = new Array(bucketSize);
            for (let j = 0; j < bucketSize; ++j) {
                result[i][j] = 0;
            }
            result[i][0] = root.histogramPlayableCountAt(histogram, i);
        }
        return result;
    }

    function eventSecond(hit: var, length: var) : var {
        let offset = root.replayEventOffset(hit, length > 0 ? length * 1000000000 : 0);
        if (offset < 0 || length <= 0) {
            return -1;
        }
        return Math.max(0, Math.min(length - 1, Math.floor(offset / 1000000000)));
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

    function judgeGraphBucket(judgement: var) : var {
        switch (judgement) {
        case Judgement.Perfect:
            return 1;
        case Judgement.Great:
            return 2;
        case Judgement.Good:
            return 3;
        case Judgement.Bad:
            return 4;
        case Judgement.Poor:
        case Judgement.EmptyPoor:
            return 5;
        default:
            return -1;
        }
    }

    function fastSlowGraphBucket(hit: var, judgement: var) : var {
        if (judgement === Judgement.Perfect) {
            return 1;
        }
        let base = 0;
        switch (judgement) {
        case Judgement.Great:
            base = 2;
            break;
        case Judgement.Good:
            base = 3;
            break;
        case Judgement.Bad:
            base = 4;
            break;
        case Judgement.Poor:
        case Judgement.EmptyPoor:
            base = 5;
            break;
        default:
            return -1;
        }
        return root.hitDeviationNanos(hit) < 0 ? base : base + 4;
    }

    function buildReplayData(type: var) : var {
        let data = root.baseReplayBuckets(type === 1 ? 6 : 10);
        let events = root.resultEvents || [];
        for (let i = 0; i < events.length; ++i) {
            let hit = events[i];
            let second = root.eventSecond(hit, data.length);
            if (second < 0) {
                continue;
            }
            let judgement = root.judgementForHit(hit);
            if (judgement === Judgement.EmptyPoor
                    || judgement < Judgement.Poor
                    || judgement > Judgement.Perfect) {
                continue;
            }
            let bucket = type === 1
                ? root.judgeGraphBucket(judgement)
                : root.fastSlowGraphBucket(hit, judgement);
            if (bucket < 0) {
                continue;
            }
            if (data[second][0] > 0) {
                data[second][0] = data[second][0] - 1;
            }
            data[second][bucket] = (data[second][bucket] || 0) + 1;
        }
        return data;
    }

    function buildGraphData() : var {
        if (root.chartType === 0) {
            return root.hasChartData ? root.chartSnapshot.normalDensityData : [];
        }
        return root.buildReplayData(root.chartType);
    }

    function drawBars(ctx: var, data: var, maxDensity: var, sourceH: var) : void {
        let colors = graphColors();
        let noGap = srcData && (srcData.noGap || 0) === 1;
        let noGapX = srcData && (srcData.noGapX || 0) === 1;
        let reverse = srcData && (srcData.orderReverse || 0) === 1;
        for (let i = 0; i < data.length; ++i) {
            let n = data[i];
            let yUnits = 0;
            let start = reverse ? n.length - 1 : 0;
            let end = reverse ? -1 : n.length;
            let step = reverse ? -1 : 1;
            for (let index = start; index !== end && yUnits < maxDensity; index += step) {
                ctx.fillStyle = colors[index] || "#cccccc";
                let amount = Math.min(maxDensity - yUnits, Math.max(0, Math.floor(n[index] || 0)));
                if (amount <= 0) {
                    continue;
                }
                if (noGap) {
                    ctx.fillRect(i * 5, sourceH - (yUnits + amount) * 5,
                        noGapX ? 5 : 4, amount * 5);
                    yUnits += amount;
                } else {
                    for (let k = 0; k < amount && yUnits < maxDensity; ++k) {
                        ctx.fillRect(i * 5, sourceH - (yUnits + 1) * 5, noGapX ? 5 : 4, 4);
                        ++yUnits;
                    }
                }
            }
        }
    }

    function gameplayElapsedNanos() : var {
        if (!root.screenRoot || !root.screenRoot.gameplayScreenActive || !root.screenRoot.gameplayPlayer) {
            return -1;
        }
        let side = root.srcData ? (root.srcData.playerSide || 1) : 1;
        let player = root.screenRoot.gameplayPlayer(side);
        return player ? Number(player.elapsed || 0) : -1;
    }

    function graphMax(data: var) : var {
        let maxValue = 20;
        for (let i = 0; i < data.length; ++i) {
            let total = 0;
            for (let j = 0; j < data[i].length; ++j) {
                total += data[i][j] || 0;
            }
            if (maxValue < total) {
                maxValue = Math.min(Math.floor(total / 10) * 10 + 10, 100);
            }
        }
        return maxValue;
    }

    visible: !!currentState
        && (currentState.a === undefined ? 255 : currentState.a) > 0
        && !!srcData
        && hasChartData

    Item {
        id: revealClip
        x: root.drawX
        y: root.drawY
        width: Math.max(0, root.drawW * root.scaleOverride * root.reveal)
        height: Math.max(1, root.drawH * root.scaleOverride)
        visible: root.visible
        clip: root.reveal < 1

        onHeightChanged: root.requestChartPaint()

        Canvas {
            id: chartCanvas
            width: Math.max(1, root.drawW * root.scaleOverride)
            height: revealClip.height
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
                if (!root.hasChartData || !root.srcData || width <= 0 || height <= 0) {
                    return;
                }

                let data = root.densityData;
                let bucketCount = root.bucketCount;
                let maxDensity = root.maxDensity;
                let sourceW = root.sourceW;
                let sourceH = root.sourceH;

                ctx.save();
                ctx.scale(width / Math.max(1, sourceW), height / Math.max(1, sourceH));
                root.drawBackground(ctx, sourceW, sourceH, maxDensity, bucketCount);
                root.drawBars(ctx, data, maxDensity, sourceH);
                ctx.restore();
            }
        }

        Rectangle {
            readonly property real elapsedNanos: root.gameplayElapsedNanos()
            readonly property real rawX: {
                if (elapsedNanos < 0 || root.bucketCount <= 0) {
                    return -1;
                }
                return elapsedNanos * chartCanvas.width / Math.max(1, root.bucketCount * 1000000000);
            }

            width: Math.max(1, 3 * chartCanvas.width / Math.max(1, root.sourceW))
            height: chartCanvas.height
            x: rawX < 0 ? 0 : Math.max(0, Math.min(chartCanvas.width - width, rawX))
            visible: root.visible && rawX >= 0
            opacity: chartCanvas.opacity
            color: "white"
            z: 1
        }
    }

    function requestChartPaint() : void {
        if (root.visible && chartCanvas.available) {
            chartCanvas.requestPaint();
        }
    }

    onChartChanged: requestChartPaint()
    onScoreChanged: requestChartPaint()
    onHasChartDataChanged: requestChartPaint()
    onDensityDataChanged: requestChartPaint()
    onSrcDataChanged: requestChartPaint()
    onVisibleChanged: requestChartPaint()
    Component.onCompleted: requestChartPaint()
}
