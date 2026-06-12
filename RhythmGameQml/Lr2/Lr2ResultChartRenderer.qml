import QtQuick
import RhythmGameQml

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property real scaleOverride: 1.0
    property var screenRoot
    property int paintedColumnCount: -1
    readonly property var emptyValueCacheState: ({
        values: [],
        fieldW: 1,
        fieldH: 1,
        step: 1,
        chartType: 0,
        chartIndex: 0,
        gaugeHard: false,
        gaugeBorderPercent: 80,
        gaugeColors: root.gaugeGraphColors(null)
    })
    readonly property int chartSide: root.srcData && root.srcData.side ? root.srcData.side : 1
    readonly property int chartIndex: root.srcData ? (root.srcData.resultChartIndex || 0) : 0
    readonly property int chartType: root.srcData ? (root.srcData.resultChartType || 0) : 0
    readonly property var resultScoreData: root.screenRoot ? root.screenRoot.resultScore(root.chartSide) : null
    readonly property var resultDataValue: root.resultScoreData && root.resultScoreData.result
        ? root.resultScoreData.result
        : null
    readonly property var chartScoreData: {
        if (!root.screenRoot || !root.srcData) {
            return null;
        }
        if (root.chartIndex === 0) {
            return root.resultScoreData;
        }
        if (root.chartIndex === 1 && root.screenRoot.resultOldBestScore) {
            return root.screenRoot.resultOldBestScore(root.chartSide);
        }
        if (root.chartIndex === 2 && root.screenRoot.resultTargetSavedScore) {
            return root.screenRoot.resultTargetSavedScore(root.chartSide);
        }
        return null;
    }

    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState && timelineState.staticState.valid
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.dsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }
    readonly property var currentState: staticTimelineState
        || (timelineState.hasState ? timelineState.state : null)
    readonly property bool cacheInputsReady: !!root.currentState && !!root.srcData
    readonly property int cacheInputFieldW: root.cacheInputsReady
        ? Math.max(1, root.srcData.op1 || Math.abs(root.currentState.w) || 1)
        : 1
    readonly property int cacheInputFieldH: root.cacheInputsReady
        ? Math.max(1, root.srcData.op2 || Math.abs(root.currentState.h) || 1)
        : 1
    readonly property int cacheInputStep: root.cacheInputsReady
        ? Math.max(1, Math.abs(root.currentState.w || root.srcData.w || 1))
        : 1
    readonly property var valueCacheState: root.buildValueCache()
    readonly property var valueCache: root.valueCacheState.values
    readonly property int cachedFieldW: root.valueCacheState.fieldW
    readonly property int cachedFieldH: root.valueCacheState.fieldH
    readonly property int cachedStep: root.valueCacheState.step
    readonly property int cachedChartType: root.valueCacheState.chartType
    readonly property int cachedChartIndex: root.valueCacheState.chartIndex
    readonly property bool cachedGaugeHard: root.valueCacheState.gaugeHard
    readonly property real cachedGaugeBorderPercent: root.valueCacheState.gaugeBorderPercent
    readonly property var cachedGaugeColors: root.valueCacheState.gaugeColors
    readonly property string resolvedSource: {
        if (!srcData || !srcData.source) return "";
        let absPath = srcData.source.replace(/\\/g, "/");
        if (/^[A-Za-z]:\//.test(absPath)) {
            return "file:///" + absPath;
        }
        if (absPath.startsWith("/")) {
            return "file://" + absPath;
        }
        return absPath;
    }
    property Lr2AnimationFrameState animationFrameState: Lr2AnimationFrameState {
        enabled: !!root.srcData
        sourceData: root.srcData
        skinTime: root.skinTime
        timers: root.timers
        timerFire: root.sourceTimerFire
    }
    readonly property int sourceFrameIndex: animationFrameState.frameIndex
    readonly property bool alphaDefaultsOpaque: {
        if (!srcData || (srcData.resultChartType || 0) <= 0 || !dsts || dsts.length === 0) {
            return false;
        }
        for (let i = 0; i < dsts.length; ++i) {
            if (((dsts[i] && dsts[i].a !== undefined ? dsts[i].a : 0) || 0) > 0) {
                return false;
            }
        }
        return true;
    }
    readonly property real effectiveAlpha: {
        if (!currentState) {
            return 0;
        }
        let alpha = currentState.a || 0;
        if (alpha <= 0 && alphaDefaultsOpaque) {
            alpha = 255;
        }
        return Math.max(0, Math.min(255, alpha));
    }

    function scoreChartReplayEvents(score: var) : var {
        return score && score.replayData ? (score.replayData.hitEvents || []) : [];
    }

    function gaugeInfo() : var {
        if (root.screenRoot && root.screenRoot.resultGaugeInfo) {
            return root.screenRoot.resultGaugeInfo(root.chartSide);
        }

        let score = root.resultScoreData;
        let infos = score && score.gaugeHistory ? score.gaugeHistory.gaugeInfo : [];
        if (!infos || infos.length === 0) {
            return null;
        }

        let best = null;
        let fallback = infos[infos.length - 1];
        for (let i = 0; i < infos.length; ++i) {
            let info = infos[i];
            let history = info && info.gaugeHistory ? info.gaugeHistory : [];
            let last = history && history.length > 0 ? history[history.length - 1].gauge : 0;
            if (!best && root.screenRoot && root.screenRoot.gaugeAboveThreshold(last, info.threshold)) {
                best = info;
            }
            if (info && !info.courseGauge) {
                fallback = info;
            }
        }
        return best || fallback;
    }

    function scoreChartFinalPoints() : var {
        let score = root.chartScoreData;
        let result = score && score.result ? score.result : null;
        if (result) {
            return result.points || 0;
        }
        if (screenRoot && srcData && root.chartIndex === 2 && screenRoot.resultTargetPoints) {
            return screenRoot.resultTargetPoints(root.chartSide);
        }
        return -1;
    }

    function graphColorComponent(state: var, name: var) : var {
        return Math.max(0, Math.min(255, state && state[name] !== undefined ? state[name] : 255));
    }

    function graphColorComponents(state: var) : var {
        let color = [
            graphColorComponent(state, "r"),
            graphColorComponent(state, "g"),
            graphColorComponent(state, "b")
        ];
        if (root.cachedChartType === 1
                && color[0] === 255 && color[1] === 255 && color[2] === 255) {
            return root.cachedChartIndex === 0 ? [0, 255, 0] : [255, 0, 0];
        }
        return color;
    }

    function graphColor(state: var) : var {
        let color = graphColorComponents(state);
        return "rgb(" + color[0] + "," + color[1] + "," + color[2] + ")";
    }

    function graphNeedsTint(state: var) : var {
        let color = graphColorComponents(state);
        return color[0] !== 255 || color[1] !== 255 || color[2] !== 255;
    }

    function drawTintedImage(ctx: var, sx: var, sy: var, srcW: var, srcH: var, dx: var, dy: var, dw: var, dh: var) : var {
        ctx.drawImage(root.resolvedSource, sx, sy, srcW, srcH, dx, dy, dw, dh);
        if (!root.graphNeedsTint(root.currentState)) {
            return;
        }

        ctx.save();
        ctx.globalAlpha = 1.0;
        ctx.globalCompositeOperation = "multiply";
        ctx.fillStyle = root.graphColor(root.currentState);
        ctx.fillRect(dx, dy, dw, dh);
        ctx.globalCompositeOperation = "destination-in";
        ctx.drawImage(root.resolvedSource, sx, sy, srcW, srcH, dx, dy, dw, dh);
        ctx.restore();
    }

    function cacheValueCount(fieldW: var, step: var) : var {
        return Math.max(1, Math.ceil(Math.max(1, fieldW) / Math.max(1, step)));
    }

    function clampPercent(value: var) : var {
        return Math.max(0, Math.min(100, value));
    }

    function scorePercent(points: var, maxPoints: var) : var {
        return clampPercent(points * 100 / Math.max(1, maxPoints || 1));
    }

    function buildLinearScoreCache(fieldW: var, step: var, finalPoints: var, maxPoints: var) : var {
        let count = cacheValueCount(fieldW, step);
        let result = new Array(count);
        for (let i = 0; i < count; ++i) {
            result[i] = scorePercent(finalPoints * (i * step / fieldW), maxPoints);
        }
        return result;
    }

    function buildReplayScoreCache(score: var, fieldW: var, step: var, maxPoints: var) : var {
        let scoreResult = score && score.result ? score.result : null;
        let events = scoreChartReplayEvents(score);
        if (!scoreResult || !events || events.length === 0) {
            return [];
        }

        let count = cacheValueCount(fieldW, step);
        let result = new Array(count);
        let length = Math.max(1, scoreResult.length || 1);
        let points = 0;
        let eventIndex = 0;
        for (let i = 0; i < count; ++i) {
            let targetTime = (i * step / fieldW) * length;
            while (eventIndex < events.length) {
                let event = events[eventIndex];
                if (event && (event.offsetFromStart || 0) > targetTime) {
                    break;
                }
                if (event && event.noteRemoved && event.points && event.points.value !== undefined) {
                    points += event.points.value || 0;
                }
                ++eventIndex;
            }
            result[i] = scorePercent(points, maxPoints);
        }
        return result;
    }

    function buildGradeTargetCache(fieldW: var, step: var, maxPoints: var) : var {
        if (!screenRoot || !srcData || root.chartIndex !== 2
                || !screenRoot.resultTargetFraction || root.chartScoreData) {
            return [];
        }

        let currentScore = root.resultScoreData;
        let currentResult = currentScore && currentScore.result ? currentScore.result : null;
        let events = scoreChartReplayEvents(currentScore);
        if (!currentResult || !events || events.length === 0) {
            return [];
        }

        let count = cacheValueCount(fieldW, step);
        let result = new Array(count);
        let length = Math.max(1, currentResult.length || 1);
        let points = 0;
        let eventIndex = 0;
        let perNotePoints = 2.0 * Math.max(0, screenRoot.resultTargetFraction(root.chartSide) || 0);
        for (let i = 0; i < count; ++i) {
            let targetTime = (i * step / fieldW) * length;
            while (eventIndex < events.length) {
                let event = events[eventIndex];
                if (event && (event.offsetFromStart || 0) > targetTime) {
                    break;
                }
                if (event && event.noteRemoved && event.points) {
                    points += perNotePoints;
                }
                ++eventIndex;
            }
            result[i] = scorePercent(points, maxPoints);
        }
        return result;
    }

    function buildGaugeCache(fieldW: var, step: var) : var {
        let info = gaugeInfo();
        let history = info && info.gaugeHistory ? info.gaugeHistory : [];
        let maxGauge = info ? Math.max(1, info.maxGauge || 100) : 100;
        let borderPercent = root.clampPercent((info ? Number(info.threshold || 0) : 80) * 100 / maxGauge);
        let colors = root.gaugeGraphColors(info);
        let gaugeName = info ? String(info.name).toUpperCase() : "";
        let gaugeHard = gaugeName.indexOf("HARD") !== -1
            || gaugeName.indexOf("DAN") !== -1
            || gaugeName.indexOf("FC") !== -1
            || gaugeName.indexOf("PERFECT") !== -1
            || gaugeName.indexOf("MAX") !== -1;

        if (!history || history.length === 0) {
            return {
                values: [],
                gaugeHard: gaugeHard,
                borderPercent: borderPercent,
                colors: colors
            };
        }

        let count = cacheValueCount(fieldW, step);
        let result = new Array(count);
        let currentResult = root.resultDataValue;
        let length = currentResult ? Math.max(1, currentResult.length || 1) : 1;
        let value = history[0].gauge !== undefined ? history[0].gauge : 0;
        let historyIndex = 0;
        for (let i = 0; i < count; ++i) {
            let targetTime = (i * step / fieldW) * length;
            while (historyIndex < history.length) {
                let entry = history[historyIndex];
                if (entry && (entry.offsetFromStart || 0) > targetTime) {
                    break;
                }
                value = entry && entry.gauge !== undefined ? entry.gauge : value;
                ++historyIndex;
            }
            result[i] = clampPercent(value * 100 / maxGauge);
        }
        return {
            values: result,
            gaugeHard: gaugeHard,
            borderPercent: borderPercent,
            colors: colors
        };
    }

    function buildScoreCache(fieldW: var, step: var) : var {
        let current = root.resultDataValue;
        let maxPoints = current ? Math.max(1, current.maxPoints || 1) : 1;
        let score = root.chartScoreData;
        let replayValues = buildReplayScoreCache(score, fieldW, step, maxPoints);
        if (replayValues.length > 0) {
            return replayValues;
        }

        let gradeValues = buildGradeTargetCache(fieldW, step, maxPoints);
        if (gradeValues.length > 0) {
            return gradeValues;
        }

        let finalPoints = scoreChartFinalPoints();
        if (finalPoints < 0) {
            return [];
        }
        return buildLinearScoreCache(fieldW, step, finalPoints, maxPoints);
    }

    function buildValueCache() : var {
        if (!root.cacheInputsReady) {
            return root.emptyValueCacheState;
        }

        let fieldW = root.cacheInputFieldW;
        let fieldH = root.cacheInputFieldH;
        let step = root.cacheInputStep;
        let gaugeCache = root.chartType === 1
            ? buildGaugeCache(fieldW, step)
            : null;
        return {
            values: gaugeCache ? gaugeCache.values : buildScoreCache(fieldW, step),
            fieldW: fieldW,
            fieldH: fieldH,
            step: step,
            chartType: root.chartType,
            chartIndex: root.chartIndex,
            gaugeHard: gaugeCache ? gaugeCache.gaugeHard : false,
            gaugeBorderPercent: gaugeCache ? gaugeCache.borderPercent : 80,
            gaugeColors: gaugeCache ? gaugeCache.colors : root.gaugeGraphColors(null)
        };
    }

    function cachedSegmentVisible(value: var) : var {
        if (root.cachedChartType !== 1) {
            return true;
        }
        if (root.cachedGaugeHard) {
            return root.cachedChartIndex === 1;
        }
        return root.cachedChartIndex === 0 ? value < 80 : value >= 80;
    }

    function drawColumnCount() : var {
        if (!root.currentState || !root.srcData || root.valueCache.length === 0) {
            return 0;
        }
        let start = Math.max(0, root.srcData.op3 || 0);
        let end = Math.max(start + 1, root.srcData.op4 || start + 1);
        let elapsed = Math.max(0, Math.min(end - start, root.skinTime - start));
        let drawLength = Math.max(0, Math.min(root.cachedFieldW, root.cachedFieldW * elapsed / (end - start)));
        return Math.min(root.valueCache.length, Math.ceil(drawLength / root.cachedStep));
    }

    function requestChartPaint() : void {
        if (chartCanvas.available) {
            chartCanvas.requestPaint();
        }
    }

    function requestTimedChartPaint() : var {
        let count = drawColumnCount();
        if (count === root.paintedColumnCount) {
            return;
        }
        root.paintedColumnCount = count;
        requestChartPaint();
    }

    function drawGraphPoint(ctx: var, imageLoaded: var, sx: var, sy: var, srcW: var, srcH: var, x: var, value: var, dstW: var, dstH: var, fieldH: var) : var {
        if (value < 0 || !root.cachedSegmentVisible(value)) {
            return;
        }
        let offsetY = Math.trunc(-fieldH * Math.max(0, Math.min(100, value)) / 100.0);
        let dx = (root.currentState.x + x) * root.scaleOverride;
        let dy = (root.currentState.y + offsetY) * root.scaleOverride;
        let dw = Math.max(1, dstW * root.scaleOverride);
        let dh = Math.max(1, dstH * root.scaleOverride);
        if (imageLoaded) {
            root.drawTintedImage(ctx, sx, sy, srcW, srcH, dx, dy, dw, dh);
        } else {
            ctx.fillRect(dx, dy, dw, dh);
        }
    }

    function logicalFillRect(ctx: var, x: var, y: var, w: var, h: var) : void {
        ctx.fillRect(
            (root.currentState.x + x) * root.scaleOverride,
            (root.currentState.y + y) * root.scaleOverride,
            Math.max(1, w * root.scaleOverride),
            Math.max(1, h * root.scaleOverride));
    }

    function gaugeGraphY(value: var, fieldH: var, lineWidth: var) : var {
        return Math.round(-root.clampPercent(value) * Math.max(1, fieldH - lineWidth) / 100.0);
    }

    function gaugeGraphColors(info: var) : var {
        let name = info ? String(info.name || "").toUpperCase() : "";
        if (name.indexOf("AEASY") !== -1 || name.indexOf("ASSIST") !== -1) {
            return { graphLine: "#ff00ff", graphBg: "#440044", borderLine: "#ff0000", borderBg: "#440000" };
        }
        if (name.indexOf("EASY") !== -1) {
            return { graphLine: "#00ffff", graphBg: "#004444", borderLine: "#ff0000", borderBg: "#440000" };
        }
        if (name.indexOf("EXHARD") !== -1 || name.indexOf("EXDAN") !== -1) {
            return { graphLine: "#ffff00", graphBg: "#444400", borderLine: "#ffff00", borderBg: "#444400" };
        }
        if (name.indexOf("FC") !== -1 || name.indexOf("PERFECT") !== -1 || name.indexOf("MAX") !== -1) {
            return { graphLine: "#cccccc", graphBg: "#444444", borderLine: "#cccccc", borderBg: "#444444" };
        }
        if (name.indexOf("HARD") !== -1 || name.indexOf("DAN") !== -1) {
            return { graphLine: "#ff0000", graphBg: "#440000", borderLine: "#ff0000", borderBg: "#440000" };
        }
        return { graphLine: "#00ff00", graphBg: "#004400", borderLine: "#ff0000", borderBg: "#440000" };
    }

    function drawGaugeGraphBackground(ctx: var, fieldW: var, fieldH: var, borderPercent: var, colors: var) : void {
        let topHeight = fieldH * (100 - root.clampPercent(borderPercent)) / 100.0;
        ctx.fillStyle = colors.graphBg;
        root.logicalFillRect(ctx, 0, -fieldH, fieldW, fieldH);
        if (topHeight > 0) {
            ctx.fillStyle = colors.borderBg;
            root.logicalFillRect(ctx, 0, -fieldH, fieldW, topHeight);
        }
    }

    function drawGaugeGraphSegment(ctx: var, x1: var, y1: var, x2: var, y2: var, lineWidth: var) : void {
        root.logicalFillRect(ctx, x1, Math.min(y1, y2), lineWidth, Math.abs(y2 - y1) + lineWidth);
        root.logicalFillRect(ctx, x1, y2, Math.max(lineWidth, x2 - x1 + lineWidth), lineWidth);
    }

    function drawGaugeGraphVertical(ctx: var, x: var, y1: var, y2: var, lineWidth: var) : void {
        root.logicalFillRect(ctx, x, Math.min(y1, y2), lineWidth, Math.abs(y2 - y1) + lineWidth);
    }

    function drawGaugeGraph(ctx: var, values: var, columnCount: var, fieldW: var, fieldH: var, step: var) : void {
        let lineWidth = Math.max(1, Math.abs(root.srcData && root.srcData.h ? root.srcData.h : 1));
        let borderPercent = root.cachedGaugeBorderPercent;
        let borderY = root.gaugeGraphY(borderPercent, fieldH, lineWidth);
        let colors = root.cachedGaugeColors || root.gaugeGraphColors(null);
        root.drawGaugeGraphBackground(ctx, fieldW, fieldH, borderPercent, colors);
        if (!values || values.length === 0 || columnCount <= 0) {
            return;
        }

        let previousValue = values[0];
        let previousX = 0;
        let previousY = root.gaugeGraphY(previousValue, fieldH, lineWidth);
        let lastX = previousX;
        let lastY = previousY;
        let lastGauge = -1;

        for (let column = 1; column < columnCount; ++column) {
            let value = values[column];
            if (value < 0 || previousValue < 0) {
                previousValue = value;
                previousX = column * step;
                previousY = root.gaugeGraphY(value, fieldH, lineWidth);
                continue;
            }

            let x = column * step;
            let y = root.gaugeGraphY(value, fieldH, lineWidth);
            if (previousValue < borderPercent) {
                if (value < borderPercent) {
                    ctx.fillStyle = colors.graphLine;
                    root.drawGaugeGraphSegment(ctx, previousX, previousY, x, y, lineWidth);
                } else {
                    ctx.fillStyle = colors.graphLine;
                    root.drawGaugeGraphVertical(ctx, previousX, previousY, borderY, lineWidth);
                    ctx.fillStyle = colors.borderLine;
                    root.drawGaugeGraphVertical(ctx, previousX, borderY, y, lineWidth);
                    root.logicalFillRect(ctx, previousX, y, Math.max(lineWidth, x - previousX + lineWidth), lineWidth);
                }
            } else {
                if (value >= borderPercent) {
                    ctx.fillStyle = colors.borderLine;
                    root.drawGaugeGraphSegment(ctx, previousX, previousY, x, y, lineWidth);
                } else {
                    ctx.fillStyle = colors.borderLine;
                    root.drawGaugeGraphVertical(ctx, previousX, borderY, previousY, lineWidth);
                    ctx.fillStyle = colors.graphLine;
                    root.drawGaugeGraphVertical(ctx, previousX, y, borderY, lineWidth);
                    root.logicalFillRect(ctx, previousX, y, Math.max(lineWidth, x - previousX + lineWidth), lineWidth);
                }
            }
            if (value >= 0) {
                lastGauge = value;
                lastX = x;
                lastY = y;
            }
            previousValue = value;
            previousX = x;
            previousY = y;
        }

        if (lastGauge >= 0) {
            ctx.fillStyle = lastGauge < borderPercent ? colors.graphLine : colors.borderLine;
            root.logicalFillRect(ctx, lastX, lastY, Math.max(lineWidth, fieldW - lastX), lineWidth);
        }
    }

    visible: currentState && effectiveAlpha > 0 && !!srcData

    Canvas {
        id: chartCanvas
        anchors.fill: parent
        visible: root.visible
        renderTarget: Canvas.Image

        onPaint: {
            let ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.globalCompositeOperation = "source-over";
            if (!root.currentState || !root.srcData) {
                return;
            }
            ctx.imageSmoothingEnabled = root.currentState.filter !== 0;

            let fieldW = root.cachedFieldW;
            let fieldH = root.cachedFieldH;
            let values = root.valueCache;
            let columnCount = root.drawColumnCount();
            ctx.globalAlpha = root.effectiveAlpha / 255.0;
            if (root.cachedChartType === 1) {
                root.drawGaugeGraph(ctx, values, columnCount, fieldW, fieldH, root.cachedStep);
                return;
            }
            if (values.length === 0 || columnCount <= 0) {
                return;
            }
            let dstW = Math.max(1, Math.abs(root.currentState.w || root.srcData.w || 1));
            let dstH = Math.max(1, Math.abs(root.currentState.h || root.srcData.h || 1));
            let step = root.cachedStep;
            let divX = Math.max(1, root.srcData.div_x || 1);
            let divY = Math.max(1, root.srcData.div_y || 1);
            let srcW = Math.max(1, (root.srcData.w || 1) / divX);
            let srcH = Math.max(1, (root.srcData.h || 1) / divY);
            let col = root.sourceFrameIndex % divX;
            let row = Math.floor(root.sourceFrameIndex / divX) % divY;
            let sx = (root.srcData.x || 0) + col * srcW;
            let sy = (root.srcData.y || 0) + row * srcH;
            let imageLoaded = root.resolvedSource !== "" && isImageLoaded(root.resolvedSource);

            ctx.fillStyle = root.graphColor(root.currentState);

            let previousValue = values[0];
            for (let column = 0; column < columnCount; ++column) {
                let x = column * step;
                let value = values[column];
                if (value < 0) {
                    previousValue = value;
                    continue;
                }

                if (previousValue >= 0) {
                    let direction = value >= previousValue ? 1 : -1;
                    let roundedStart = Math.trunc(previousValue);
                    let roundedEnd = Math.trunc(value);
                    for (let v = roundedStart; v !== roundedEnd; v += direction) {
                        root.drawGraphPoint(ctx, imageLoaded, sx, sy, srcW, srcH, x, v, dstW, dstH, fieldH);
                    }
                }
                root.drawGraphPoint(ctx, imageLoaded, sx, sy, srcW, srcH, x, value, dstW, dstH, fieldH);
                previousValue = value;
            }
        }

        Component.onCompleted: {
            if (root.resolvedSource !== "") {
                loadImage(root.resolvedSource);
            }
            root.paintedColumnCount = -1;
            root.requestTimedChartPaint();
        }
        onImageLoaded: requestPaint()
    }

    onResolvedSourceChanged: {
        if (resolvedSource !== "") {
            chartCanvas.loadImage(resolvedSource);
        }
        requestChartPaint();
    }
    onValueCacheStateChanged: {
        root.paintedColumnCount = -1;
        root.requestTimedChartPaint();
    }
    onSkinTimeChanged: requestTimedChartPaint()
}
