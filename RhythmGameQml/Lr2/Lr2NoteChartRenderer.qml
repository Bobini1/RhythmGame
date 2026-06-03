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
    property string chartRevision: ""
    property real scaleOverride: 1.0
    property string cachedDataRevision: ""
    property var cachedDensityData: []
    property int cachedMaxDensity: 20
    property int cachedSourceW: 5
    property int cachedSourceH: 100
    readonly property var snapshotChart: ({
        chartData: chart && chart.chartData !== undefined ? chart.chartData : chart,
        revision: chartRevision
    })
    property Lr2ChartDataSnapshot chartSnapshot: Lr2ChartDataSnapshot {
        chart: root.snapshotChart
    }

    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    property Lr2TimelineState timelineState: Lr2TimelineState {
        id: timelineTracker

        enabled: !timelineTracker.canUseStaticState
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
        || timelineState.state
    readonly property int fieldW: Math.max(1, srcData ? (srcData.fieldW || 1) : 1)
    readonly property int fieldH: Math.max(1, srcData ? (srcData.fieldH || 1) : 1)
    readonly property real stateW: currentState ? (currentState.w || 0) : 0
    readonly property real stateH: currentState ? (currentState.h || 0) : 0
    readonly property real drawW: Math.abs(stateW) > 0 ? Math.abs(stateW) : fieldW
    readonly property real drawH: Math.abs(stateH) > 0 ? Math.abs(stateH) : fieldH
    readonly property real drawX: currentState
        ? (currentState.x + (stateW < 0 ? stateW : 0)) * scaleOverride
        : 0
    readonly property real drawY: currentState
        ? (currentState.y + (Math.abs(stateH) > 0 && stateH < 0 ? stateH : 0)
            - (Math.abs(stateH) > 0 ? 0 : fieldH)) * scaleOverride
        : 0
    readonly property real reveal: {
        if (!srcData || (srcData.delay || 0) <= 0) {
            return 1;
        }
        return Math.max(0, Math.min(1, effectiveSkinTime / Math.max(1, srcData.delay || 1)));
    }
    readonly property int basicNoteCount: chartSnapshot.normalNoteCount
        + chartSnapshot.scratchCount
        + chartSnapshot.lnCount
        + chartSnapshot.bssCount
        + chartSnapshot.mineCount
    readonly property bool hasChartData: chartSnapshot.hasHistogram || basicNoteCount > 0
    readonly property string dataRevision: chartSnapshot.revision.length > 0
        ? chartSnapshot.revision
        : (chartRevision
           + ":fallback:"
           + chartSnapshot.length
           + ":"
           + chartSnapshot.normalNoteCount
           + ":"
           + chartSnapshot.scratchCount
           + ":"
           + chartSnapshot.lnCount
           + ":"
           + chartSnapshot.bssCount
           + ":"
           + chartSnapshot.mineCount)
    readonly property int effectiveSkinTime: Lr2SkinUtils.skinTimeForClock(skinClock, skinClockMode, skinTime)

    function densityAt(series: var, index: var) : var {
        return series && index < series.length ? (Number(series[index]) || 0) : 0;
    }

    function noteColors() : var {
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

    function buildNormalData(histogram: var) : var {
        let normal = histogram && histogram[0] ? histogram[0] : [];
        let scratch = histogram && histogram[1] ? histogram[1] : [];
        let ln = histogram && histogram[2] ? histogram[2] : [];
        let bss = histogram && histogram[3] ? histogram[3] : [];
        let mine = histogram && histogram[4] ? histogram[4] : [];
        let count = Math.max(normal.length, scratch.length, ln.length, bss.length, mine.length);
        let data = new Array(count);
        for (let i = 0; i < count; ++i) {
            data[i] = [
                0,
                densityAt(bss, i),
                densityAt(scratch, i),
                0,
                densityAt(ln, i),
                densityAt(normal, i),
                densityAt(mine, i)
            ];
        }
        return data;
    }

    function spreadCount(data: var, total: var, slot: var) : void {
        total = Math.max(0, Math.floor(total || 0));
        if (total <= 0 || !data || data.length <= 0) {
            return;
        }

        const base = Math.floor(total / data.length);
        let remainder = total - base * data.length;
        for (let i = 0; i < data.length; ++i) {
            data[i][slot] += base + (remainder > 0 ? 1 : 0);
            if (remainder > 0) {
                --remainder;
            }
        }
    }

    function buildFallbackData() : var {
        const seconds = Math.max(1, Math.ceil(Math.max(0, chartSnapshot.length || 0) / 1000000000));
        const bucketCount = Math.max(1, Math.min(240, seconds));
        let data = new Array(bucketCount);
        for (let i = 0; i < bucketCount; ++i) {
            data[i] = [0, 0, 0, 0, 0, 0, 0];
        }

        spreadCount(data, chartSnapshot.bssCount, 1);
        spreadCount(data, chartSnapshot.scratchCount, 2);
        spreadCount(data, chartSnapshot.lnCount, 4);
        spreadCount(data, chartSnapshot.normalNoteCount, 5);
        spreadCount(data, chartSnapshot.mineCount, 6);
        return data;
    }

    function drawBars(ctx: var, data: var, maxDensity: var, sourceH: var) : void {
        let colors = noteColors();
        let noGap = srcData && (srcData.noGap || 0) === 1;
        let reverse = srcData && (srcData.orderReverse || 0) === 1;
        for (let i = 0; i < data.length; ++i) {
            let n = data[i];
            let yUnits = 0;
            let start = reverse ? n.length - 1 : 0;
            let end = reverse ? -1 : n.length;
            let step = reverse ? -1 : 1;
            for (let index = start; index !== end && yUnits < maxDensity; index += step) {
                ctx.fillStyle = colors[index] || "#cccccc";
                let amount = Math.max(0, Math.floor(n[index] || 0));
                for (let k = 0; k < amount && yUnits < maxDensity; ++k) {
                    ctx.fillRect(i * 5, sourceH - (yUnits + 1) * 5, 4, noGap ? 5 : 4);
                    ++yUnits;
                }
            }
        }
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

    function updateCachedGraphData() : var {
        if (cachedDataRevision === dataRevision) {
            return;
        }

        cachedDataRevision = dataRevision;
        cachedDensityData = chartSnapshot.hasHistogram
            ? buildNormalData(chartSnapshot.histogramData)
            : buildFallbackData();
        let bucketCount = Math.max(1, cachedDensityData.length);
        cachedMaxDensity = graphMax(cachedDensityData);
        cachedSourceW = bucketCount * 5;
        cachedSourceH = cachedMaxDensity * 5;
    }

    visible: !!currentState
        && (currentState.a === undefined ? 255 : currentState.a) > 0
        && !!srcData
        && hasChartData

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
            if (!root.hasChartData || !root.srcData || width <= 0 || height <= 0) {
                return;
            }

            root.updateCachedGraphData();
            let data = root.cachedDensityData;
            let bucketCount = Math.max(1, data.length);
            let maxDensity = root.cachedMaxDensity;
            let sourceW = root.cachedSourceW;
            let sourceH = root.cachedSourceH;

            ctx.save();
            ctx.scale(width / Math.max(1, sourceW), height / Math.max(1, sourceH));
            root.drawBackground(ctx, sourceW, sourceH, maxDensity, bucketCount);
            ctx.beginPath();
            ctx.rect(0, 0, sourceW * root.reveal, sourceH);
            ctx.clip();
            root.drawBars(ctx, data, maxDensity, sourceH);
            ctx.restore();
        }
    }

    function requestChartPaint() : void {
        if (root.visible && chartCanvas.available) {
            chartCanvas.requestPaint();
        }
    }

    onChartChanged: requestChartPaint()
    onHasChartDataChanged: requestChartPaint()
    onDataRevisionChanged: requestChartPaint()
    onSrcDataChanged: requestChartPaint()
    onCurrentStateChanged: requestChartPaint()
    onRevealChanged: requestChartPaint()
    onVisibleChanged: requestChartPaint()
    Component.onCompleted: requestChartPaint()
}
