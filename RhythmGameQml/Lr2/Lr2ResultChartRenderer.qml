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
    property var liveCacheStore: ({
        score: { key: "", values: [], eventIndex: 0, points: 0 },
        gradeTarget: { key: "", values: [], eventIndex: 0, points: 0 },
        gaugeCaches: {},
        gaugeScoreGuid: "",
        gaugeWarmKey: ""
    })
    property int renderedColumnCount: 0
    property string renderedPaintKey: ""
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
    readonly property int effectiveChartSide: root.chartSide === 2 ? 2 : 1
    readonly property int chartIndex: root.srcData ? (root.srcData.resultChartIndex || 0) : 0
    readonly property int chartType: root.srcData ? (root.srcData.resultChartType || 0) : 0
    readonly property bool gameplayChartActive: !!root.screenRoot && !!root.screenRoot.gameplayScreenActive
    readonly property bool gameplayMyBestGaugeChart: root.gameplayChartActive
        && root.chartType === 1 && root.chartSide === 3
    readonly property var gameplayPlayerData: root.gameplayChartActive && root.screenRoot.gameplayPlayer
        ? root.screenRoot.gameplayPlayer(root.effectiveChartSide)
        : null
    readonly property var gameplayScoreData: root.gameplayChartActive && root.screenRoot.gameplayScore
        ? root.screenRoot.gameplayScore(root.effectiveChartSide)
        : null
    readonly property var gameplayGaugeData: root.gameplayChartActive
            && !root.gameplayMyBestGaugeChart
        ? root.liveGaugeByName(root.gameplayActiveGaugeName)
        : null
    readonly property string gameplayActiveGaugeName: root.gameplayChartActive
            && root.screenRoot
            && root.screenRoot.gameplayFrameStateRef
        ? (root.effectiveChartSide === 2
            ? (root.screenRoot.gameplayFrameStateRef.activeGaugeName2 || "")
            : (root.screenRoot.gameplayFrameStateRef.activeGaugeName1 || ""))
        : ""
    readonly property real gameplayActiveGaugeValue: root.gameplayChartActive
            && root.screenRoot
            && root.screenRoot.gameplayFrameStateRef
        ? (root.effectiveChartSide === 2
            ? (root.screenRoot.gameplayFrameStateRef.gaugeValue2 || 0)
            : (root.screenRoot.gameplayFrameStateRef.gaugeValue1 || 0))
        : 0.0
    readonly property int gameplayHitEventsRevision: root.gameplayChartActive
        ? (root.effectiveChartSide === 2
            ? (root.screenRoot.gameplayHitEventsRevision2 || 0)
            : (root.screenRoot.gameplayHitEventsRevision1 || 0))
        : 0
    readonly property int gameplayScoresRevision: root.gameplayChartActive
        ? (root.screenRoot.gameplayScoresRevision || 0)
        : 0
    readonly property real gameplayScorePoints: root.gameplayScoreData
        ? (root.gameplayScoreData.points || 0)
        : 0
    readonly property real gameplayScoreMaxPoints: root.gameplayScoreData
        ? (root.gameplayScoreData.maxPoints || 0)
        : 0
    readonly property real gameplayScoreMaxPointsNow: root.gameplayScoreData
        ? (root.gameplayScoreData.maxPointsNow || 0)
        : 0
    readonly property var resultScoreData: root.screenRoot && !root.gameplayChartActive && root.screenRoot.resultScore
        ? root.screenRoot.resultScore(root.effectiveChartSide)
        : null
    readonly property var resultDataValue: root.resultScoreData && root.resultScoreData.result
        ? root.resultScoreData.result
        : null
    readonly property var chartScoreData: {
        if (!root.screenRoot || !root.srcData) {
            return null;
        }
        if (root.gameplayChartActive) {
            if (root.chartIndex === 1 && root.screenRoot.gameplayBestSavedScore) {
                return root.screenRoot.gameplayBestSavedScore();
            }
            if (root.chartIndex === 2 && root.screenRoot.gameplayTargetSavedScore) {
                return root.screenRoot.gameplayTargetSavedScore();
            }
            return null;
        }
        if (root.chartIndex === 0) {
            return root.resultScoreData;
        }
        if (root.chartIndex === 1 && root.screenRoot.resultOldBestScore) {
            return root.screenRoot.resultOldBestScore(root.effectiveChartSide);
        }
        if (root.chartIndex === 2 && root.screenRoot.resultTargetSavedScore) {
            return root.screenRoot.resultTargetSavedScore(root.effectiveChartSide);
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

    function gameplayChartLength() : var {
        let length = Number(root.gameplayPlayerData && root.gameplayPlayerData.chartLength !== undefined
            ? root.gameplayPlayerData.chartLength : 0);
        if (isFinite(length) && length > 0) {
            return length;
        }
        return 1;
    }

    function gameplayChartElapsed() : var {
        let length = root.gameplayChartLength();
        let elapsed = Number(root.gameplayPlayerData && root.gameplayPlayerData.elapsed !== undefined
            ? root.gameplayPlayerData.elapsed : 0);
        if (!isFinite(elapsed)) {
            return 0;
        }
        return Math.max(0, Math.min(length, elapsed));
    }

    function chartLengthValue() : var {
        if (root.gameplayChartActive) {
            return root.gameplayChartLength();
        }
        let currentResult = root.resultDataValue;
        return currentResult ? Math.max(1, currentResult.length || 1) : 1;
    }

    function savedScoreGaugeInfo(score: var) : var {
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

    function liveGaugeByName(name: var) : var {
        let gauges = root.gameplayScoreData ? (root.gameplayScoreData.gauges || []) : [];
        if (!gauges || gauges.length === 0) {
            return null;
        }

        let normalizedName = String(name || "").toUpperCase();
        let fallback = null;
        for (let i = 0; i < gauges.length; ++i) {
            let gauge = gauges[i];
            if (!gauge) {
                continue;
            }
            fallback = gauge;
            if (normalizedName.length > 0
                    && String(gauge.name || "").toUpperCase() === normalizedName) {
                return gauge;
            }
        }
        return fallback;
    }

    function liveGaugeInfo(gauge: var) : var {
        if (!gauge) {
            return null;
        }
        return {
            maxGauge: Math.max(1, gauge.gaugeMax || 100),
            threshold: gauge.threshold || 0,
            name: gauge.name || "",
            courseGauge: gauge.courseGauge || false,
            gaugeHistory: gauge.gaugeHistory || []
        };
    }

    function gaugeInfo() : var {
        if (root.gameplayChartActive) {
            if (root.gameplayMyBestGaugeChart && root.screenRoot.gameplayBestSavedScore) {
                root.gameplayScoresRevision;
                return root.savedScoreGaugeInfo(root.screenRoot.gameplayBestSavedScore());
            }
            root.gameplayActiveGaugeName;
            root.gameplayActiveGaugeValue;
            return root.liveGaugeInfo(root.gameplayGaugeData);
        }

        if (root.screenRoot && root.screenRoot.resultGaugeInfo) {
            return root.screenRoot.resultGaugeInfo(root.effectiveChartSide);
        }

        return root.savedScoreGaugeInfo(root.resultScoreData);
    }

    function scoreChartFinalPoints() : var {
        if (root.gameplayChartActive) {
            if (root.chartIndex === 0) {
                return root.gameplayScorePoints;
            }
            if (root.chartIndex === 1 && root.screenRoot.gameplayHighScoreFinalPoints) {
                return root.screenRoot.gameplayHighScoreFinalPoints();
            }
            if (root.chartIndex === 2 && root.screenRoot.gameplayTargetFinalPoints) {
                return root.screenRoot.gameplayTargetFinalPoints(root.effectiveChartSide);
            }
            return -1;
        }
        let score = root.chartScoreData;
        let result = score && score.result ? score.result : null;
        if (result) {
            return result.points || 0;
        }
        if (screenRoot && srcData && root.chartIndex === 2 && screenRoot.resultTargetPoints) {
            return screenRoot.resultTargetPoints(root.effectiveChartSide);
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
        if (root.gameplayChartActive && root.cachedChartType === 2
                && color[0] === 255 && color[1] === 255 && color[2] === 255) {
            if (root.cachedChartIndex === 1) {
                return [20, 255, 20];
            }
            if (root.cachedChartIndex === 2) {
                return [255, 20, 20];
            }
            return [20, 20, 255];
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

    function resetLiveGraphCache(cache: var, key: var, count: var, initialPercent: var) : var {
        cache.key = key;
        cache.values = new Array(count);
        cache.eventIndex = 0;
        cache.points = 0;
        cache.historyIndex = 0;
        cache.value = 0;
        for (let i = 0; i < count; ++i) {
            cache.values[i] = initialPercent;
        }
        return cache;
    }

    function fillLiveGraphFrom(values: var, column: var, percent: var) : void {
        let start = Math.max(0, Math.min(values.length, column));
        for (let i = start; i < values.length; ++i) {
            values[i] = percent;
        }
    }

    function eventColumn(event: var, fieldW: var, step: var, length: var) : var {
        let offset = Number(event && event.offsetFromStart !== undefined ? event.offsetFromStart : 0);
        if (!isFinite(offset)) {
            offset = 0;
        }
        return Math.floor(Math.max(0, Math.min(fieldW, fieldW * offset / Math.max(1, length))) / step);
    }

    function buildGameplayLiveScoreCache(fieldW: var, step: var, maxPoints: var) : var {
        root.gameplayHitEventsRevision;
        if (!root.screenRoot || !root.screenRoot.gameplayHitEventsForSide || !root.gameplayScoreData) {
            return [];
        }

        let events = root.screenRoot.gameplayHitEventsForSide(root.effectiveChartSide) || [];
        let count = cacheValueCount(fieldW, step);
        let length = root.gameplayChartLength();
        let key = [
            "score",
            root.effectiveChartSide,
            root.gameplayScoreData.guid || "",
            fieldW,
            step,
            length,
            maxPoints
        ].join("|");
        let cache = root.liveCacheStore.score;
        if (cache.key !== key || !cache.values || cache.values.length !== count
                || cache.eventIndex > events.length) {
            cache = root.resetLiveGraphCache(cache, key, count, 0);
        }
        for (let i = cache.eventIndex; i < events.length; ++i) {
            let event = events[i];
            if (event && event.noteRemoved && event.points && event.points.value !== undefined) {
                cache.points += event.points.value || 0;
                root.fillLiveGraphFrom(
                    cache.values,
                    root.eventColumn(event, fieldW, step, length),
                    scorePercent(cache.points, maxPoints));
            }
        }
        cache.eventIndex = events.length;
        return cache.values;
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

    function buildGameplayGradeTargetCache(fieldW: var, step: var, maxPoints: var) : var {
        root.gameplayHitEventsRevision;
        if (!screenRoot || !srcData || root.chartIndex !== 2
                || root.chartScoreData || !screenRoot.gameplayHitEventsForSide) {
            return [];
        }

        let finalPoints = root.scoreChartFinalPoints();
        if (finalPoints < 0) {
            return [];
        }

        let events = screenRoot.gameplayHitEventsForSide(root.effectiveChartSide) || [];
        let count = cacheValueCount(fieldW, step);
        let length = root.gameplayChartLength();
        let perNotePoints = 2.0 * finalPoints / Math.max(1, maxPoints);
        let key = [
            "target",
            root.effectiveChartSide,
            root.gameplayScoreData ? root.gameplayScoreData.guid || "" : "",
            fieldW,
            step,
            length,
            maxPoints,
            finalPoints
        ].join("|");
        let cache = root.liveCacheStore.gradeTarget;
        if (cache.key !== key || !cache.values || cache.values.length !== count
                || cache.eventIndex > events.length) {
            cache = root.resetLiveGraphCache(cache, key, count, 0);
        }
        for (let i = cache.eventIndex; i < events.length; ++i) {
            let event = events[i];
            if (event && event.noteRemoved && event.points) {
                cache.points += perNotePoints;
                root.fillLiveGraphFrom(
                    cache.values,
                    root.eventColumn(event, fieldW, step, length),
                    scorePercent(cache.points, maxPoints));
            }
        }
        cache.eventIndex = events.length;
        return cache.values;
    }

    function buildGaugeValuesFromHistory(history: var, fieldW: var, step: var, length: var, maxGauge: var) : var {
        let count = cacheValueCount(fieldW, step);
        let result = new Array(count);
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

    function liveGaugeCacheKey(gaugeName: var, maxGauge: var, borderPercent: var, fieldW: var, step: var, length: var) : var {
        return [
            "gauge",
            root.effectiveChartSide,
            root.gameplayScoreData ? root.gameplayScoreData.guid || "" : "",
            gaugeName,
            maxGauge,
            borderPercent,
            fieldW,
            step,
            length
        ].join("|");
    }

    function updateLiveGaugeCache(info: var, fieldW: var, step: var, length: var) : var {
        let history = info && info.gaugeHistory ? info.gaugeHistory : [];
        if (!history || history.length === 0) {
            return null;
        }

        let maxGauge = Math.max(1, info.maxGauge || 100);
        let borderPercent = root.clampPercent(Number(info.threshold || 0) * 100 / maxGauge);
        let gaugeName = String(info.name || "").toUpperCase();
        let key = root.liveGaugeCacheKey(gaugeName, maxGauge, borderPercent, fieldW, step, length);
        let count = cacheValueCount(fieldW, step);
        let store = root.liveCacheStore;
        let cache = store.gaugeCaches[key];
        if (!cache || !cache.values || cache.values.length !== count
                || cache.historyIndex > history.length) {
            let lastEntry = history[history.length - 1];
            cache = {
                values: root.buildGaugeValuesFromHistory(history, fieldW, step, length, maxGauge),
                historyIndex: history.length,
                value: lastEntry && lastEntry.gauge !== undefined ? lastEntry.gauge : 0
            };
            store.gaugeCaches[key] = cache;
            return cache;
        }

        for (let i = cache.historyIndex; i < history.length; ++i) {
            let entry = history[i];
            if (entry && entry.gauge !== undefined) {
                cache.value = entry.gauge;
                root.fillLiveGraphFrom(
                    cache.values,
                    root.eventColumn(entry, fieldW, step, length),
                    clampPercent(cache.value * 100 / maxGauge));
            }
        }
        cache.historyIndex = history.length;
        return cache;
    }

    function warmLiveGaugeCaches(fieldW: var, step: var, length: var) : void {
        let gauges = root.gameplayScoreData ? (root.gameplayScoreData.gauges || []) : [];
        if (!gauges || gauges.length === 0) {
            return;
        }
        let store = root.liveCacheStore;
        let scoreGuid = root.gameplayScoreData ? root.gameplayScoreData.guid || "" : "";
        if (store.gaugeScoreGuid !== scoreGuid) {
            store.gaugeScoreGuid = scoreGuid;
            store.gaugeWarmKey = "";
            store.gaugeCaches = {};
        }
        let warmKey = [
            scoreGuid,
            root.gameplayActiveGaugeName,
            root.gameplayActiveGaugeValue,
            fieldW,
            step,
            length
        ].join("|");
        if (store.gaugeWarmKey === warmKey) {
            return;
        }
        store.gaugeWarmKey = warmKey;
        for (let i = 0; i < gauges.length; ++i) {
            root.updateLiveGaugeCache(root.liveGaugeInfo(gauges[i]), fieldW, step, length);
        }
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
        let length = root.chartLengthValue();
        let result = null;
        if (root.gameplayChartActive && !root.gameplayMyBestGaugeChart) {
            root.warmLiveGaugeCaches(fieldW, step, length);
            let cache = root.updateLiveGaugeCache(info, fieldW, step, length);
            result = cache ? cache.values : [];
        } else {
            result = root.buildGaugeValuesFromHistory(history, fieldW, step, length, maxGauge);
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
        let maxPoints = root.gameplayChartActive
            ? Math.max(1, root.gameplayScoreMaxPoints || 1)
            : (current ? Math.max(1, current.maxPoints || 1) : 1);
        if (root.gameplayChartActive && root.chartIndex === 0) {
            return buildGameplayLiveScoreCache(fieldW, step, maxPoints);
        }

        let score = root.chartScoreData;
        let replayValues = buildReplayScoreCache(score, fieldW, step, maxPoints);
        if (replayValues.length > 0) {
            return replayValues;
        }

        let gradeValues = root.gameplayChartActive
            ? buildGameplayGradeTargetCache(fieldW, step, maxPoints)
            : buildGradeTargetCache(fieldW, step, maxPoints);
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
        if (root.gameplayChartActive
                && root.gameplayPlayerData
                && (root.cachedChartType !== 1 || !root.gameplayMyBestGaugeChart)) {
            let length = root.gameplayChartLength();
            let elapsed = root.gameplayChartElapsed();
            let drawLength = Math.max(0, Math.min(root.cachedFieldW, root.cachedFieldW * elapsed / length));
            return Math.min(root.valueCache.length, Math.ceil(drawLength / root.cachedStep));
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

    function scoreGraphY(value: var, fieldH: var) : var {
        return Math.round(-root.clampPercent(value) * Math.max(1, fieldH) / 100.0);
    }

    function drawScoreLineGraph(ctx: var, values: var, columnCount: var, fieldW: var, fieldH: var, step: var, startColumn: var) : void {
        if (!values || values.length === 0 || columnCount <= 0) {
            return;
        }

        let firstColumn = Math.max(0, Math.min(columnCount - 1, startColumn || 0));
        let lineWidth = Math.max(1, Math.abs(root.srcData && root.srcData.w ? root.srcData.w : 1));
        ctx.save();
        ctx.strokeStyle = root.graphColor(root.currentState);
        ctx.lineWidth = Math.max(1, lineWidth * root.scaleOverride);
        ctx.lineCap = "butt";
        ctx.lineJoin = "round";
        ctx.beginPath();

        let started = false;
        if (firstColumn > 0 && values[firstColumn - 1] >= 0) {
            let previousX = (root.currentState.x + Math.min(fieldW, (firstColumn - 1) * step)) * root.scaleOverride;
            let previousY = (root.currentState.y + root.scoreGraphY(values[firstColumn - 1], fieldH)) * root.scaleOverride;
            ctx.moveTo(previousX, previousY);
            started = true;
        }
        for (let column = firstColumn; column < columnCount; ++column) {
            let value = values[column];
            if (value < 0) {
                started = false;
                continue;
            }

            let x = (root.currentState.x + Math.min(fieldW, column * step)) * root.scaleOverride;
            let y = (root.currentState.y + root.scoreGraphY(value, fieldH)) * root.scaleOverride;
            if (!started) {
                ctx.moveTo(x, y);
                started = true;
            } else {
                ctx.lineTo(x, y);
            }
        }

        ctx.stroke();
        ctx.restore();
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

    function drawGaugeGraphBackground(ctx: var, fieldW: var, fieldH: var, borderPercent: var, colors: var, startX: var, widthOverride: var) : void {
        let x = Math.max(0, startX || 0);
        let width = widthOverride === undefined
            ? Math.max(0, fieldW - x)
            : Math.max(0, widthOverride || 0);
        if (width <= 0) {
            return;
        }
        let topHeight = fieldH * (100 - root.clampPercent(borderPercent)) / 100.0;
        ctx.fillStyle = colors.graphBg;
        root.logicalFillRect(ctx, x, -fieldH, width, fieldH);
        if (topHeight > 0) {
            ctx.fillStyle = colors.borderBg;
            root.logicalFillRect(ctx, x, -fieldH, width, topHeight);
        }
    }

    function drawGaugeGraphSegment(ctx: var, x1: var, y1: var, x2: var, y2: var, lineWidth: var) : void {
        root.logicalFillRect(ctx, x1, Math.min(y1, y2), lineWidth, Math.abs(y2 - y1) + lineWidth);
        root.logicalFillRect(ctx, x1, y2, Math.max(lineWidth, x2 - x1 + lineWidth), lineWidth);
    }

    function drawGaugeGraphVertical(ctx: var, x: var, y1: var, y2: var, lineWidth: var) : void {
        root.logicalFillRect(ctx, x, Math.min(y1, y2), lineWidth, Math.abs(y2 - y1) + lineWidth);
    }

    function drawGaugeGraph(ctx: var, values: var, columnCount: var, fieldW: var, fieldH: var, step: var, startColumn: var, fullPaint: var) : void {
        let lineWidth = Math.max(1, Math.abs(root.srcData && root.srcData.h ? root.srcData.h : 1));
        let borderPercent = root.cachedGaugeBorderPercent;
        let borderY = root.gaugeGraphY(borderPercent, fieldH, lineWidth);
        let colors = root.cachedGaugeColors || root.gaugeGraphColors(null);
        let firstColumn = Math.max(1, Math.min(columnCount, startColumn || 1));
        if (fullPaint) {
            root.drawGaugeGraphBackground(ctx, fieldW, fieldH, borderPercent, colors, 0);
        } else {
            root.drawGaugeGraphBackground(
                ctx,
                fieldW,
                fieldH,
                borderPercent,
                colors,
                Math.max(0, (firstColumn - 1) * step));
        }
        if (!values || values.length === 0 || columnCount <= 0) {
            return;
        }

        let previousValue = values[firstColumn - 1];
        let previousX = (firstColumn - 1) * step;
        let previousY = root.gaugeGraphY(previousValue, fieldH, lineWidth);
        let lastX = previousX;
        let lastY = previousY;
        let lastGauge = -1;

        for (let column = firstColumn; column < columnCount; ++column) {
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

        if (fullPaint && lastGauge >= 0 && !(root.gameplayChartActive && !root.gameplayMyBestGaugeChart)) {
            ctx.fillStyle = lastGauge < borderPercent ? colors.graphLine : colors.borderLine;
            root.logicalFillRect(ctx, lastX, lastY, Math.max(lineWidth, fieldW - lastX), lineWidth);
        }
    }

    function paintKey() : var {
        if (!root.currentState || !root.srcData) {
            return "";
        }
        return [
            root.resolvedSource,
            root.currentState.x || 0,
            root.currentState.y || 0,
            root.currentState.w || 0,
            root.currentState.h || 0,
            root.effectiveAlpha,
            root.currentState.r === undefined ? 255 : root.currentState.r,
            root.currentState.g === undefined ? 255 : root.currentState.g,
            root.currentState.b === undefined ? 255 : root.currentState.b,
            root.cachedFieldW,
            root.cachedFieldH,
            root.cachedStep,
            root.cachedChartType,
            root.cachedChartIndex,
            root.cachedGaugeHard ? 1 : 0,
            root.cachedGaugeBorderPercent
        ].join("|");
    }

    function clearGraphPaintArea(ctx: var, startColumn: var, fieldW: var, fieldH: var, step: var) : void {
        let startX = Math.max(0, Math.min(fieldW, Math.max(0, startColumn || 0) * step));
        ctx.clearRect(
            (root.currentState.x + startX) * root.scaleOverride,
            (root.currentState.y - fieldH) * root.scaleOverride,
            Math.max(1, (fieldW - startX + step) * root.scaleOverride),
            Math.max(1, fieldH * root.scaleOverride));
    }

    visible: currentState && effectiveAlpha > 0 && !!srcData

    Canvas {
        id: chartCanvas
        anchors.fill: parent
        visible: root.visible
        renderTarget: Canvas.Image

        onPaint: {
            let ctx = getContext("2d");
            ctx.globalCompositeOperation = "source-over";
            if (!root.currentState || !root.srcData) {
                ctx.clearRect(0, 0, width, height);
                root.renderedPaintKey = "";
                root.renderedColumnCount = 0;
                return;
            }
            ctx.imageSmoothingEnabled = root.currentState.filter !== 0;

            let fieldW = root.cachedFieldW;
            let fieldH = root.cachedFieldH;
            let values = root.valueCache;
            let columnCount = root.drawColumnCount();
            let key = root.paintKey();
            let incremental = root.gameplayChartActive
                && root.renderedPaintKey === key
                && root.renderedColumnCount > 0
                && columnCount >= root.renderedColumnCount;
            let startColumn = incremental
                ? Math.max(0, root.renderedColumnCount - 1)
                : 0;
            if (incremental) {
                root.clearGraphPaintArea(ctx, startColumn, fieldW, fieldH, root.cachedStep);
            } else {
                ctx.clearRect(0, 0, width, height);
            }
            ctx.globalAlpha = root.effectiveAlpha / 255.0;
            if (root.cachedChartType === 1) {
                root.drawGaugeGraph(ctx, values, columnCount, fieldW, fieldH, root.cachedStep, startColumn, !incremental);
                root.renderedPaintKey = key;
                root.renderedColumnCount = columnCount;
                return;
            }
            if (root.gameplayChartActive) {
                root.drawScoreLineGraph(ctx, values, columnCount, fieldW, fieldH, root.cachedStep, startColumn);
                root.renderedPaintKey = key;
                root.renderedColumnCount = columnCount;
                return;
            }
            ctx.clearRect(0, 0, width, height);
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
        root.renderedColumnCount = 0;
        root.renderedPaintKey = "";
        root.requestTimedChartPaint();
    }
    onSkinTimeChanged: requestTimedChartPaint()
}
