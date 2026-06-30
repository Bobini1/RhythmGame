pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

QtObject {
    id: root

    required property var host
    required property var selectContext

    property var resultOldScores1: []
    property var resultOldScores2: []
    property int resultOldScoresRequest: 0
    property int resultTimer151SkinTime: -1
    property int resultTimer152SkinTime: -1
    property int resultGraphStartSkinTime: 500
    property int resultGraphEndSkinTime: 2000
    property var resultTimingStatsCache: ({})
    property var resultJudgeTimingCountsCache: ({})
    property int resultGaugeSelectionIndex1: -1
    property int resultGaugeSelectionIndex2: -1
    readonly property bool resultScreenActive: root.host ? root.host.resultScreenActive : false
    readonly property bool gameplayScreenActive: root.host ? root.host.gameplayScreenActive : false

    function resultScore(side: var) : var {
        return root.host.scores && root.host.scores.length >= side
            ? root.host.scores[side - 1]
            : null;
    }

    function resultData(side: var) : var {
        let score = root.resultScore(side);
        return score && score.result ? score.result : null;
    }

    function resultProfile(side: var) : var {
        return root.host.profiles && root.host.profiles.length >= side
            ? root.host.profiles[side - 1]
            : (Rg.profileList ? Rg.profileList.mainProfile : null);
    }

    function resultCourseChartData() : var {
        if (root.host.chartData) {
            return root.host.chartData;
        }
        let charts = root.host.chartDatas || [];
        for (let i = charts.length - 1; i >= 0; --i) {
            let chart = charts[i];
            if (chart && typeof chart !== "string") {
                return chart;
            }
        }
        return null;
    }

    function resultChartData() : var {
        if (root.host.effectiveScreenKey === "courseResult") {
            return root.resultCourseChartData();
        }
        if (root.host.chartData) {
            return root.host.chartData;
        }
        return root.host.chartDatas && root.host.chartDatas.length > 0
            ? root.host.chartDatas[0]
            : null;
    }

    function displayChartData() : var {
        if (root.resultScreenActive) {
            return root.resultChartData();
        }
        if (root.host.effectiveScreenKey === "select") {
            return root.selectContext.selectedStateChartData;
        }
        if (root.gameplayScreenActive) {
            return root.host.gameplayChartData();
        }
        return root.host.chart && root.host.chart.chartData ? root.host.chart.chartData : null;
    }

    function resultClearOption() : var {
        let result = root.resultData(1);
        let clearType = result ? String(result.clearType || "FAILED") : "FAILED";
        if (root.host && root.host.skinClearTypeForStatus) {
            clearType = root.host.skinClearTypeForStatus(clearType);
        }
        return clearType !== "FAILED" && clearType !== "NOPLAY" ? 90 : 91;
    }

    function resultTotalNotes(result: var) : var {
        if (!result) {
            return 0;
        }
        let maxPoints = Math.floor(result.maxPoints || 0);
        return maxPoints > 0 ? Math.floor(maxPoints / 2) : Math.max(0, result.maxHits || 0);
    }

    function resultJudgementCount(result: var, judgement: var) : var {
        let counts = result && result.judgementCounts ? result.judgementCounts : [];
        return judgement >= 0 && judgement < counts.length ? (counts[judgement] || 0) : 0;
    }

    function resultPoorCount(result: var) : var {
        return root.resultJudgementCount(result, Judgement.Poor)
            + root.resultJudgementCount(result, Judgement.EmptyPoor);
    }

    function resultBadPoor(result: var) : var {
        return root.resultJudgementCount(result, Judgement.Bad) + root.resultPoorCount(result);
    }

    function emptyJudgeTimingCounts() : var {
        return {
            early: [0, 0, 0, 0, 0, 0],
            late: [0, 0, 0, 0, 0, 0]
        };
    }

    function emptyJudgeTimingStats() : var {
        return {
            count: 0,
            sum: 0,
            sumSq: 0
        };
    }

    function judgementTimingBucket(judgement: var) : var {
        switch (judgement) {
        case Judgement.Perfect:
            return 0;
        case Judgement.Great:
            return 1;
        case Judgement.Good:
            return 2;
        case Judgement.Bad:
            return 3;
        default:
            return -1;
        }
    }

    function hitDeviationNanos(hit: var) : var {
        if (!hit) {
            return 0;
        }
        if (hit.points && hit.points.deviation !== undefined) {
            return Number(hit.points.deviation || 0);
        }
        if (hit.hitOffset !== undefined) {
            return Number(hit.hitOffset || 0);
        }
        return 0;
    }

    function hitDeviationMillis(hit: var) : var {
        return Math.round(-root.hitDeviationNanos(hit) / 1000000);
    }

    function judgementUpdatesJudgeTimingValue(judgement: var) : var {
        return judgement >= Judgement.Bad && judgement <= Judgement.Perfect;
    }

    function cloneJudgeTimingCounts(counts: var) : var {
        if (!counts || !counts.early || !counts.late) {
            return root.emptyJudgeTimingCounts();
        }
        return {
            early: counts.early.slice(),
            late: counts.late.slice()
        };
    }

    function emptyJudgeLaneValues() : var {
        return [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    }

    function nowJudgeValue(judgement: var) : var {
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
            return 5;
        case Judgement.EmptyPoor:
            return 6;
        default:
            return 0;
        }
    }

    function laneJudgeValue(judgement: var, timing: var) : var {
        switch (judgement) {
        case Judgement.Perfect:
            return 1;
        case Judgement.Great:
            return timing > 0 ? 2 : 3;
        case Judgement.Good:
            return timing > 0 ? 4 : 5;
        case Judgement.Bad:
            return timing > 0 ? 6 : 7;
        default:
            return 0;
        }
    }

    function setGameplayJudgeLaneValue(scoreSide: var, hit: var, value: var) : var {
        let lane = root.host.gameplayLr2LaneForHit(scoreSide, hit);
        if (lane < 0) {
            return;
        }

        let valuesName = scoreSide === 2 ? "gameplayJudgeLaneValues2" : "gameplayJudgeLaneValues1";
        let values = root.host[valuesName];
        if (!values || !values.length) {
            values = root.emptyJudgeLaneValues();
            root.host[valuesName] = values;
        }
        if (lane >= values.length) {
            values.length = lane + 1;
            for (let i = 0; i < values.length; ++i) {
                values[i] = values[i] || 0;
            }
        }
        values[lane] = value;
    }

    function gameplayJudgeValueForId(num: var) : var {
        if (num === 520 || num === 522) {
            return root.host.gameplayJudgeNowValue1;
        }
        if (num === 521) {
            return root.host.gameplayJudgeNowValue2;
        }

        let side = 1;
        let offset = -1;
        if (num >= 500 && num <= 519) {
            side = Math.floor((num - 500) / 10) + 1;
            offset = (num - 500) % 10;
        } else if (num >= 1510 && num <= 1599) {
            side = 1;
            offset = (num - 1510) + 10;
        } else if (num >= 1610 && num <= 1699) {
            side = 2;
            offset = (num - 1610) + 10;
        } else {
            return 0;
        }

        let values = side === 2
            ? root.host.gameplayJudgeLaneValues2
            : root.host.gameplayJudgeLaneValues1;
        return values && offset >= 0 && offset < values.length ? (values[offset] || 0) : 0;
    }

    function recordGameplayJudgeTiming(scoreSide: var, hit: var) : var {
        let judgement = root.host.gameplayJudgementFromHit(hit);
        let bucket = root.judgementTimingBucket(judgement);
        if (bucket < 0) {
            return;
        }

        let countsName = scoreSide === 2 ? "gameplayJudgeTimingCounts2" : "gameplayJudgeTimingCounts1";
        let counts = root.host[countsName];
        if (!counts || !counts.early || !counts.late) {
            counts = root.emptyJudgeTimingCounts();
            root.host[countsName] = counts;
        }
        let side = root.hitDeviationNanos(hit) < 0 ? counts.early : counts.late;
        side[bucket] = (side[bucket] || 0) + 1;

        let timing = root.hitDeviationMillis(hit);
        if (root.judgementUpdatesJudgeTimingValue(judgement)) {
            let timingName = scoreSide === 2 ? "gameplayLastJudgeTiming2" : "gameplayLastJudgeTiming1";
            root.host[timingName] = timing;

            let statsName = scoreSide === 2 ? "gameplayJudgeTimingStats2" : "gameplayJudgeTimingStats1";
            let stats = root.host[statsName];
            if (!stats) {
                stats = root.emptyJudgeTimingStats();
                root.host[statsName] = stats;
            }
            stats.count = (stats.count || 0) + 1;
            stats.sum = (stats.sum || 0) + timing;
            stats.sumSq = (stats.sumSq || 0) + timing * timing;
        }

        if (scoreSide === 2) {
            root.host.gameplayJudgeNowValue2 = root.nowJudgeValue(judgement);
        } else {
            root.host.gameplayJudgeNowValue1 = root.nowJudgeValue(judgement);
        }

        let laneValue = root.laneJudgeValue(judgement, timing);
        if (laneValue > 0) {
            root.setGameplayJudgeLaneValue(scoreSide, hit, laneValue);
        }
    }

    function gameplayJudgeTimingNumber(num: var, side: var) : var {
        return root.judgeTimingNumberFromCounts(
            num,
            side === 2 ? root.host.gameplayJudgeTimingCounts2 : root.host.gameplayJudgeTimingCounts1);
    }

    function resultJudgeTimingNumber(num: var, side: var) : var {
        return root.judgeTimingNumberFromCounts(num, root.resultJudgeTimingCounts(side));
    }

    function judgeTimingCount(counts: var, bucket: var, early: var) : var {
        if (bucket < 0 || !counts) {
            return 0;
        }
        let source = early ? counts.early : counts.late;
        return source && bucket < source.length ? (source[bucket] || 0) : 0;
    }

    function timingBucketCountsInTotal(bucket: var) : var {
        return bucket >= 0 && bucket <= 3;
    }

    function totalJudgeTimingCount(counts: var, early: var) : var {
        let total = 0;
        for (let i = 0; i <= 3; ++i) {
            if (root.timingBucketCountsInTotal(i)) {
                total += root.judgeTimingCount(counts, i, early);
            }
        }
        return total;
    }

    function judgeTimingNumberFromCounts(num: var, counts: var) : var {
        if (num >= 410 && num <= 419) {
            let bucket = Math.floor((num - 410) / 2);
            let early = (num - 410) % 2 === 0;
            return root.judgeTimingCount(counts, bucket, early);
        }
        switch (num) {
        case 421:
            return root.judgeTimingCount(counts, 5, true);
        case 422:
            return root.judgeTimingCount(counts, 5, false);
        case 423:
            return root.totalJudgeTimingCount(counts, true);
        case 424:
            return root.totalJudgeTimingCount(counts, false);
        default:
            return 0;
        }
    }

    function gameplayJudgeTimingStats(side: var) : var {
        let stats = side === 2 ? root.host.gameplayJudgeTimingStats2 : root.host.gameplayJudgeTimingStats1;
        return stats || root.emptyJudgeTimingStats();
    }

    function timingStatsMean(stats: var) : var {
        let count = stats ? Number(stats.count || 0) : 0;
        return count > 0 ? Number(stats.sum || 0) / count : 0;
    }

    function timingStatsStdDev(stats: var) : var {
        let count = stats ? Number(stats.count || 0) : 0;
        if (count <= 0) {
            return 0;
        }
        let mean = root.timingStatsMean(stats);
        return Math.sqrt(Math.max(0, Number(stats.sumSq || 0) / count - mean * mean));
    }

    function resultCacheKey(score: var) : var {
        if (!score) {
            return "";
        }
        let replay = score.replayData || null;
        let events = replay && replay.hitEvents ? replay.hitEvents : [];
        let result = score.result || null;
        let guid = replay && replay.guid ? replay.guid : (result && result.guid ? result.guid : "");
        return String(guid) + ":" + events.length;
    }

    function resultJudgeTimingCounts(side: var) : var {
        let score = root.resultScore(side);
        let key = root.resultCacheKey(score);
        if (key.length === 0) {
            return root.emptyJudgeTimingCounts();
        }

        let cache = root.resultJudgeTimingCountsCache || {};
        if (cache[key]) {
            return cache[key];
        }

        let counts = root.emptyJudgeTimingCounts();
        let replay = score && score.replayData ? score.replayData : null;
        if (replay && replay.earlyTimingCounts && replay.lateTimingCounts) {
            counts.early = replay.earlyTimingCounts;
            counts.late = replay.lateTimingCounts;
            cache[key] = counts;
            return counts;
        }

        let events = replay && replay.hitEvents ? replay.hitEvents : [];
        for (let hit of events || []) {
            let judgement = root.host.gameplayJudgementFromHit(hit);
            let bucket = root.judgementTimingBucket(judgement);
            if (bucket < 0) {
                continue;
            }
            let sideCounts = root.hitDeviationNanos(hit) < 0 ? counts.early : counts.late;
            sideCounts[bucket] = (sideCounts[bucket] || 0) + 1;
        }

        cache[key] = counts;
        return counts;
    }

    function resultTimingStats(side: var) : var {
        let score = root.resultScore(side);
        let key = root.resultCacheKey(score);
        if (key.length === 0) {
            return { averageDuration: 0, average: 0, stddev: 0 };
        }

        let cache = root.resultTimingStatsCache || {};
        if (cache[key]) {
            return cache[key];
        }

        let events = score && score.replayData && score.replayData.hitEvents
            ? score.replayData.hitEvents
            : [];
        let count = 0;
        let absSum = 0;
        let sum = 0;
        let sumSq = 0;
        for (let hit of events || []) {
            if (!hit || !hit.noteRemoved) {
                continue;
            }
            let judgement = root.host.gameplayJudgementFromHit(hit);
            if (judgement !== Judgement.Poor
                    && (judgement < Judgement.Bad || judgement > Judgement.Perfect)) {
                continue;
            }
            let ms = root.hitDeviationMillis(hit);
            if (ms < -150 || ms > 150) {
                continue;
            }
            ++count;
            absSum += Math.abs(ms);
            sum += ms;
            sumSq += ms * ms;
        }

        let stats = { averageDuration: 0, average: 0, stddev: 0 };
        if (count > 0) {
            let average = sum / count;
            stats = {
                averageDuration: absSum / count,
                average: average,
                stddev: Math.sqrt(Math.max(0, sumSq / count - average * average))
            };
        }

        cache[key] = stats;
        return stats;
    }

    function signedAfterDot(value: var) : var {
        let scaled = Math.floor(Math.abs(value || 0) * 100) % 100;
        return value < 0 ? -scaled : scaled;
    }

    function resultGaugeInfos(side: var) : var {
        let score = root.resultScore(side);
        let infos = score && score.gaugeHistory ? score.gaugeHistory.gaugeInfo : [];
        if (!infos || infos.length === 0) {
            return [];
        }
        return infos;
    }

    function defaultResultGaugeInfo(infos: var) : var {
        if (!infos || infos.length === 0) {
            return null;
        }

        let best = null;
        let fallback = infos[infos.length - 1];
        for (let i = 0; i < infos.length; ++i) {
            let info = infos[i];
            let history = info && info.gaugeHistory ? info.gaugeHistory : [];
            let last = history && history.length > 0 ? history[history.length - 1].gauge : 0;
            if (!best && root.host && root.host.gaugeAboveThreshold(last, info.threshold)) {
                best = info;
            }
            if (info && !info.courseGauge) {
                fallback = info;
            }
        }
        return best || fallback;
    }

    function resultGaugeInfoKey(info: var) : var {
        return String(info && info.name ? info.name : "").toUpperCase();
    }

    function resultGaugeIndexForInfo(infos: var, selected: var) : var {
        if (!infos || !selected) {
            return -1;
        }
        let selectedKey = root.resultGaugeInfoKey(selected);
        for (let i = 0; i < infos.length; ++i) {
            if (root.resultGaugeInfoKey(infos[i]) === selectedKey) {
                return i;
            }
        }
        return -1;
    }

    function resultGaugeSelectionIndex(side: var) : var {
        return side === 2 ? root.resultGaugeSelectionIndex2 : root.resultGaugeSelectionIndex1;
    }

    function setResultGaugeSelectionIndex(side: var, index: var) : void {
        let next = Math.max(0, Math.floor(index || 0));
        if (side === 2) {
            root.resultGaugeSelectionIndex2 = next;
        } else {
            root.resultGaugeSelectionIndex1 = next;
        }
    }

    function resultGaugeInfo(side: var) : var {
        let infos = root.resultGaugeInfos(side);
        let index = root.resultGaugeSelectionIndex(side);
        if (index >= 0 && index < infos.length) {
            return infos[index];
        }
        return root.defaultResultGaugeInfo(infos);
    }

    function resultGaugeName(side: var) : var {
        let info = root.resultGaugeInfo(side);
        return String(info && info.name ? info.name : "").toUpperCase();
    }

    function cycleResultGauge(side: var, delta: var) : var {
        let infos = root.resultGaugeInfos(side);
        if (!infos || infos.length === 0) {
            return false;
        }

        let cycleInfos = infos.slice();
        cycleInfos.reverse();
        let currentInfo = root.resultGaugeInfo(side);
        let current = root.resultGaugeIndexForInfo(cycleInfos, currentInfo);
        if (current < 0) {
            current = 0;
        }

        let step = Math.round(delta === undefined ? 1 : delta);
        if (step === 0 || isNaN(step)) {
            step = 1;
        }
        let next = ((current + step) % cycleInfos.length + cycleInfos.length) % cycleInfos.length;
        let rawIndex = root.resultGaugeIndexForInfo(infos, cycleInfos[next]);
        if (rawIndex < 0) {
            return false;
        }
        root.setResultGaugeSelectionIndex(side, rawIndex);
        return true;
    }

    function resultGaugeValue(side: var) : var {
        let info = root.resultGaugeInfo(side);
        let history = info && info.gaugeHistory ? info.gaugeHistory : [];
        return history && history.length > 0 ? (history[history.length - 1].gauge || 0) : 0;
    }

    function gaugeAfterDot(value: var) : var {
        let scaled = Math.max(0, value || 0) * 10;
        return scaled > 0 && scaled < 1 ? 1 : Math.floor(scaled) % 10;
    }

    function resultExScore(result: var) : var {
        return result ? Math.floor(result.points || 0) : 0;
    }

    function resultHasPlayedScore(result: var) : var {
        if (!result) {
            return false;
        }
        return String(result.clearType || "FAILED").toUpperCase() !== "NOPLAY";
    }

    function resultLr2Score(result: var) : var {
        let totalNotes = root.resultTotalNotes(result);
        if (totalNotes <= 0) {
            return 0;
        }
        let pgreat = root.resultJudgementCount(result, Judgement.Perfect);
        let great = root.resultJudgementCount(result, Judgement.Great);
        let good = root.resultJudgementCount(result, Judgement.Good);
        return Math.floor((good + (great + pgreat * 2) * 2) * 50000 / totalNotes);
    }

    function resultScorePrint(result: var) : var {
        let value = root.resultLr2Score(result);
        let chartData = root.resultChartData();
        let keymode = chartData ? chartData.keymode : (result ? result.keymode : 0);
        return keymode === 7 || keymode === 14 ? value : Math.floor(value / 20) * 10;
    }

    function resultRateInteger(result: var) : var {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(root.resultExScore(result) * 100 / denominator) : 0;
    }

    function resultRateDecimal(result: var) : var {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(root.resultExScore(result) * 10000 / denominator) % 100 : 0;
    }

    function resultScoreRateInteger(points: var, result: var) : var {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(points * 100 / denominator) : 0;
    }

    function resultScoreRateDecimal(points: var, result: var) : var {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(points * 10000 / denominator) % 100 : 0;
    }

    function resultRawRank(result: var) : var {
        if (!root.resultHasPlayedScore(result)) {
            return -1;
        }
        let denominator = root.resultTotalNotes(result) * 2;
        if (denominator <= 0) {
            return 1;
        }
        return Math.max(1, Math.floor(root.resultExScore(result) * 9 / denominator));
    }

    function resultRankDelta(result: var) : var {
        let totalNotes = root.resultTotalNotes(result);
        let perfectScore = totalNotes * 2;
        let exScore = root.resultExScore(result);
        if (totalNotes <= 0 || exScore === perfectScore) {
            return 0;
        }
        let rank = Math.floor(exScore * 9 / perfectScore);
        rank = Math.max(1, Math.min(8, rank));
        return exScore - Math.floor(perfectScore * (rank + 1) / 9);
    }

    function resultRankOptionForResult(result: var, baseOption: var) : var {
        if (!root.resultHasPlayedScore(result)) {
            return baseOption + 8;
        }
        if (root.resultExScore(result) <= 0 || root.resultTotalNotes(result) <= 0) {
            return baseOption + 8;
        }
        let rank = root.resultRawRank(result);
        if (rank >= 8) {
            return baseOption;
        }
        if (rank <= 1) {
            return baseOption + 7;
        }
        return baseOption + (8 - rank);
    }

    function resultOldScores(side: var) : var {
        return side === 2 ? root.resultOldScores2 : root.resultOldScores1;
    }

    function resultBestScoreByPoints(scores: var) : var {
        let best = null;
        let bestRate = -1;
        let bestHasMaxPoints = false;
        for (let score of scores || []) {
            let result = score && score.result ? score.result : null;
            if (!root.resultHasPlayedScore(result)) {
                continue;
            }
            let maxPoints = result ? (result.maxPoints || 0) : 0;
            let hasMaxPoints = maxPoints > 0;
            let rate = hasMaxPoints ? (result.points || 0) / maxPoints : 0;
            if (rate < bestRate) {
                continue;
            }
            if (Math.abs(rate - bestRate) <= 0.0000001
                    && (bestHasMaxPoints || !hasMaxPoints)) {
                continue;
            }
            best = score;
            bestRate = rate;
            bestHasMaxPoints = hasMaxPoints;
        }
        return best;
    }

    function resultOldBestScore(side: var) : var {
        return root.resultBestScoreByPoints(root.resultOldScores(side));
    }

    function resultLastOldScore(side: var) : var {
        let scores = root.resultOldScores(side) || [];
        return scores.length > 0 ? scores[0] : null;
    }

    function resultTargetSavedScore(side: var) : var {
        let profile = root.resultProfile(side);
        let vars = profile && profile.vars ? profile.vars.generalVars : null;
        switch (vars ? vars.scoreTarget : ScoreTarget.BestScore) {
        case ScoreTarget.LastScore:
            return root.resultLastOldScore(side);
        case ScoreTarget.BestScore:
            return root.resultOldBestScore(side);
        default:
            return null;
        }
    }

    function resultUsesNextRankTarget(side: var) : var {
        let profile = root.resultProfile(side);
        let vars = profile && profile.vars ? profile.vars.generalVars : null;
        return vars && vars.scoreTarget === ScoreTarget.NextRank;
    }

    function resultTargetFraction(side: var) : var {
        let profile = root.resultProfile(side);
        let vars = profile && profile.vars ? profile.vars.generalVars : null;
        return vars ? (vars.targetScoreFraction || 0) : 0;
    }

    function resultTargetPoints(side: var) : var {
        let targetScore = root.resultTargetSavedScore(side);
        if (targetScore && targetScore.result) {
            return root.resultExScore(targetScore.result);
        }
        let current = root.resultData(side);
        if (root.resultUsesNextRankTarget(side)) {
            return current
                ? root.host.nextRankTargetPoints(
                    root.resultExScore(root.resultOldBestResult(side)),
                    current.maxPoints || 0)
                : 0;
        }
        return current ? Math.floor((current.maxPoints || 0) * root.resultTargetFraction(side)) : 0;
    }

    function resultHighScorePoints(side: var) : var {
        return root.resultExScore(root.resultOldBestResult(side));
    }

    function resultTargetMaxPoints(side: var) : var {
        let current = root.resultData(side);
        return current ? Math.max(1, current.maxPoints || 1) : 1;
    }

    function resultOldBestResult(side: var) : var {
        let score = root.resultOldBestScore(side);
        return score && score.result ? score.result : null;
    }

    function resultUpdatedBestResult(side: var) : var {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        if (!current) {
            return old;
        }
        if (!old || (current.maxPoints || 0) <= 0 || (old.maxPoints || 0) <= 0) {
            return current;
        }
        return current.points / current.maxPoints >= old.points / old.maxPoints ? current : old;
    }

    function resultScoreImproved(side: var) : var {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || root.resultExScore(current) > root.resultExScore(old));
    }

    function resultComboImproved(side: var) : var {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || (current.maxCombo || 0) > (old.maxCombo || 0));
    }

    function resultBadPoorImproved(side: var) : var {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || root.resultBadPoor(current) < root.resultBadPoor(old));
    }

    function updateResultOldScores() : var {
        if (!root.resultScreenActive) {
            return;
        }

        let request = ++root.resultOldScoresRequest;
        root.resultOldScores1 = [];
        root.resultOldScores2 = [];

        for (let side = 1; side <= 2; ++side) {
            let score = root.resultScore(side);
            let profile = root.resultProfile(side);
            let scoreDb = profile ? profile.scoreDb : null;
            if (!score || !score.result || !scoreDb) {
                continue;
            }

            let currentGuid = score.result.guid || "";
            if (root.host.course && root.host.course.identifier) {
                let courseId = root.host.course.identifier;
                scoreDb.getScoresForCourseId([courseId]).then(result => {
                    if (request !== root.resultOldScoresRequest) {
                        return;
                    }
                    let list = result && result.scores ? (result.scores[courseId] || []) : [];
                    let filtered = list.filter(oldScore => oldScore && oldScore.result
                        && oldScore.result.guid !== currentGuid);
                    if (side === 2) {
                        root.resultOldScores2 = filtered;
                    } else {
                        root.resultOldScores1 = filtered;
                    }
                });
            } else {
                let chartData = root.resultChartData();
                let md5 = chartData && chartData.md5 ? String(chartData.md5).toUpperCase() : "";
                if (md5.length === 0) {
                    continue;
                }
                scoreDb.getScoresForMd5([md5]).then(result => {
                    if (request !== root.resultOldScoresRequest) {
                        return;
                    }
                    let list = result && result.scores
                        ? (result.scores[md5] || result.scores[md5.toLowerCase()] || [])
                        : [];
                    let filtered = list.filter(oldScore => oldScore && oldScore.result
                        && oldScore.result.guid !== currentGuid);
                    if (side === 2) {
                        root.resultOldScores2 = filtered;
                    } else {
                        root.resultOldScores1 = filtered;
                    }
                });
            }
        }
    }
}
