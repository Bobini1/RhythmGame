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
    readonly property int resultRevision: screenRoot ? screenRoot.resultOldScoresRevision : 0

    function resultScore() {
        let side = srcData && srcData.side ? srcData.side : 1;
        return screenRoot ? screenRoot.resultScore(side) : null;
    }

    function resultData() {
        let score = resultScore();
        return score && score.result ? score.result : null;
    }

    function replayEvents() {
        let score = resultScore();
        return score && score.replayData ? (score.replayData.hitEvents || []) : [];
    }

    function scoreChartSide() {
        return srcData && srcData.side ? srcData.side : 1;
    }

    function scoreChartScore() {
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

    function scoreChartReplayEvents(score) {
        return score && score.replayData ? (score.replayData.hitEvents || []) : [];
    }

    function gaugeInfo() {
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
            if (!best && last > (info.threshold || 0)) {
                best = info;
            }
            if (info && !info.courseGauge) {
                fallback = info;
            }
        }
        return best || fallback;
    }

    function scoreChartFinalPoints() {
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

    function graphColor(state) {
        let r = Math.max(0, Math.min(255, state && state.r !== undefined ? state.r : 255));
        let g = Math.max(0, Math.min(255, state && state.g !== undefined ? state.g : 255));
        let b = Math.max(0, Math.min(255, state && state.b !== undefined ? state.b : 255));
        let a = Math.max(0, Math.min(1, (state && state.a !== undefined ? state.a : 255) / 255.0));
        return "rgba(" + r + "," + g + "," + b + "," + a + ")";
    }

    function cacheValueCount(fieldW, step) {
        return Math.max(1, Math.ceil(Math.max(1, fieldW) / Math.max(1, step)));
    }

    function clampPercent(value) {
        return Math.max(0, Math.min(100, value));
    }

    function scorePercent(points, maxPoints) {
        return clampPercent(points * 100 / Math.max(1, maxPoints || 1));
    }

    function buildLinearScoreCache(fieldW, step, finalPoints, maxPoints) {
        let count = cacheValueCount(fieldW, step);
        let result = new Array(count);
        for (let i = 0; i < count; ++i) {
            result[i] = scorePercent(finalPoints * (i * step / fieldW), maxPoints);
        }
        return result;
    }

    function buildReplayScoreCache(score, fieldW, step, maxPoints) {
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

    function buildGradeTargetCache(fieldW, step, maxPoints) {
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

    function buildGaugeCache(fieldW, step) {
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

    function buildScoreCache(fieldW, step) {
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

    function rebuildValueCache() {
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

    function cachedSegmentVisible(value) {
        if (root.cachedChartType !== 1) {
            return true;
        }
        if (root.cachedGaugeHard) {
            return root.cachedChartIndex === 1;
        }
        return root.cachedChartIndex === 0 ? value < 80 : value >= 80;
    }

    function drawColumnCount() {
        if (!root.currentState || !root.srcData || root.valueCache.length === 0) {
            return 0;
        }
        let start = Math.max(0, root.srcData.op3 || 0);
        let end = Math.max(start + 1, root.srcData.op4 || start + 1);
        let elapsed = Math.max(0, Math.min(end - start, root.skinTime - start));
        let drawLength = Math.max(0, Math.min(root.cachedFieldW, root.cachedFieldW * elapsed / (end - start)));
        return Math.min(root.valueCache.length, Math.ceil(drawLength / root.cachedStep));
    }

    function requestChartPaint() {
        if (chartCanvas.available) {
            chartCanvas.requestPaint();
        }
    }

    function requestTimedChartPaint() {
        let count = drawColumnCount();
        if (count === root.paintedColumnCount) {
            return;
        }
        root.paintedColumnCount = count;
        requestChartPaint();
    }

    function drawGraphPoint(ctx, imageLoaded, sx, sy, srcW, srcH, x, value, dstW, dstH, fieldH) {
        if (value < 0 || !root.cachedSegmentVisible(value)) {
            return;
        }
        let offsetY = -fieldH * Math.max(0, Math.min(100, value)) / 100.0;
        let dx = (root.currentState.x + x) * root.scaleOverride;
        let dy = (root.currentState.y + offsetY) * root.scaleOverride;
        let dw = Math.max(1, dstW * root.scaleOverride);
        let dh = Math.max(1, dstH * root.scaleOverride);
        if (imageLoaded) {
            ctx.drawImage(root.resolvedSource, sx, sy, srcW, srcH, dx, dy, dw, dh);
        } else {
            ctx.fillRect(dx, dy, dw, dh);
        }
    }

    visible: currentState && currentState.a > 0 && !!srcData

    Canvas {
        id: chartCanvas
        anchors.fill: parent
        visible: root.visible
        renderTarget: Canvas.Image

        onPaint: {
            let ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            if (!root.currentState || !root.srcData) {
                return;
            }

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

            ctx.globalAlpha = Math.max(0, Math.min(1, (root.currentState.a || 255) / 255.0));
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
                    let roundedStart = Math.round(previousValue);
                    let roundedEnd = Math.round(value);
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
