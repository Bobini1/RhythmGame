pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host
    required property var selectContext

    property var resultOldScores1: []
    property var resultOldScores2: []
    property int resultOldScoresRevision: 0
    property int resultOldScoresRequest: 0
    property int resultTimer151SkinTime: -1
    property int resultTimer152SkinTime: -1
    property int resultGraphStartSkinTime: 500
    property int resultGraphEndSkinTime: 2000
    property var resultTimingStatsCache: ({})
    property var resultJudgeTimingCountsCache: ({})

    function isResultScreen() {
        return root.host.effectiveScreenKey === "result"
            || root.host.effectiveScreenKey === "courseResult";
    }

    function resultScore(side) {
        return root.host.scores && root.host.scores.length >= side
            ? root.host.scores[side - 1]
            : null;
    }

    function resultData(side) {
        let score = root.resultScore(side);
        return score && score.result ? score.result : null;
    }

    function resultProfile(side) {
        return root.host.profiles && root.host.profiles.length >= side
            ? root.host.profiles[side - 1]
            : (Rg.profileList ? Rg.profileList.mainProfile : null);
    }

    function resultChartData() {
        if (root.host.effectiveScreenKey === "courseResult") {
            return null;
        }
        if (root.host.chartData) {
            return root.host.chartData;
        }
        return root.host.chartDatas && root.host.chartDatas.length > 0
            ? root.host.chartDatas[0]
            : null;
    }

    function displayChartData() {
        if (root.isResultScreen()) {
            return root.resultChartData();
        }
        if (root.host.effectiveScreenKey === "select") {
            return root.selectContext.selectedChartData();
        }
        if (root.host.isGameplayScreen()) {
            return root.host.gameplayChartData();
        }
        return root.host.chart && root.host.chart.chartData ? root.host.chart.chartData : null;
    }

    function resultClearOption() {
        let result = root.resultData(1);
        let clearType = result ? String(result.clearType || "FAILED") : "FAILED";
        return clearType !== "FAILED" && clearType !== "NOPLAY" ? 90 : 91;
    }

    function resultTotalNotes(result) {
        if (!result) {
            return 0;
        }
        let maxPoints = Math.floor(result.maxPoints || 0);
        return maxPoints > 0 ? Math.floor(maxPoints / 2) : Math.max(0, result.maxHits || 0);
    }

    function resultJudgementCount(result, judgement) {
        let counts = result && result.judgementCounts ? result.judgementCounts : [];
        return judgement >= 0 && judgement < counts.length ? (counts[judgement] || 0) : 0;
    }

    function resultPoorCount(result) {
        return root.resultJudgementCount(result, Judgement.Poor)
            + root.resultJudgementCount(result, Judgement.EmptyPoor);
    }

    function resultBadPoor(result) {
        return root.resultJudgementCount(result, Judgement.Bad) + root.resultPoorCount(result);
    }

    function emptyJudgeTimingCounts() {
        return {
            early: [0, 0, 0, 0, 0, 0],
            late: [0, 0, 0, 0, 0, 0]
        };
    }

    function judgementTimingBucket(judgement) {
        switch (judgement) {
        case Judgement.Perfect:
            return 0;
        case Judgement.Great:
            return 1;
        case Judgement.Good:
            return 2;
        case Judgement.Bad:
            return 3;
        case Judgement.Poor:
            return 4;
        case Judgement.EmptyPoor:
            return 5;
        default:
            return -1;
        }
    }

    function hitDeviationNanos(hit) {
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

    function hitDeviationMillis(hit) {
        return Math.round(-root.hitDeviationNanos(hit) / 1000000);
    }

    function judgementUpdatesJudgeTimingValue(judgement) {
        return judgement >= Judgement.Bad && judgement <= Judgement.Perfect;
    }

    function cloneJudgeTimingCounts(counts) {
        if (!counts || !counts.early || !counts.late) {
            return root.emptyJudgeTimingCounts();
        }
        return {
            early: counts.early.slice(),
            late: counts.late.slice()
        };
    }

    function emptyJudgeLaneValues() {
        return [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    }

    function nowJudgeValue(judgement) {
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

    function laneJudgeValue(judgement, timing) {
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

    function setGameplayJudgeLaneValue(scoreSide, hit, value) {
        let lane = root.host.gameplayLr2LaneForHit(scoreSide, hit);
        if (lane < 0) {
            return;
        }

        let valuesName = scoreSide === 2 ? "gameplayJudgeLaneValues2" : "gameplayJudgeLaneValues1";
        let values = (root.host[valuesName] && root.host[valuesName].length)
            ? root.host[valuesName].slice()
            : root.emptyJudgeLaneValues();
        if (lane >= values.length) {
            values.length = lane + 1;
            for (let i = 0; i < values.length; ++i) {
                values[i] = values[i] || 0;
            }
        }
        values[lane] = value;
        root.host[valuesName] = values;
    }

    function gameplayJudgeValueForId(num) {
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

    function recordGameplayJudgeTiming(scoreSide, hit) {
        let judgement = root.host.gameplayJudgementFromHit(hit);
        let bucket = root.judgementTimingBucket(judgement);
        if (bucket < 0) {
            return;
        }

        let countsName = scoreSide === 2 ? "gameplayJudgeTimingCounts2" : "gameplayJudgeTimingCounts1";
        let counts = root.cloneJudgeTimingCounts(root.host[countsName]);
        let side = root.hitDeviationNanos(hit) < 0 ? counts.early : counts.late;
        side[bucket] = (side[bucket] || 0) + 1;
        root.host[countsName] = counts;

        let timing = root.hitDeviationMillis(hit);
        if (root.judgementUpdatesJudgeTimingValue(judgement)) {
            let timingName = scoreSide === 2 ? "gameplayLastJudgeTiming2" : "gameplayLastJudgeTiming1";
            root.host[timingName] = timing;
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

    function judgeTimingCount(counts, bucket, early) {
        if (bucket < 0 || !counts) {
            return 0;
        }
        let source = early ? counts.early : counts.late;
        return source && bucket < source.length ? (source[bucket] || 0) : 0;
    }

    function judgeTimingNumberFromCounts(num, counts) {
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
        case 423: {
            let total = 0;
            for (let i = 1; i <= 5; ++i) {
                total += root.judgeTimingCount(counts, i, true);
            }
            return total;
        }
        case 424: {
            let total = 0;
            for (let i = 1; i <= 5; ++i) {
                total += root.judgeTimingCount(counts, i, false);
            }
            return total;
        }
        default:
            return 0;
        }
    }

    function resultCacheKey(score) {
        if (!score) {
            return "";
        }
        let replay = score.replayData || null;
        let events = replay && replay.hitEvents ? replay.hitEvents : [];
        let result = score.result || null;
        let guid = replay && replay.guid ? replay.guid : (result && result.guid ? result.guid : "");
        return String(guid) + ":" + events.length;
    }

    function resultJudgeTimingCounts(side) {
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
        let events = score && score.replayData && score.replayData.hitEvents
            ? score.replayData.hitEvents
            : [];
        for (let hit of events || []) {
            let judgement = root.host.gameplayJudgementFromHit(hit);
            let bucket = root.judgementTimingBucket(judgement);
            if (bucket < 0) {
                continue;
            }
            let sideCounts = root.hitDeviationNanos(hit) < 0 ? counts.early : counts.late;
            sideCounts[bucket] = (sideCounts[bucket] || 0) + 1;
        }

        let next = Object.assign({}, cache);
        next[key] = counts;
        root.resultJudgeTimingCountsCache = next;
        return counts;
    }

    function resultTimingStats(side) {
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
            let judgement = root.host.gameplayJudgementFromHit(hit);
            if (judgement < Judgement.Bad || judgement > Judgement.Perfect) {
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

        let next = Object.assign({}, cache);
        next[key] = stats;
        root.resultTimingStatsCache = next;
        return stats;
    }

    function signedAfterDot(value) {
        let scaled = Math.floor(Math.abs(value || 0) * 100) % 100;
        return value < 0 ? -scaled : scaled;
    }

    function resultGaugeInfo(side) {
        let score = root.resultScore(side);
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

    function resultGaugeValue(side) {
        let info = root.resultGaugeInfo(side);
        let history = info && info.gaugeHistory ? info.gaugeHistory : [];
        return history && history.length > 0 ? (history[history.length - 1].gauge || 0) : 0;
    }

    function gaugeAfterDot(value) {
        let scaled = Math.max(0, value || 0) * 10;
        return scaled > 0 && scaled < 1 ? 1 : Math.floor(scaled) % 10;
    }

    function resultExScore(result) {
        return result ? Math.floor(result.points || 0) : 0;
    }

    function resultLr2Score(result) {
        let totalNotes = root.resultTotalNotes(result);
        if (totalNotes <= 0) {
            return 0;
        }
        let pgreat = root.resultJudgementCount(result, Judgement.Perfect);
        let great = root.resultJudgementCount(result, Judgement.Great);
        let good = root.resultJudgementCount(result, Judgement.Good);
        return Math.floor((good + (great + pgreat * 2) * 2) * 50000 / totalNotes);
    }

    function resultScorePrint(result) {
        let value = root.resultLr2Score(result);
        let chartData = root.resultChartData();
        let keymode = chartData ? chartData.keymode : (result ? result.keymode : 0);
        return keymode === 7 || keymode === 14 ? value : Math.floor(value / 20) * 10;
    }

    function resultRateInteger(result) {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(root.resultExScore(result) * 100 / denominator) : 0;
    }

    function resultRateDecimal(result) {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(root.resultExScore(result) * 10000 / denominator) % 100 : 0;
    }

    function resultScoreRateInteger(points, result) {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(points * 100 / denominator) : 0;
    }

    function resultScoreRateDecimal(points, result) {
        let denominator = root.resultTotalNotes(result) * 2;
        return denominator > 0 ? Math.floor(points * 10000 / denominator) % 100 : 0;
    }

    function resultRawRank(result) {
        let denominator = root.resultTotalNotes(result) * 2;
        if (!result || denominator <= 0 || root.resultExScore(result) <= 0) {
            return -1;
        }
        return Math.floor(root.resultExScore(result) * 9 / denominator);
    }

    function resultRankDelta(result) {
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

    function resultRankOptionForResult(result, baseOption) {
        let rank = root.resultRawRank(result);
        if (rank < 0) {
            return baseOption + 8;
        }
        if (rank >= 8) {
            return baseOption;
        }
        if (rank <= 1) {
            return baseOption + 7;
        }
        return baseOption + (8 - rank);
    }

    function resultOldScores(side) {
        root.resultOldScoresRevision;
        return side === 2 ? root.resultOldScores2 : root.resultOldScores1;
    }

    function resultBestScoreByPoints(scores) {
        let best = null;
        let bestRate = -1;
        for (let score of scores || []) {
            let result = score && score.result ? score.result : null;
            let maxPoints = result ? (result.maxPoints || 0) : 0;
            if (maxPoints <= 0) {
                continue;
            }
            let rate = (result.points || 0) / maxPoints;
            if (rate > bestRate) {
                best = score;
                bestRate = rate;
            }
        }
        return best;
    }

    function resultOldBestScore(side) {
        return root.resultBestScoreByPoints(root.resultOldScores(side));
    }

    function resultLastOldScore(side) {
        root.resultOldScoresRevision;
        let scores = root.resultOldScores(side) || [];
        return scores.length > 0 ? scores[0] : null;
    }

    function resultTargetSavedScore(side) {
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

    function resultTargetFraction(side) {
        let profile = root.resultProfile(side);
        let vars = profile && profile.vars ? profile.vars.generalVars : null;
        return vars ? (vars.targetScoreFraction || 0) : 0;
    }

    function resultTargetPoints(side) {
        let targetScore = root.resultTargetSavedScore(side);
        if (targetScore && targetScore.result) {
            return root.resultExScore(targetScore.result);
        }
        let current = root.resultData(side);
        return current ? Math.floor((current.maxPoints || 0) * root.resultTargetFraction(side)) : 0;
    }

    function resultHighScorePoints(side) {
        return root.resultExScore(root.resultOldBestResult(side));
    }

    function resultTargetMaxPoints(side) {
        let current = root.resultData(side);
        return current ? Math.max(1, current.maxPoints || 1) : 1;
    }

    function resultOldBestResult(side) {
        let score = root.resultOldBestScore(side);
        return score && score.result ? score.result : null;
    }

    function resultUpdatedBestResult(side) {
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

    function resultScoreImproved(side) {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || root.resultExScore(current) > root.resultExScore(old));
    }

    function resultComboImproved(side) {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || (current.maxCombo || 0) > (old.maxCombo || 0));
    }

    function resultBadPoorImproved(side) {
        let current = root.resultData(side);
        let old = root.resultOldBestResult(side);
        return !!current && (!old || root.resultBadPoor(current) < root.resultBadPoor(old));
    }

    function updateResultOldScores() {
        if (!root.isResultScreen()) {
            return;
        }

        let request = ++root.resultOldScoresRequest;
        root.resultOldScores1 = [];
        root.resultOldScores2 = [];
        root.resultOldScoresRevision++;

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
                    root.resultOldScoresRevision++;
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
                    root.resultOldScoresRevision++;
                });
            }
        }
    }
}
