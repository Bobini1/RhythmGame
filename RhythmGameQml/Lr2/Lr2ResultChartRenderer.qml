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
    property var valueCache: []
    property int cachedFieldW: 1
    property int cachedFieldH: 1
    property int cachedStep: 1
    property int cachedChartType: 0
    property int cachedChartIndex: 0
    property bool cachedGaugeHard: false
    property int paintedColumnCount: -1

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
    readonly property int resultRevision: screenRoot
        ? screenRoot.resultOldScoresRevision + screenRoot.resultGaugeSelectionRevision
        : 0
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

    function resultScore() : var {
        let side = srcData && srcData.side ? srcData.side : 1;
        return screenRoot ? screenRoot.resultScore(side) : null;
    }

    function resultData() : var {
        let score = resultScore();
        return score && score.result ? score.result : null;
    }

    function replayEvents() : var {
        let score = resultScore();
        return score && score.replayData ? (score.replayData.hitEvents || []) : [];
    }

    function scoreChartSide() : var {
        return srcData && srcData.side ? srcData.side : 1;
    }

    function scoreChartScore() : var {
        if (!screenRoot || !srcData) {
            return null;
        }
        let index = srcData.resultChartIndex || 0;
        let side = scoreChartSide();
        if (index === 0) {
            return screenRoot.resultScore(side);
        }
        if (index === 1 && screenRoot.resultOldBestScore) {
            return screenRoot.resultOldBestScore(side);
        }
        if (index === 2 && screenRoot.resultTargetSavedScore) {
            return screenRoot.resultTargetSavedScore(side);
        }
        return null;
    }

    function scoreChartReplayEvents(score: var) : var {
        return score && score.replayData ? (score.replayData.hitEvents || []) : [];
    }

    function gaugeInfo() : var {
        if (root.screenRoot && root.screenRoot.resultGaugeInfo) {
            return root.screenRoot.resultGaugeInfo(scoreChartSide());
        }

        let score = resultScore();
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
        let score = scoreChartScore();
        let result = score && score.result ? score.result : null;
        if (result) {
            return result.points || 0;
        }
        if (screenRoot && srcData && (srcData.resultChartIndex || 0) === 2
                && screenRoot.resultTargetPoints) {
            return screenRoot.resultTargetPoints(scoreChartSide());
        }
        return -1;
    }

    function graphColorComponent(state: var, name: var) : var {
        return Math.max(0, Math.min(255, state && state[name] !== undefined ? state[name] : 255));
    }

    function graphColor(state: var) : var {
        let r = graphColorComponent(state, "r");
        let g = graphColorComponent(state, "g");
        let b = graphColorComponent(state, "b");
        return "rgb(" + r + "," + g + "," + b + ")";
    }

    function graphNeedsTint(state: var) : var {
        let r = graphColorComponent(state, "r");
        let g = graphColorComponent(state, "g");
        let b = graphColorComponent(state, "b");
        return r !== 255 || g !== 255 || b !== 255;
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
        if (!screenRoot || !srcData || (srcData.resultChartIndex || 0) !== 2
                || !screenRoot.resultTargetFraction || scoreChartScore()) {
            return [];
        }

        let currentScore = resultScore();
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
        let perNotePoints = 2.0 * Math.max(0, screenRoot.resultTargetFraction(scoreChartSide()) || 0);
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
        root.cachedGaugeHard = !!info && (String(info.name).toUpperCase().indexOf("HARD") !== -1
            || String(info.name).toUpperCase().indexOf("DAN") !== -1
            || String(info.name).toUpperCase().indexOf("FC") !== -1
            || String(info.name).toUpperCase().indexOf("PERFECT") !== -1
            || String(info.name).toUpperCase().indexOf("MAX") !== -1);

        let count = cacheValueCount(fieldW, step);
        let result = new Array(count);
        if (!history || history.length === 0) {
            for (let i = 0; i < count; ++i) {
                result[i] = 0;
            }
            return result;
        }

        let currentResult = resultData();
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
        return result;
    }

    function buildScoreCache(fieldW: var, step: var) : var {
        let current = resultData();
        let maxPoints = current ? Math.max(1, current.maxPoints || 1) : 1;
        let score = scoreChartScore();
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

    function rebuildValueCache() : var {
        if (!root.currentState || !root.srcData) {
            root.valueCache = [];
            root.paintedColumnCount = -1;
            return;
        }

        let fieldW = Math.max(1, root.srcData.op1 || Math.abs(root.currentState.w) || 1);
        let fieldH = Math.max(1, root.srcData.op2 || Math.abs(root.currentState.h) || 1);
        let dstW = Math.max(1, Math.abs(root.currentState.w || root.srcData.w || 1));
        let chartType = root.srcData.resultChartType || 0;
        root.cachedFieldW = fieldW;
        root.cachedFieldH = fieldH;
        root.cachedStep = Math.max(1, dstW);
        root.cachedChartType = chartType;
        root.cachedChartIndex = root.srcData.resultChartIndex || 0;
        root.cachedGaugeHard = false;
        root.valueCache = chartType === 1
            ? buildGaugeCache(fieldW, root.cachedStep)
            : buildScoreCache(fieldW, root.cachedStep);
        root.paintedColumnCount = -1;
        requestTimedChartPaint();
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

            ctx.globalAlpha = root.effectiveAlpha / 255.0;
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
            root.rebuildValueCache();
        }
        onImageLoaded: requestPaint()
    }

    onResolvedSourceChanged: {
        if (resolvedSource !== "") {
            chartCanvas.loadImage(resolvedSource);
        }
        requestChartPaint();
    }
    onCurrentStateChanged: rebuildValueCache()
    onDstsChanged: rebuildValueCache()
    onSkinTimeChanged: requestTimedChartPaint()
    onSrcDataChanged: rebuildValueCache()
    onScreenRootChanged: rebuildValueCache()
    onResultRevisionChanged: rebuildValueCache()
}
