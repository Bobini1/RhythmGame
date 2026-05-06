import QtQuick
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property var chart
    property real scaleOverride: 1.0
    property string cachedDataRevision: ""
    property var cachedDensityData: []
    property int cachedMaxDensity: 20
    property int cachedSourceW: 5
    property int cachedSourceH: 100

    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.dsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptions: root.timelineActiveOptions
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
        return Math.max(0, Math.min(1, skinTime / Math.max(1, srcData.delay || 1)));
    }
    function chartWithHistogram(value) {
        return value
            && value.histogramData !== undefined
            && value.histogramData !== null
                ? value
                : null;
    }

    function histogramRevision(histogram) {
        if (!histogram) {
            return "";
        }
        let parts = [];
        for (let i = 0; i < 5; ++i) {
            let series = histogram[i] || [];
            parts.push(series.length || 0);
        }
        return parts.join(":");
    }

    readonly property var chartData: {
        if (!chart) {
            return null;
        }
        return chartWithHistogram(chart.chartData) || chartWithHistogram(chart);
    }
    readonly property string dataRevision: chartData
        ? (String(chartData.md5 || "")
           + ":" + String(chartData.length || 0)
           + ":" + String(chartData.normalNoteCount || 0)
           + ":" + String(chartData.scratchCount || 0)
           + ":" + String(chartData.lnCount || 0)
           + ":" + String(chartData.bssCount || 0)
           + ":" + String(chartData.mineCount || 0)
           + ":" + histogramRevision(chartData.histogramData))
        : ""

    function densityAt(series, index) {
        return series && index < series.length ? (Number(series[index]) || 0) : 0;
    }

    function hexColor(value, fallback) {
        let raw = value === undefined || value === null ? "" : String(value);
        raw = raw.replace(/[^0-9a-fA-F]/g, "");
        if (raw.length < 6) {
            return fallback;
        }
        return "#" + raw.substring(0, 6);
    }

    function noteColors() {
        return [
            "#44ff44", "#228822", "#ff4444", "#4444ff",
            "#222288", "#cccccc", "#880000"
        ];
    }

    function drawBackground(ctx, sourceW, sourceH, maxDensity, bucketCount) {
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

    function buildNormalData(histogram) {
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

    function drawBars(ctx, data, maxDensity, sourceH) {
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

    function graphMax(data) {
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

    function updateCachedGraphData() {
        if (cachedDataRevision === dataRevision) {
            return;
        }

        cachedDataRevision = dataRevision;
        cachedDensityData = buildNormalData(chartData ? (chartData.histogramData || []) : []);
        let bucketCount = Math.max(1, cachedDensityData.length);
        cachedMaxDensity = graphMax(cachedDensityData);
        cachedSourceW = bucketCount * 5;
        cachedSourceH = cachedMaxDensity * 5;
    }

    visible: !!currentState
        && (currentState.a === undefined ? 255 : currentState.a) > 0
        && !!srcData
        && !!chartData

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
            if (!root.chartData || !root.srcData || width <= 0 || height <= 0) {
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

    function requestChartPaint() {
        if (root.visible && chartCanvas.available) {
            chartCanvas.requestPaint();
        }
    }

    onChartChanged: requestChartPaint()
    onChartDataChanged: requestChartPaint()
    onDataRevisionChanged: requestChartPaint()
    onSrcDataChanged: requestChartPaint()
    onCurrentStateChanged: requestChartPaint()
    onRevealChanged: requestChartPaint()
    onVisibleChanged: requestChartPaint()
    Component.onCompleted: requestChartPaint()
}
