pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: resolver

    required property var screenRoot
    required property var selectContext
    required property var playContext

    readonly property var root: screenRoot

    function optionText(labels, index) {
        return index >= 0 && index < labels.length ? labels[index] : "";
    }

    function clearLabelForLamp(lamp) {
        switch (lamp) {
        case 1:
            return "FAILED";
        case 2:
            return "EASY";
        case 3:
            return "NORMAL";
        case 4:
            return "HARD";
        case 5:
            return "FULLCOMBO";
        default:
            return "NO PLAY";
        }
    }

    function courseStage(index) {
        return index >= 0 && index < root.selectedCourseStages.length
            ? root.selectedCourseStages[index]
            : null;
    }

    function chartTitle(chart) {
        if (typeof chart === "string") {
            return chart;
        }
        return chart ? (chart.title || "") : "";
    }

    function chartSubtitle(chart) {
        if (typeof chart === "string") {
            return "";
        }
        return chart ? (chart.subtitle || "") : "";
    }

    function lr2SelectOptionText(st) {
        switch (st) {
        case 50:
            return root.lr2SkinPreviewTitle();
        case 51:
            return root.lr2SkinPreviewMaker();
        case 63:
            return root.optionText(root.lr2RandomLabels, root.lr2RandomIndexP1);
        case 64:
            return root.optionText(root.lr2RandomLabels, root.lr2RandomIndexP2);
        case 65:
            return root.optionText(root.lr2GaugeLabels, root.lr2GaugeIndexP1);
        case 66:
            return root.optionText(root.lr2GaugeLabels, root.lr2GaugeIndexP2);
        case 67:
            return "OFF";
        case 68:
            return "OFF";
        case 69:
            return root.optionText(root.lr2DpOptionLabels, root.lr2DpOptionIndex);
        case 70:
            return root.optionText(["OFF", "FLIP"], root.lr2FlipIndex);
        case 71:
            return root.optionText(["OFF", "ON"], root.lr2ScoreGraphIndex);
        case 72:
            return root.optionText(root.lr2GhostLabels, root.lr2GhostIndex);
        case 73:
            return root.optionText(["OFF", "ON"], root.lr2LaneCoverIndex);
        case 74:
            return root.optionText(root.lr2HiSpeedFixLabels, root.lr2HiSpeedFixIndex);
        case 75:
            return root.optionText(root.lr2BgaSizeLabels, root.lr2BgaSizeIndex);
        case 76:
            return root.optionText(["ON", "OFF"], root.lr2BgaIndex);
        case 77:
            return "32BIT";
        case 78:
            return "OFF";
        case 79:
            return globalRoot.isFullScreen() ? "FULLSCREEN" : "WINDOW";
        case 80:
            return "OFF";
        case 81:
            return root.optionText(root.lr2ReplayLabels, root.lr2ReplayType);
        case 82:
            return "MISSION COMPLETED";
        case 83:
            return "";
        case 84:
            return root.optionText(root.lr2HidSudLabels, root.lr2HidSudIndexP1);
        case 85:
            return root.optionText(root.lr2HidSudLabels, root.lr2HidSudIndexP2);
        case 120:
            return Rg.profileList.mainProfile.vars.generalVars.name || "";
        case 121:
        case 122:
        case 123:
        case 124:
            return "";
        case 130:
            return root.clearLabelForLamp(selectContext.entryLamp(selectContext.current));
        case 131:
        case 132:
        case 133:
        case 134:
            return "";
        case 150:
        case 151:
        case 152:
        case 153:
        case 154:
        case 155:
        case 156:
        case 157:
        case 158:
        case 159:
            return root.chartTitle(root.courseStage(st - 150));
        case 160:
        case 161:
        case 162:
        case 163:
        case 164:
            return root.chartSubtitle(root.courseStage(st - 160));
        case 170:
            return root.effectiveScreenKey === "select" ? selectContext.entryDisplayName(selectContext.current, true) : "";
        case 171:
            return "TITLE";
        case 172:
            return "ARTIST";
        case 173:
            return "GENRE";
        case 174:
            return "TAG";
        case 183:
            return "";
        case 190:
            return "OFF";
        case 191:
        case 192:
        case 193:
        case 194:
        case 195:
            return "NO LIMIT";
        case 196:
            return "RANDOM";
        case 198:
        case 199:
            return "GROOVE";
        default:
            if (st >= 100 && st < 110) {
                return root.lr2SkinSettingName(st - 100);
            }
            if (st >= 110 && st < 120) {
                return root.lr2SkinSettingValueText(st - 110);
            }
            return "";
        }
    }

    function resolveText(st) {
        let revision = root.effectiveScreenKey === "select"
            ? selectContext.selectionRevision + selectContext.scoreRevision + selectContext.listRevision + root.lr2SkinSettingsRevision
            : (root.isResultScreen() ? root.resultOldScoresRevision : root.gameplayRevision);
        let chartData = root.displayChartData();
        let currentEntry = root.effectiveScreenKey === "select" ? selectContext.current : null;
        switch (st) {
        case 1:
            return root.optionText(root.lr2TargetLabels, root.lr2ScoreTargetIndex);
        case 2:
            return Rg.profileList.mainProfile.vars.generalVars.name || "";
        case 10:
            if (chartData) {
                return (chartData.title || "").replace(/\r\n|\n|\r/g, " ");
            }
            if (root.effectiveScreenKey === "select") {
                return selectContext.entryMainTitle(currentEntry);
            }
            return root.course ? (root.course.name || "") : (root.chart && root.chart.course ? root.chart.course.name : "");
        case 11:
            return chartData ? (chartData.subtitle || "") : "";
        case 12:
            return chartData ? ((chartData.title || "") + (chartData.subtitle ? " " + chartData.subtitle : "")) : selectContext.entryDisplayName(currentEntry, true);
        case 13:
            return chartData ? (chartData.genre || "") : "Course";
        case 14:
            return chartData ? (chartData.artist || "") : "";
        case 15:
            return chartData ? (chartData.subartist || "") : "";
        case 16:
            if (!chartData) {
                return "";
            }
            return (chartData.artist || "")
                + (chartData.subartist ? " " + chartData.subartist : "");
        case 17:
            return chartData ? String(chartData.playLevel || "") : "";
        case 18:
            return chartData ? String(selectContext.entryDifficulty(chartData) || "") : "";
        case 20:
            if (root.effectiveScreenKey === "select") {
                return selectContext.entryMainTitle(currentEntry);
            }
            return root.isResultScreen() ? "" : "";
        case 21:
            return root.effectiveScreenKey === "select" ? selectContext.entrySubtitle(currentEntry) : "";
        case 23:
            return root.effectiveScreenKey === "select" ? selectContext.entryGenre(currentEntry) : "";
        case 24:
            return root.effectiveScreenKey === "select" ? selectContext.entryArtist(currentEntry) : "";
        case 25:
            return root.effectiveScreenKey === "select" ? selectContext.entrySubartist(currentEntry) : "";
        case 26:
            return "";
        case 27:
            return chartData ? String(chartData.playLevel || "") : "";
        case 28:
            return chartData ? String(selectContext.entryDifficulty(chartData) || "") : "";
        case 29:
            return chartData ? String(chartData.rank || "") : "";
        case 30:
            if (root.effectiveScreenKey !== "select") {
                return "";
            }
            if (selectContext.searchText.length > 0) {
                return selectContext.searchText;
            }
            if (root.selectSearchHasFocus()) {
                return "";
            }
            let folderName = selectContext.currentFolderDisplayName();
            return folderName.length > 0 ? folderName : root.lr2SearchPlaceholderText;
        case 60:
            return root.effectiveScreenKey === "select" ? selectContext.keyFilterLabel() : "ALL";
        case 61:
            return root.effectiveScreenKey === "select" ? selectContext.sortLabel() : "DIRECTORY";
        case 62:
            return root.effectiveScreenKey === "select" ? selectContext.difficultyFilterLabel() : "ALL";
        case 120:
        case 121:
        case 122:
        case 123:
        case 124:
        case 125:
        case 126:
        case 127:
        case 128:
        case 129: {
            let rankingName = root.lr2RankingEntryName(st - 120);
            return rankingName.length > 0 ? rankingName : root.lr2SelectOptionText(st);
        }
        case 1000:
            return root.effectiveScreenKey === "select" ? selectContext.currentFolderDisplayName() : "";
        case 1001:
            return root.effectiveScreenKey === "select" ? selectContext.currentTableName() : "";
        case 1002:
            return root.effectiveScreenKey === "select" ? selectContext.currentTableLevelName() : "";
        case 1003:
            return root.effectiveScreenKey === "select" ? selectContext.currentTableFullName() : "";
        default:
            return root.effectiveScreenKey === "select" ? root.lr2SelectOptionText(st) : "";
        }
    }

    function resolveGameplayNumber(num) {
        root.gameplayRevision;
        let chartData = root.gameplayChartData();
        let p1 = root.gameplayPlayer(1);
        let s1 = root.gameplayScore(1);
        let s2 = root.gameplayScore(2);

        if ((num >= 1510 && num <= 1599) || (num >= 1610 && num <= 1699)) {
            return root.gameplayJudgeValueForId(num);
        }
        if (num >= 271 && num <= 289) {
            return 0;
        }

        switch (num) {
        case 20:
            return root.lr2CurrentFps;
        case 10:
            return root.lr2HiSpeedP1;
        case 11:
            return root.lr2HiSpeedP2;
        case 310:
            return root.hiSpeedInteger(1);
        case 311:
            return root.hiSpeedAfterDot(1);
        case 312:
            return root.durationNumber(1, false);
        case 313:
            return root.durationNumber(1, true);
        case 314:
            return root.liftNumber(1);
        case 315:
            return root.hiddenNumber(1);
        case 1312:
        case 1313:
        case 1314:
        case 1315:
        case 1316:
        case 1317:
        case 1318:
        case 1319:
        case 1320:
        case 1321:
        case 1322:
        case 1323:
        case 1324:
        case 1325:
        case 1326:
        case 1327:
            return root.bpmDurationNumber(num, chartData);
        case 12: {
            let vars = root.mainGeneralVars();
            return vars ? Math.round(vars.offset || 0) : 0;
        }
        case 13:
            return root.lr2TargetPercent;
        case 14:
            return root.laneCoverNumber(1);
        case 15:
            return root.laneCoverNumber(2);
        case 100:
            return root.gameplayDisplayedScorePrint(1);
        case 101:
            return root.gameplayExScore(s1);
        case 102:
            return root.gameplayRateInteger(s1, true);
        case 103:
            return root.gameplayRateDecimal(s1, true);
        case 104:
            return root.gameplayCombo(1, false);
        case 105:
            return root.gameplayCombo(1, true);
        case 106:
            return root.gameplayTotalNotes(s1);
        case 107:
            return Math.floor(root.gameplayGaugeValue(s1) / 2) * 2;
        case 407:
            return root.gaugeAfterDot(root.gameplayGaugeValue(s1));
        case 108:
            return root.gameplayExScore(s1) - root.gameplayExScore(s2);
        case 109:
            return root.gameplayRankDelta(s1);
        case 110:
            return root.gameplayJudgementCount(s1, 5);
        case 111:
            return root.gameplayJudgementCount(s1, 4);
        case 112:
            return root.gameplayJudgementCount(s1, 3);
        case 113:
            return root.gameplayJudgementCount(s1, 2);
        case 114:
            return root.gameplayPoorCount(s1);
        case 115:
            return root.gameplayRateInteger(s1, false);
        case 116:
            return root.gameplayRateDecimal(s1, false);
        case 410:
        case 411:
        case 412:
        case 413:
        case 414:
        case 415:
        case 416:
        case 417:
        case 418:
        case 419:
            return root.judgeTimingNumberFromCounts(num, root.gameplayJudgeTimingCounts1);
        case 420:
            return root.gameplayJudgementCount(s1, Judgement.EmptyPoor);
        case 421:
        case 422:
        case 423:
        case 424:
            return root.judgeTimingNumberFromCounts(num, root.gameplayJudgeTimingCounts1);
        case 425:
            return root.gameplayJudgementCount(s1, Judgement.Bad)
                + root.gameplayJudgementCount(s1, Judgement.Poor)
                + root.gameplayJudgementCount(s1, Judgement.EmptyPoor);
        case 426:
            return root.gameplayPoorCount(s1);
        case 427:
            return root.gameplayJudgementCount(s1, Judgement.Bad) + root.gameplayPoorCount(s1);
        case 525:
            return root.gameplayLastJudgeTiming1;
        case 526:
            return root.gameplayLastJudgeTiming2;
        case 527:
            return root.gameplayLastJudgeTiming1;
        case 500:
        case 501:
        case 502:
        case 503:
        case 504:
        case 505:
        case 506:
        case 507:
        case 508:
        case 509:
        case 510:
        case 511:
        case 512:
        case 513:
        case 514:
        case 515:
        case 516:
        case 517:
        case 518:
        case 519:
        case 520:
        case 521:
        case 522:
            return root.gameplayJudgeValueForId(num);
        case 120:
            return root.gameplayDisplayedScorePrint(2);
        case 121:
            return root.gameplayExScore(s2);
        case 122:
            return root.gameplayRateInteger(s2, true);
        case 123:
            return root.gameplayRateDecimal(s2, true);
        case 124:
            return root.gameplayCombo(2, false);
        case 125:
            return root.gameplayCombo(2, true);
        case 126:
            return root.gameplayTotalNotes(s2);
        case 127:
            return Math.floor(root.gameplayGaugeValue(s2) / 2) * 2;
        case 128:
            return root.gameplayExScore(s2) - root.gameplayExScore(s1);
        case 129:
            return root.gameplayRankDelta(s2);
        case 130:
            return root.gameplayJudgementCount(s2, 5);
        case 131:
            return root.gameplayJudgementCount(s2, 4);
        case 132:
            return root.gameplayJudgementCount(s2, 3);
        case 133:
            return root.gameplayJudgementCount(s2, 2);
        case 134:
            return root.gameplayPoorCount(s2);
        case 135:
            return root.gameplayRateInteger(s2, false);
        case 136:
            return root.gameplayRateDecimal(s2, false);
        case 150:
            return root.gameplayHighScorePoints();
        case 151:
            return root.gameplayTargetScorePoints();
        case 152:
            return root.gameplayExScore(s1) - root.gameplayHighScorePoints();
        case 153:
            return root.gameplayExScore(s1) - root.gameplayTargetScorePoints();
        case 154:
            return root.gameplayRankDelta(s1);
        case 155:
            return root.gameplayScoreRateInteger(root.gameplayHighScorePoints(), s1);
        case 156:
            return root.gameplayScoreRateDecimal(root.gameplayHighScorePoints(), s1);
        case 157:
            return root.gameplayScoreRateInteger(root.gameplayTargetScorePoints(), s1);
        case 158:
            return root.gameplayScoreRateDecimal(root.gameplayTargetScorePoints(), s1);
        case 160:
            return p1 && (p1.bpm || 0) > 0 ? Math.round(p1.bpm) : 1;
        case 161:
            return Math.floor(root.gameplayTimeSeconds(1, false) / 60);
        case 162:
            return root.gameplayTimeSeconds(1, false) % 60;
        case 163:
            return Math.floor(root.gameplayTimeSeconds(1, true) / 60);
        case 164:
            return root.gameplayTimeSeconds(1, true) % 60;
        case 1163: {
            let seconds = root.chartLengthSeconds(chartData);
            return seconds >= 0 ? Math.floor(seconds / 60) % 60 : -1;
        }
        case 1164: {
            let seconds = root.chartLengthSeconds(chartData);
            return seconds >= 0 ? seconds % 60 : -1;
        }
        case 165:
            return root.chart && root.chart.status !== undefined ? 100 : 0;
        case 350:
            return chartData ? (chartData.normalNoteCount || 0) : -1;
        case 351:
            return chartData ? (chartData.lnCount || 0) : -1;
        case 352:
            return chartData ? (chartData.scratchCount || 0) : -1;
        case 353:
            return chartData ? (chartData.bssCount || 0) : -1;
        case 354:
            return chartData ? (chartData.mineCount || 0) : -1;
        case 360:
            return root.chartDensityNumber(chartData, "peakDensity", false);
        case 361:
            return root.chartDensityNumber(chartData, "peakDensity", true);
        case 362:
            return root.chartDensityNumber(chartData, "endDensity", false);
        case 363:
            return root.chartDensityNumber(chartData, "endDensity", true);
        case 364:
            return root.chartDensityNumber(chartData, "avgDensity", false);
        case 365:
            return root.chartDensityNumber(chartData, "avgDensity", true);
        case 368:
            return chartData ? Math.floor(chartData.total || 0) : -1;
        case 42:
        case 96:
            return chartData ? (chartData.playLevel || 0) : 0;
        case 90:
        case 290:
            return chartData && (chartData.maxBpm || chartData.mainBpm)
                ? Math.round(chartData.maxBpm || chartData.mainBpm)
                : -1;
        case 91:
        case 291:
            return chartData && (chartData.minBpm || chartData.mainBpm)
                ? Math.round(chartData.minBpm || chartData.mainBpm)
                : -1;
        case 92:
            return chartData && chartData.mainBpm ? Math.round(chartData.mainBpm) : -1;
        default:
            return 0;
        }
    }

    function resultCompareResult() {
        return root.resultData(2) || root.resultOldBestResult(1);
    }

    function resolveResultSideNumber(num, result) {
        switch (num) {
        case 0:
            return root.resultScorePrint(result);
        case 1:
            return root.resultExScore(result);
        case 2:
            return root.resultRateInteger(result);
        case 3:
            return root.resultRateDecimal(result);
        case 5:
            return result ? (result.maxCombo || 0) : 0;
        case 6:
            return root.resultTotalNotes(result);
        case 7:
            return result === root.resultData(2)
                ? Math.floor(root.resultGaugeValue(2))
                : Math.floor(root.resultGaugeValue(1));
        case 10:
            return root.resultJudgementCount(result, Judgement.Perfect);
        case 11:
            return root.resultJudgementCount(result, Judgement.Great);
        case 12:
            return root.resultJudgementCount(result, Judgement.Good);
        case 13:
            return root.resultJudgementCount(result, Judgement.Bad);
        case 14:
            return root.resultPoorCount(result);
        default:
            return 0;
        }
    }

    function resolveResultTargetSideNumber(num, side) {
        let targetScore = root.resultTargetSavedScore(side);
        let targetResult = targetScore && targetScore.result ? targetScore.result : null;
        if (targetResult) {
            return root.resolveResultSideNumber(num, targetResult);
        }

        let points = root.resultTargetPoints(side);
        let maxPoints = root.resultTargetMaxPoints(side);
        let totalNotes = Math.floor(maxPoints / 2);
        switch (num) {
        case 0:
            return Math.floor(points * 100000 / maxPoints);
        case 1:
            return points;
        case 2:
            return Math.floor(points * 100 / maxPoints);
        case 3:
            return Math.floor(points * 10000 / maxPoints) % 100;
        case 6:
            return totalNotes;
        default:
            return 0;
        }
    }

    function resolveResultNumber(num) {
        root.resultOldScoresRevision;
        let current = root.resultData(1);
        let old = root.resultOldBestResult(1);
        let chartData = root.resultChartData();

        if (num >= 271 && num <= 289) {
            return 0;
        }

        if (num >= 100 && num <= 116) {
            return root.resolveResultSideNumber(num - 100, current);
        }
        if (num >= 120 && num <= 136) {
            return root.resolveResultTargetSideNumber(num - 120, 1);
        }

        switch (num) {
        case 20:
            return root.lr2CurrentFps;
        case 310:
            return root.hiSpeedInteger(1);
        case 311:
            return root.hiSpeedAfterDot(1);
        case 312:
            return root.durationNumber(1, false);
        case 313:
            return root.durationNumber(1, true);
        case 314:
            return root.liftNumber(1);
        case 315:
            return root.hiddenNumber(1);
        case 1312:
        case 1313:
        case 1314:
        case 1315:
        case 1316:
        case 1317:
        case 1318:
        case 1319:
        case 1320:
        case 1321:
        case 1322:
        case 1323:
        case 1324:
        case 1325:
        case 1326:
        case 1327:
            return root.bpmDurationNumber(num, chartData);
        case 42:
        case 96: {
            return chartData ? (chartData.playLevel || 0) : 0;
        }
        case 90:
        case 290: {
            return chartData && (chartData.maxBpm || chartData.mainBpm)
                ? Math.round(chartData.maxBpm || chartData.mainBpm)
                : -1;
        }
        case 91:
        case 291: {
            return chartData && (chartData.minBpm || chartData.mainBpm)
                ? Math.round(chartData.minBpm || chartData.mainBpm)
                : -1;
        }
        case 92:
            return chartData && chartData.mainBpm ? Math.round(chartData.mainBpm) : -1;
        case 350:
            return current ? (current.normalNoteCount || 0) : (chartData ? (chartData.normalNoteCount || 0) : -1);
        case 351:
            return current ? (current.lnCount || 0) : (chartData ? (chartData.lnCount || 0) : -1);
        case 352:
            return current ? (current.scratchCount || 0) : (chartData ? (chartData.scratchCount || 0) : -1);
        case 353:
            return current ? (current.bssCount || 0) : (chartData ? (chartData.bssCount || 0) : -1);
        case 354:
            return current ? (current.mineCount || 0) : (chartData ? (chartData.mineCount || 0) : -1);
        case 360:
            return root.chartDensityNumber(chartData, "peakDensity", false);
        case 361:
            return root.chartDensityNumber(chartData, "peakDensity", true);
        case 362:
            return root.chartDensityNumber(chartData, "endDensity", false);
        case 363:
            return root.chartDensityNumber(chartData, "endDensity", true);
        case 364:
            return root.chartDensityNumber(chartData, "avgDensity", false);
        case 365:
            return root.chartDensityNumber(chartData, "avgDensity", true);
        case 368:
            return chartData ? Math.floor(chartData.total || 0) : -1;
        case 370:
            return root.clearTypeValue(current ? current.clearType : "NOPLAY");
        case 371:
            return root.clearTypeValue(old ? old.clearType : "NOPLAY");
        case 372: {
            let stats = root.resultTimingStats(1);
            return Math.floor(stats.averageDuration || 0);
        }
        case 373: {
            let stats = root.resultTimingStats(1);
            return Math.floor((stats.averageDuration || 0) * 100) % 100;
        }
        case 374: {
            let stats = root.resultTimingStats(1);
            return Math.floor(stats.average || 0);
        }
        case 375: {
            let stats = root.resultTimingStats(1);
            return root.signedAfterDot(stats.average || 0);
        }
        case 376: {
            let stats = root.resultTimingStats(1);
            return Math.floor(stats.stddev || 0);
        }
        case 377: {
            let stats = root.resultTimingStats(1);
            return Math.floor((stats.stddev || 0) * 100) % 100;
        }
        case 407:
            return root.gaugeAfterDot(root.resultGaugeValue(1));
        case 410:
        case 411:
        case 412:
        case 413:
        case 414:
        case 415:
        case 416:
        case 417:
        case 418:
        case 419:
            return root.judgeTimingNumberFromCounts(num, root.resultJudgeTimingCounts(1));
        case 420:
            return root.resultJudgementCount(current, Judgement.EmptyPoor);
        case 421:
        case 422:
        case 423:
        case 424:
            return root.judgeTimingNumberFromCounts(num, root.resultJudgeTimingCounts(1));
        case 425:
            return root.resultJudgementCount(current, Judgement.Bad)
                + root.resultJudgementCount(current, Judgement.Poor)
                + root.resultJudgementCount(current, Judgement.EmptyPoor);
        case 426:
            return root.resultPoorCount(current);
        case 427:
            return root.resultJudgementCount(current, Judgement.Bad) + root.resultPoorCount(current);
        case 1163: {
            let seconds = root.chartLengthSeconds(chartData || current);
            return seconds >= 0 ? Math.floor(seconds / 60) % 60 : -1;
        }
        case 1164: {
            let seconds = root.chartLengthSeconds(chartData || current);
            return seconds >= 0 ? seconds % 60 : -1;
        }
        case 150:
            return root.resultHighScorePoints(1);
        case 151:
            return root.resultTargetPoints(1);
        case 152:
            return root.resultExScore(current) - root.resultHighScorePoints(1);
        case 153:
            return root.resultExScore(current) - root.resultTargetPoints(1);
        case 154:
            return root.resultRankDelta(current);
        case 155:
            return root.resultScoreRateInteger(root.resultHighScorePoints(1), current);
        case 156:
            return root.resultScoreRateDecimal(root.resultHighScorePoints(1), current);
        case 157:
            return root.resultScoreRateInteger(root.resultTargetPoints(1), current);
        case 158:
            return root.resultScoreRateDecimal(root.resultTargetPoints(1), current);
        case 170:
            return root.resultExScore(old);
        case 171:
            return root.resultExScore(current);
        case 172:
            return root.resultExScore(current) - root.resultExScore(old);
        case 173:
            return old ? (old.maxCombo || 0) : 0;
        case 174:
            return current ? (current.maxCombo || 0) : 0;
        case 175:
            return (current ? (current.maxCombo || 0) : 0) - (old ? (old.maxCombo || 0) : 0);
        case 176:
            return root.resultBadPoor(old);
        case 177:
            return root.resultBadPoor(current);
        case 178:
            return root.resultBadPoor(current) - root.resultBadPoor(old);
        case 179:
            return root.lr2RankingPlayerRank();
        case 180:
            return root.lr2RankingPlayerCount();
        case 181:
            return root.lr2RankingClearPercentValue(false, "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX");
        case 182:
            return 0;
        case 183:
            return root.resultRateInteger(old);
        case 184:
            return root.resultRateDecimal(old);
        default:
            if (num === 200) {
                return root.lr2RankingPlayerCount();
            }
            if (num === 201) {
                return root.lr2RankingTotalPlayCount(root.lr2RankingEntries());
            }
            if (num >= 202 && num <= 242) {
                switch (num) {
                case 202:
                    return root.lr2RankingClearCount("NOPLAY");
                case 203:
                    return root.lr2RankingClearPercentValue(false, "NOPLAY");
                case 204:
                    return root.lr2RankingClearCount("AEASY");
                case 205:
                    return root.lr2RankingClearPercentValue(false, "AEASY");
                case 206:
                    return root.lr2RankingClearCount("LIGHTASSIST");
                case 207:
                    return root.lr2RankingClearPercentValue(false, "LIGHTASSIST");
                case 208:
                    return root.lr2RankingClearCount("EXHARD");
                case 209:
                    return root.lr2RankingClearPercentValue(false, "EXHARD");
                case 210:
                    return root.lr2RankingClearCount("FAILED");
                case 211:
                    return root.lr2RankingClearPercentValue(false, "FAILED");
                case 212:
                    return root.lr2RankingClearCount("EASY");
                case 213:
                    return root.lr2RankingClearPercentValue(false, "EASY");
                case 214:
                    return root.lr2RankingClearCount("NORMAL");
                case 215:
                    return root.lr2RankingClearPercentValue(false, "NORMAL");
                case 216:
                    return root.lr2RankingClearCount("HARD");
                case 217:
                    return root.lr2RankingClearPercentValue(false, "HARD");
                case 218:
                    return root.lr2RankingClearCount("FC");
                case 219:
                    return root.lr2RankingClearPercentValue(false, "FC");
                case 222:
                    return root.lr2RankingClearCount("PERFECT");
                case 223:
                    return root.lr2RankingClearPercentValue(false, "PERFECT");
                case 224:
                    return root.lr2RankingClearCount("MAX");
                case 225:
                    return root.lr2RankingClearPercentValue(false, "MAX");
                case 226:
                    return root.lr2RankingClearCount("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX");
                case 227:
                    return root.lr2RankingClearPercentValue(false, "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX");
                case 228:
                    return root.lr2RankingClearCount("FC", "PERFECT", "MAX");
                case 229:
                    return root.lr2RankingClearPercentValue(false, "FC", "PERFECT", "MAX");
                case 230:
                    return root.lr2RankingClearPercentValue(true, "NOPLAY");
                case 231:
                    return root.lr2RankingClearPercentValue(true, "AEASY");
                case 232:
                    return root.lr2RankingClearPercentValue(true, "LIGHTASSIST");
                case 233:
                    return root.lr2RankingClearPercentValue(true, "EXHARD");
                case 234:
                    return root.lr2RankingClearPercentValue(true, "FAILED");
                case 235:
                    return root.lr2RankingClearPercentValue(true, "EASY");
                case 236:
                    return root.lr2RankingClearPercentValue(true, "NORMAL");
                case 237:
                    return root.lr2RankingClearPercentValue(true, "HARD");
                case 238:
                    return root.lr2RankingClearPercentValue(true, "FC");
                case 239:
                    return root.lr2RankingClearPercentValue(true, "PERFECT");
                case 240:
                    return root.lr2RankingClearPercentValue(true, "MAX");
                case 241:
                    return root.lr2RankingClearPercentValue(true, "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX");
                case 242:
                    return root.lr2RankingClearPercentValue(true, "FC", "PERFECT", "MAX");
                default:
                    return 0;
                }
            }
            if (num >= 380 && num <= 389) {
                return root.lr2RankingEntryExScore(num - 380);
            }
            if (num >= 390 && num <= 399) {
                return root.lr2RankingEntryClearValue(num - 390);
            }
            return 0;
        }
    }

    function resolveNumber(num) {
        if (root.isResultScreen()) {
            return root.resolveResultNumber(num);
        }
        if (root.isGameplayScreen()) {
            return root.resolveGameplayNumber(num);
        }
        if (root.effectiveScreenKey === "select" || root.effectiveScreenKey === "decide") {
            switch (num) {
            case 10:
                return root.lr2HiSpeedP1;
            case 11:
                return root.lr2HiSpeedP2;
            case 12: {
                let vars = root.mainGeneralVars();
                return vars ? Math.round(vars.offset || 0) : 0;
            }
            case 13:
                return root.lr2TargetPercent;
            case 14:
                return root.laneCoverNumber(1);
            case 15:
                return root.laneCoverNumber(2);
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28:
            case 29:
                return root.dateTimeNumber(num);
            case 310:
                return root.hiSpeedInteger(1);
            case 311:
                return root.hiSpeedAfterDot(1);
            case 312:
                return root.durationNumber(1, false);
            case 313:
                return root.durationNumber(1, true);
            case 314:
                return root.liftNumber(1);
            case 315:
                return root.hiddenNumber(1);
            case 1312:
            case 1313:
            case 1314:
            case 1315:
            case 1316:
            case 1317:
            case 1318:
            case 1319:
            case 1320:
            case 1321:
            case 1322:
            case 1323:
            case 1324:
            case 1325:
            case 1326:
            case 1327:
                return root.bpmDurationNumber(num, selectContext.selectedChartData());
            }
            if ((num >= 50 && num <= 66) || num === 8) {
                return root.lr2SliderNumber(num);
            }
            if (num >= 250 && num <= 259) {
                let stage = root.courseStage(num - 250);
                return selectContext.entryPlayLevel(stage);
            }
            if (num === 220) {
                return -1;
            }
            if (num >= 380 && num <= 389) {
                return root.lr2RankingEntryExScore(num - 380);
            }
            if (num >= 390 && num <= 399) {
                return root.lr2RankingEntryClearValue(num - 390);
            }
            if ((num >= 271 && num <= 289)
                    || num === 292 || num === 293) {
                return 0;
            }
            let selectValue = selectContext.numberValue(num);
            if (root.effectiveScreenKey === "select" || selectValue !== 0) {
                return selectValue;
            }
            let chartData = root.chart && root.chart.chartData ? root.chart.chartData : null;
            switch (num) {
            case 42:
            case 96:
                return chartData ? (chartData.playLevel || 0) : 0;
            case 90:
            case 290:
                return chartData && (chartData.maxBpm || chartData.mainBpm)
                    ? Math.round(chartData.maxBpm || chartData.mainBpm)
                    : -1;
            case 91:
            case 291:
                return chartData && (chartData.minBpm || chartData.mainBpm)
                    ? Math.round(chartData.minBpm || chartData.mainBpm)
                    : -1;
            default:
                return 0;
            }
        }
        return 0;
    }

    function numberValue(src) {
        if (src && src.nowCombo) {
            return (src.side || (src.timer === 47 ? 2 : 1)) === 2
                ? root.gameplayJudgeCombo2
                : root.gameplayJudgeCombo1;
        }
        return root.resolveNumber(src ? src.num : 0);
    }

    function imageSetValue(imageSetRef, sourceCount) {
        let id = Math.floor(imageSetRef || 0);
        switch (id) {
        case 12:
            return root.effectiveScreenKey === "select" && sourceCount >= 5
                ? selectContext.sortFrameForSourceCount(sourceCount)
                : root.resolveNumber(id);
        case 40:
            return root.lr2GaugeIndexP1;
        case 41:
            return root.lr2GaugeIndexP2;
        case 42:
            return root.lr2RandomIndexP1;
        case 43:
            return root.lr2RandomIndexP2;
        case 54:
            return root.lr2DpOptionIndex;
        case 55:
            return root.lr2HiSpeedFixIndex;
        case 72:
            return sourceCount >= 3 ? root.lr2BeatorajaBgaIndex : root.lr2BgaIndex;
        case 77:
            return root.lr2BeatorajaTargetIndex;
        case 78:
            return root.lr2GaugeAutoShiftIndex;
        case 301:
        case 302:
        case 303:
        case 304:
        case 305:
        case 306:
        case 307:
        case 308:
            return 0;
        default:
            return root.resolveNumber(id);
        }
    }

    function imageSetSourceFor(src) {
        if (!src || !src.imageSet || !src.imageSetSources || src.imageSetSources.length <= 0) {
            return src;
        }
        let value = Math.floor(root.imageSetValue(src.imageSetRef || 0, src.imageSetSources.length));
        if (!isFinite(value) || value < 0 || value >= src.imageSetSources.length) {
            value = 0;
        }
        let selected = src.imageSetSources[value] || src.imageSetSources[0];
        if (!selected) {
            return src;
        }
        let source = selected.source !== undefined ? selected.source : src.source;
        let specialType = selected.specialType !== undefined ? selected.specialType : src.specialType;
        if (!source && !specialType) {
            source = src.source || "";
            specialType = src.specialType || 0;
        }
        return {
            gr: selected.gr !== undefined ? selected.gr : (src.gr || 0),
            x: selected.x !== undefined ? selected.x : (src.x || 0),
            y: selected.y !== undefined ? selected.y : (src.y || 0),
            w: selected.w !== undefined ? selected.w : (src.w || 0),
            h: selected.h !== undefined ? selected.h : (src.h || 0),
            div_x: Math.max(1, selected.div_x || 1),
            div_y: Math.max(1, selected.div_y || 1),
            cycle: src.cycle || 0,
            timer: src.timer || 0,
            zeropadding: src.zeropadding !== undefined ? src.zeropadding : -1,
            op1: src.op1 || 0,
            op2: src.op2 || 0,
            op3: src.op3 || 0,
            op4: src.op4 || 0,
            resultChartType: selected.resultChartType || 0,
            resultChartIndex: selected.resultChartIndex || 0,
            button: false,
            onMouse: false,
            mouseCursor: false,
            slider: false,
            specialType: specialType || 0,
            side: selected.side || 0,
            source: source || ""
        };
    }

    function numberForceHidden(src) {
        if (!src || !src.nowCombo || !root.isGameplayScreen()) {
            return false;
        }
        let side = src.side || (src.timer === 47 ? 2 : 1);
        let judgement = side === 2 ? root.gameplayLastJudgement2 : root.gameplayLastJudgement1;
        let combo = side === 2 ? root.gameplayJudgeCombo2 : root.gameplayJudgeCombo1;
        return combo <= 0
            || (src.judgementIndex >= 0 && judgement !== src.judgementIndex);
    }

    function numberAnimationRevision(src) {
        if (!src || !src.nowCombo || !root.isGameplayScreen()) {
            return 0;
        }
        return (src.side || (src.timer === 47 ? 2 : 1)) === 2
            ? root.gameplayJudgeRevision2
            : root.gameplayJudgeRevision1;
    }

    function resolveBarGraph(type) {
        if (root.effectiveScreenKey === "select") {
            return selectContext.barGraphValue(type);
        }
        if (root.isGameplayScreen()) {
            return playContext.barGraphValue(type);
        }
        if (root.isResultScreen()) {
            return root.resultBarGraphValue(type);
        }
        return 0;
    }

    function normalizedBarValue(value, maximum) {
        return maximum > 0 ? Math.max(0, Math.min(1, value / maximum)) : 0;
    }

    function resultBarGraphValue(type) {
        let current = root.resultData(1);
        let old = root.resultOldBestResult(1);
        let maxPoints = current ? Math.max(0, current.maxPoints || 0) : 0;
        let totalNotes = root.resultTotalNotes(current);
        switch (type) {
        case 101:
        case 102:
            return 1;
        case 110:
        case 111:
            return root.normalizedBarValue(root.resultExScore(current), maxPoints);
        case 112:
        case 113:
            return root.normalizedBarValue(root.resultExScore(old),
                                           maxPoints || (old ? (old.maxPoints || 0) : 0));
        case 114:
        case 115:
            return root.normalizedBarValue(root.resultTargetPoints(1), maxPoints);
        case 140:
            return root.normalizedBarValue(root.resultJudgementCount(current, Judgement.Perfect), totalNotes);
        case 141:
            return root.normalizedBarValue(root.resultJudgementCount(current, Judgement.Great), totalNotes);
        case 142:
            return root.normalizedBarValue(root.resultJudgementCount(current, Judgement.Good), totalNotes);
        case 143:
            return root.normalizedBarValue(root.resultJudgementCount(current, Judgement.Bad), totalNotes);
        case 144:
            return root.normalizedBarValue(root.resultPoorCount(current), totalNotes);
        case 145:
            return root.normalizedBarValue(current ? (current.maxCombo || 0) : 0, totalNotes);
        case 146:
            return root.normalizedBarValue(root.resultScorePrint(current),
                                           (current && (current.keymode === 7 || current.keymode === 14)) ? 20000 : 10000);
        case 147:
            return root.normalizedBarValue(root.resultExScore(current), maxPoints);
        default:
            return 0;
        }
    }

}
