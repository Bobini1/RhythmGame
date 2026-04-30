pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host
    required property var selectContext
    required property var skinModel

    function addOption(options, option) {
        host.addOption(options, option);
    }

    function finalizeOptionList(options) {
        return host.finalizeOptionList(options);
    }

    function appendCommonRuntimeOptions(options) {
        let vars = host.mainGeneralVars();
        root.addOption(options, vars && vars.bgaSize === 1 ? 31 : 30);
        if (host.isGameplayScreen()) {
            root.addOption(options, host.gameplayAutoplayActive() ? 33 : 32);
        } else {
            root.addOption(options, 32); // autoplay off unless a launch button explicitly requests it.
        }
        if (host.effectiveScreenKey === "decide") {
            root.addOption(options, 33); // LR2 decide reports both autoplay states true.
        }
        let ghostPosition = vars ? Math.max(0, Math.min(3, vars.ghostPosition || 0)) : 0;
        root.addOption(options, 34 + ghostPosition);
        root.addOption(options, vars && vars.scoreGraphEnabled === false ? 38 : 39);
        root.addOption(options, vars && vars.bgaOn === false ? 40 : 41);
        root.addOption(options, host.gaugeColorOption(1));
        root.addOption(options, host.gaugeColorOption(2));
        host.addGaugeExOption(options, 1);
        host.addGaugeExOption(options, 2);
        root.addOption(options, host.gameplayGaugeTrophyOption(1));
        root.addOption(options, host.gameplayGaugeTrophyOption(2));
        root.addOption(options, 46); // difficulty filter enabled.
        root.addOption(options, host.isLoggedIn() ? 51 : 50);
        root.addOption(options, 52); // extra mode off.
        root.addOption(options, 54); // 1P autoscratch/assist off.
        root.addOption(options, 56); // 2P autoscratch/assist off.
        root.addOption(options, 61);
        root.addOption(options, host.clearStatusOption());
        if (host.isGameplayScreen()) {
            // LR2 play skins use 80 while the chart is still loading, then
            // switch to 81 when timer 40 (READY / load complete) fires.
            root.addOption(options, host.gameplayReadySkinTime >= 0 ? 81 : 80);
        } else {
            root.addOption(options, 80); // select/decide are already loaded when the skin is shown.
        }
        root.addOption(options, host.gameplayReplayActive() ? 84 : 82);
        root.addOption(options, 572); // course editor/making mode is not supported here.
        root.addOption(options, 622); // ghost battle is not supported from select.
        root.addOption(options, 624); // rival compare is not supported from select.
    }

    function appendPanelOptions(options) {
        root.addOption(options, host.selectPanel > 0 ? 20 + host.selectPanel : 20);
    }

    function appendChartOptions(options, chartData) {
        let allowStageFileOption = host.effectiveScreenKey !== "decide";
        if (!chartData) {
            if (allowStageFileOption) {
                root.addOption(options, 190);
            }
            root.addOption(options, 192);
            root.addOption(options, 194);
            root.addOption(options, 170);
            root.addOption(options, 172);
            root.addOption(options, 174);
            root.addOption(options, 176);
            root.addOption(options, 178);
            root.appendReplayOptions(options, null);
            root.addOption(options, 150);
            return;
        }
        if (allowStageFileOption) {
            root.addOption(options, chartData.stageFile ? 191 : 190);
        }
        root.addOption(options, chartData.banner ? 193 : 192);
        root.addOption(options, chartData.backBmp ? 195 : 194);
        root.addOption(options, selectContext.hasBga(chartData) ? 171 : 170);
        root.addOption(options, selectContext.hasLongNote(chartData) ? 173 : 172);
        root.addOption(options, selectContext.hasAttachedText(chartData) ? 175 : 174);
        root.addOption(options, (chartData.maxBpm || 0) !== (chartData.minBpm || 0) ? 177 : 176);
        if (host.skinUsesOption(1177) && host.chartHasBpmStop(chartData)) {
            root.addOption(options, 1177);
        }
        root.addOption(options, chartData.isRandom ? 179 : 178);
        root.addOption(options, selectContext.judgeOption(chartData));
        root.addOption(options, selectContext.highLevelOption(chartData));
        root.appendReplayOptions(options, chartData);

        let difficulty = selectContext.entryDifficulty(chartData);
        root.addOption(options, difficulty >= 1 && difficulty <= 5 ? 150 + difficulty : 150);

        let keymode = chartData.keymode || 0;
        root.appendKeymodeOption(options, keymode, 160);
        root.appendKeymodeOption(options, root.keymodeAfterOptions(keymode), 165);
    }

    function replayOptionForSlot(slot, available) {
        if (slot === 0) {
            return available ? 197 : 196;
        }
        if (slot === 1) {
            return available ? 1197 : 1196;
        }
        if (slot === 2) {
            return available ? 1200 : 1199;
        }
        return available ? 1203 : 1202;
    }

    function selectedReplayOptionForSlot(slot) {
        return 1205 + Math.max(0, Math.min(3, slot));
    }

    function replaySlotAvailable(chartData, slot) {
        if (!chartData || host.effectiveScreenKey !== "select") {
            return !!chartData && slot === 0 && selectContext.hasReplay(chartData);
        }
        return !!selectContext.replayScoreForType(chartData, slot);
    }

    function appendReplayOptions(options, chartData) {
        if (!host.selectUsesReplayOptions()) {
            return;
        }
        let selectedReplayAvailable = false;
        for (let slot = 0; slot < 4; ++slot) {
            let available = root.replaySlotAvailable(chartData, slot);
            root.addOption(options, root.replayOptionForSlot(slot, available));
            if (slot === host.lr2ReplayType && available) {
                selectedReplayAvailable = true;
            }
        }
        if (selectedReplayAvailable) {
            root.addOption(options, root.selectedReplayOptionForSlot(host.lr2ReplayType));
        }
    }

    function appendKeymodeOption(options, keymode, baseOption) {
        switch (keymode) {
        case 7:
            root.addOption(options, baseOption);
            break;
        case 5:
            root.addOption(options, baseOption + 1);
            break;
        case 14:
            root.addOption(options, baseOption + 2);
            break;
        case 10:
            root.addOption(options, baseOption + 3);
            break;
        case 24:
            root.addOption(options, 1160);
            break;
        case 48:
            root.addOption(options, 1161);
            break;
        case 9:
            root.addOption(options, 164);
            break;
        }
    }

    function keymodeAfterOptions(keymode) {
        if (!host.spToDpActive()) {
            return keymode;
        }
        if (keymode === 7) {
            return 14;
        }
        if (keymode === 5) {
            return 10;
        }
        return keymode;
    }

    function chartKeymodeForStatus(item, selectedChart) {
        if (selectedChart && selectedChart.keymode) {
            return selectedChart.keymode;
        }
        if ((selectContext.isChart(item) || selectContext.isEntry(item)) && item.keymode) {
            return item.keymode;
        }
        return 0;
    }

    function appendEntryStatusOptions(options, item, selectedChart) {
        if (!host.selectUsesEntryStatusOptions()) {
            return;
        }
        let folderLike = selectContext.isFolderLikeForLamp(item);
        if (!selectContext.isChart(item)
                && !selectContext.isEntry(item)
                && !selectContext.isRankingEntry(item)
                && !folderLike) {
            return;
        }
        if (!folderLike && root.chartKeymodeForStatus(item, selectedChart) <= 0) {
            return;
        }
        let clearOption = selectContext.beatorajaClearOption(item);
        let hasExactBeatorajaLamp = clearOption >= 1100 && host.skinUsesOption(clearOption);
        let lamp = selectContext.entryLamp(item);
        if (!hasExactBeatorajaLamp && lamp >= 0 && lamp <= 5) {
            root.addOption(options, 100 + lamp);
        }
        if (folderLike) {
            return;
        }
        root.addOption(options, clearOption);

        let rank = selectContext.entryRank(item);
        if (lamp > 0 && rank >= 1) {
            root.addOption(options, 208 - Math.min(rank, 8));
            root.addOption(options, 118 - Math.min(rank, 8));
        }
    }

    function appendCourseOptions(options, item) {
        if (!selectContext.isCourse(item) || !host.selectUsesCourseDetailOptions()) {
            return;
        }
        root.addOption(options, 290);
        root.addOption(options, 293); // rank certification / class
        if (!item.loadCharts) {
            return;
        }

        let stages = item.loadCharts();
        for (let i = 0; i < Math.min(10, stages.length); ++i) {
            root.addOption(options, 580 + i);
        }
        for (let stage = 0; stage < Math.min(5, stages.length); ++stage) {
            let difficulty = selectContext.entryDifficulty(stages[stage]);
            root.addOption(options, 700 + stage * 10 + Math.max(0, Math.min(5, difficulty)));
        }
    }

    function appendDifficultyBarOptions(options) {
        if (!host.selectUsesDifficultyBarOptions()) {
            return;
        }
        for (let diff = 1; diff <= 5; ++diff) {
            if (selectContext.hasDifficulty(diff)) {
                root.addOption(options, 504 + diff);
                root.addOption(options, selectContext.difficultyLevelBarOption(diff));
            } else {
                root.addOption(options, 499 + diff);
            }

            let diffCount = selectContext.difficultyCount(diff);
            if (diffCount === 1) {
                root.addOption(options, 509 + diff);
            } else if (diffCount > 1) {
                root.addOption(options, 514 + diff);
            }
            root.addOption(options, 510 + diff * 10 + selectContext.difficultyLamp(diff));
        }
    }

    function appendSelectItemTypeOptions(options, item) {
        if (selectContext.isChart(item) || selectContext.isEntry(item) || selectContext.isRankingEntry(item)) {
            root.addOption(options, 2);
            root.addOption(options, 5);
        } else if (selectContext.isCourse(item)) {
            root.addOption(options, 3);
            root.addOption(options, 5);
            root.addOption(options, 290);
        } else {
            root.addOption(options, 1);
        }
    }

    function appendSelectedChartModeOptions(options, chartData) {
        let keymode = chartData ? (chartData.keymode || 0) : 0;
        let doubleMode = keymode === 10 || keymode === 14
            || ((keymode === 5 || keymode === 7) && host.spToDpActive());
        let battleMode = host.battleModeActive();
        if (doubleMode) {
            root.addOption(options, 10);
        }
        if (battleMode) {
            root.addOption(options, 11);
        }
        if (doubleMode || battleMode) {
            root.addOption(options, 12);
        }
        if (battleMode) {
            root.addOption(options, 13);
        }
    }

    function appendGameplayLaneCoverOptions(options, side) {
        let vars = host.generalVarsForSide(side);
        if (vars && vars.laneCoverOn) {
            root.addOption(options, 271);
        }
        if (vars && vars.liftOn) {
            root.addOption(options, 272);
        }
        if (vars && vars.hiddenOn) {
            root.addOption(options, 273);
        }
    }

    function appendJudgementExistOptions(options, resultOrScore) {
        if (host.judgementCountForExist(resultOrScore, Judgement.Perfect) > 0) {
            root.addOption(options, 2241);
        }
        if (host.judgementCountForExist(resultOrScore, Judgement.Great) > 0) {
            root.addOption(options, 2242);
        }
        if (host.judgementCountForExist(resultOrScore, Judgement.Good) > 0) {
            root.addOption(options, 2243);
        }
        if (host.judgementCountForExist(resultOrScore, Judgement.Bad) > 0) {
            root.addOption(options, 2244);
        }
        if (host.judgementCountForExist(resultOrScore, Judgement.Poor) > 0) {
            root.addOption(options, 2245);
        }
        if (host.judgementCountForExist(resultOrScore, Judgement.EmptyPoor) > 0) {
            root.addOption(options, 2246);
        }
    }

    function appendGameplaySideOptions(options, side) {
        let score = host.gameplayScore(side);
        root.addOption(options, host.gameplayGaugeOption(side));
        root.addOption(options, host.gameplayLaneOption(score));
        root.addOption(options, host.gameplayLaneCoverOption(side));
        root.appendGameplayLaneCoverOptions(options, side);

        let currentRank = host.gameplayRankOption(score, side === 2 ? 210 : 200, true);
        if (currentRank >= 0) {
            root.addOption(options, currentRank);
        }
        if (side === 1) {
            let totalRank = host.gameplayRankOption(score, 220, false);
            if (totalRank >= 0) {
                root.addOption(options, totalRank);
            }
            root.addOption(options, host.gameplayExactRankOption(score, 300));
        } else {
            root.addOption(options, host.gameplayExactRankOption(score, 310));
        }
        host.addGameplayGaugeRangeOption(options, score, side === 2 ? 250 : 230);
        if (side === 1 && host.gameplayGaugeQualified(score)) {
            root.addOption(options, 1240);
        }
        let judgementOption = host.gameplayJudgementOption(side, side === 2 ? 261 : 241);
        if (judgementOption >= 0) {
            root.addOption(options, judgementOption);
        }
        let timing = side === 2 ? host.gameplayLastJudgeTiming2 : host.gameplayLastJudgeTiming1;
        if (timing !== 0 && judgementOption >= 0 && judgementOption !== (side === 2 ? 261 : 241)) {
            root.addOption(options, timing > 0 ? (side === 2 ? 1262 : 1242) : (side === 2 ? 1263 : 1243));
        }
        root.addOption(options, host.gameplayPoorBgaOption(side, side === 2 ? 267 : 247));
    }

    function appendGameplayRuntimeOptions(options) {
        host.gameplayRevision;
        let chartData = host.gameplayChartData();
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendChartOptions(options, chartData);
        root.appendGameplaySideOptions(options, 1);
        if (host.gameplayLanePlayer(2)) {
            root.appendGameplaySideOptions(options, 2);
        }
        root.appendJudgementExistOptions(options, host.gameplayScore(1));
        if (host.gameplaySudChanging(1)) {
            root.addOption(options, 270);
        }
        if (host.gameplaySudChanging(2)) {
            root.addOption(options, 271);
        }

        root.addOption(options, 142); // autoscratch off.
        if (host.battleModeActive()) {
            root.addOption(options, 144);
        }
        if (host.spToDpActive()) {
            root.addOption(options, 145);
        }
        if (host.chart && host.chart.chartDatas && host.chart.currentChartIndex !== undefined) {
            let stage = Math.max(0, host.chart.currentChartIndex || 0);
            let count = host.chart.chartDatas.length || 0;
            root.addOption(options, 290);
            root.addOption(options, 293);
            if (count > 0 && stage === count - 1) {
                root.addOption(options, 289);
            } else if (count > 0) {
                root.addOption(options, 280 + Math.min(stage, 8));
            }
        }
    }

    function appendResultRuntimeOptions(options) {
        host.resultOldScoresRevision;
        let chartData = host.resultChartData();
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendChartOptions(options, chartData);
        root.addOption(options, host.resultClearOption());
        if (options.indexOf(351) === -1) {
            root.addOption(options, 350);
        }

        let current1 = host.resultData(1);
        let current2 = host.resultData(2);
        root.addOption(options, host.resultRankOptionForResult(current1, 300));
        if (current2) {
            root.addOption(options, host.resultRankOptionForResult(current2, 310));
        }
        root.addOption(options, host.resultRankOptionForResult(host.resultOldBestResult(1), 320));
        root.addOption(options, host.resultRankOptionForResult(host.resultUpdatedBestResult(1), 340));

        if (host.resultScoreImproved(1)) {
            root.addOption(options, 330);
            if (host.resultRawRank(current1) > host.resultRawRank(host.resultOldBestResult(1))) {
                root.addOption(options, 335);
            }
        } else {
            root.addOption(options, 1330);
            if (host.resultRawRank(current1) === host.resultRawRank(host.resultOldBestResult(1))) {
                root.addOption(options, 1335);
            }
        }
        root.addOption(options, host.resultComboImproved(1) ? 331 : 1331);
        root.addOption(options, host.resultBadPoorImproved(1) ? 332 : 1332);

        let targetDiff = host.resultExScore(current1) - host.resultTargetPoints(1);
        if (targetDiff > 0) {
            root.addOption(options, 336);
        } else if (targetDiff === 0) {
            root.addOption(options, 1336);
        }

        if (current1 && current2) {
            let diff = host.resultExScore(current1) - host.resultExScore(current2);
            root.addOption(options, diff > 0 ? 352 : (diff < 0 ? 353 : 354));
        }

        root.appendJudgementExistOptions(options, current1);
    }

    function appendCurrentSelectOptions(options, item, selectedChart) {
        root.appendSelectItemTypeOptions(options, item);
        root.appendSelectedChartModeOptions(options, selectedChart);
        root.appendEntryStatusOptions(options, item, selectedChart);
        root.appendCourseOptions(options, item);

        if (selectedChart) {
            root.appendDifficultyBarOptions(options);
        }

        root.appendChartOptions(options, selectedChart);
        if (host.selectUsesScoreOptionIds()) {
            for (let optionId of selectContext.scoreOptionIds(item)) {
                root.addOption(options, optionId);
            }
        }
    }

    function appendDecideOptions(options) {
        let chartData = host.chart && host.chart.chartData
            ? host.chart.chartData
            : selectContext.selectedChartData();
        root.appendSelectItemTypeOptions(options, chartData);
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendEntryStatusOptions(options, chartData, chartData);
        root.appendChartOptions(options, chartData);
        if (host.selectUsesScoreOptionIds()) {
            for (let optionId of selectContext.scoreOptionIds(chartData)) {
                root.addOption(options, optionId);
            }
        }
    }

    function runtimeOwnsOptionPair(option) {
        switch (option) {
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 1046:
        case 1047:
        case 50:
        case 51:
        case 80:
        case 81:
            return true;
        default:
            return false;
        }
    }

    function appendParserActiveOptions(result) {
        root.addOption(result, 0);
        let staticOptions = skinModel.effectiveActiveOptions && skinModel.effectiveActiveOptions.length
            ? skinModel.effectiveActiveOptions
            : host.parseActiveOptions;
        for (let option of staticOptions) {
            if (!root.runtimeOwnsOptionPair(option)) {
                root.addOption(result, option);
            }
        }
    }

    function appendStaticSelectOptions(result) {
        root.appendParserActiveOptions(result);

        root.addOption(result, 20);  // no side panel active
        root.addOption(result, 46);  // difficulty filter enabled
        root.addOption(result, 52);  // non-extra mode
        root.addOption(result, 572); // not course-select mode
        root.addOption(result, 622); // no ghost battle
        root.addOption(result, 624); // no rival score
    }

    function buildBarActiveOptions() {
        let result = root.finalizeOptionList([]);
        root.appendStaticSelectOptions(result);
        root.addOption(result, selectContext.rankingMode ? 621 : 620);
        return result;
    }

    function buildBaseActiveOptions(barOptions) {
        let result = host.effectiveScreenKey === "select"
            ? root.finalizeOptionList((barOptions || host.barActiveOptions).slice())
            : root.finalizeOptionList([]);
        if (host.effectiveScreenKey !== "select") {
            root.appendParserActiveOptions(result);
            return result;
        }

        root.addOption(result, host.lr2RankingStatusOption());
        let rankingCount = host.lr2RankingPlayerCount();
        if (rankingCount === 0) {
            root.addOption(result, 610);
        }
        for (let threshold = 1; threshold <= 6; ++threshold) {
            if (rankingCount > threshold - 1) {
                root.addOption(result, 610 + threshold);
            }
        }
        root.addOption(result, 622); // not ghost battle
        root.addOption(result, 624); // not rival compare
        root.appendPanelOptions(result);

        return result;
    }

    function buildRuntimeActiveOptions(baseOptions) {
        let result = root.finalizeOptionList(baseOptions.slice());
        if (host.effectiveScreenKey === "select") {
            root.appendCommonRuntimeOptions(result);
            root.appendCurrentSelectOptions(result, selectContext.current, selectContext.selectedChartData());
        } else if (host.effectiveScreenKey === "decide") {
            root.appendCommonRuntimeOptions(result);
            root.appendDecideOptions(result);
        } else if (host.isGameplayScreen()) {
            root.appendCommonRuntimeOptions(result);
            root.appendGameplayRuntimeOptions(result);
        } else if (host.isResultScreen()) {
            root.appendCommonRuntimeOptions(result);
            root.appendResultRuntimeOptions(result);
        } else {
            root.appendChartOptions(result, host.chart && host.chart.chartData ? host.chart.chartData : null);
        }

        return result;
    }
}
