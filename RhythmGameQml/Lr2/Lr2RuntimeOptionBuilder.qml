pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

QtObject {
    id: root

    required property var host
    required property var selectContext
    required property var skinModel
    required property var rankingState
    readonly property var runtimeUsedOptions: host.usedOptionFilterActive ? host.usedOptionLookup : null
    readonly property bool hostGameplayScreen: host.gameplayScreenActive
    readonly property bool hostResultScreen: host.resultScreenActive
    readonly property var hostMainGeneralVars: host.mainGeneralVarsRef

    function optionLookupFor(options: var) : var {
        if (!options) {
            return {};
        }
        if (options.__lookup) {
            return options.__lookup;
        }
        let lookup = {};
        for (let i = 0; i < options.length; ++i) {
            lookup[options[i]] = true;
        }
        options.__lookup = lookup;
        return lookup;
    }

    function appendUniqueRuntimeOption(options: var, option: var) : void {
        if (option === undefined || option === null) {
            return;
        }
        let lookup = root.optionLookupFor(options);
        if (lookup[option] === true && options.indexOf(option) !== -1) {
            return;
        }
        options.push(option);
        lookup[option] = true;
    }

    function addRuntimeOption(options: var, option: var) : void {
        if (option === undefined || option === null) {
            return;
        }
        let usedOptions = root.runtimeUsedOptions;
        if (usedOptions && usedOptions[Math.abs(option)] !== true) {
            return;
        }
        root.appendUniqueRuntimeOption(options, option);
    }

    function addSelectChartDetailRuntimeOption(options: var, option: var) : void {
        if (option === undefined || option === null || option <= 0) {
            return;
        }
        root.appendUniqueRuntimeOption(options, option);
    }

    function removeRuntimeOptionRange(options: var, first: var, last: var) : void {
        if (!options) {
            return;
        }

        let writeIndex = 0;
        let changed = false;
        let lookup = {};
        for (let i = 0; i < options.length; ++i) {
            let option = options[i];
            let optionNumber = Number(option);
            let absOption = Math.abs(optionNumber);
            if (isFinite(absOption) && absOption >= first && absOption <= last) {
                changed = true;
                continue;
            }

            options[writeIndex++] = option;
            lookup[option] = true;
        }

        if (!changed) {
            return;
        }

        options.length = writeIndex;
        options.__lookup = lookup;
    }

    function setRuntimeOptionRange(options: var, first: var, last: var, option: var) : void {
        root.removeRuntimeOptionRange(options, first, last);
        if (option !== undefined && option !== null && option > 0) {
            root.addRuntimeOption(options, option);
        }
    }

    function setSelectChartDetailRuntimeOptionRange(options: var, first: var, last: var, option: var) : void {
        root.removeRuntimeOptionRange(options, first, last);
        if (host.effectiveScreenKey === "select") {
            root.addSelectChartDetailRuntimeOption(options, option);
        } else if (option !== undefined && option !== null && option > 0) {
            root.addRuntimeOption(options, option);
        }
    }

    function addOption(options: var, option: var) : void {
        if (option === undefined || option === null) {
            return;
        }
        if (Lr2SkinOptionRules.isRuntimeOwnedOption(option)) {
            root.addRuntimeOption(options, option);
            return;
        }
        host.addOption(options, option);
    }

    function finalizeOptionList(options: var) : var {
        return host.finalizeOptionList(options);
    }

    function runtimeOptionUsed(lookup: var, option: var) : var {
        return !lookup || !!lookup[Math.abs(option)];
    }

    function runtimeOptionRangeUsed(lookup: var, first: var, last: var) : var {
        if (!lookup) {
            return true;
        }
        for (let option = first; option <= last; ++option) {
            if (lookup[option]) {
                return true;
            }
        }
        return false;
    }

    function runtimeAnyOptionUsed(lookup: var, options: var) : var {
        if (!lookup) {
            return true;
        }
        for (let option of options) {
            if (lookup[Math.abs(option)] === true) {
                return true;
            }
        }
        return false;
    }

    function selectReplayOptionsUsed() : var {
        return root.runtimeAnyOptionUsed(root.runtimeUsedOptions, [
            196, 197, 1196, 1197, 1199, 1200,
            1202, 1203, 1205, 1206, 1207, 1208
        ]);
    }

    function selectScoreOptionIdsUsed() : var {
        return root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 118, 130)
            || root.runtimeAnyOptionUsed(root.runtimeUsedOptions, [144, 145, 1128]);
    }

    function selectEntryStatusOptionsUsed() : var {
        return root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 100, 130)
            || root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 200, 207)
            || root.runtimeAnyOptionUsed(root.runtimeUsedOptions, [1100, 1101, 1102, 1103, 1104]);
    }

    function selectCourseDetailOptionsUsed() : var {
        return root.runtimeAnyOptionUsed(root.runtimeUsedOptions, [290, 293])
            || root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 580, 589)
            || root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 700, 755);
    }

    function selectDifficultyBarOptionsUsed() : var {
        return root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 70, 79)
            || root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 500, 570);
    }

    function selectDifficultyLampOptionsUsed() : var {
        return root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 520, 570);
    }

    function selectRankingStatusOptionsUsed() : var {
        return root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 600, 616);
    }

    function copyActiveOptions(options: var) : var {
        let result = [];
        let source = options || [];
        let lookup = {};
        for (let i = 0; i < source.length; ++i) {
            let option = source[i];
            if (option === undefined || option === null) {
                continue;
            }
            if (lookup[option] === true) {
                continue;
            }
            result.push(option);
            lookup[option] = true;
        }
        result.__lookup = lookup;
        return result;
    }

    function cloneOptionList(options: var) : var {
        let result = [];
        let source = options || [];
        let length = source.length || 0;
        result.length = 0;
        for (let i = 0; i < length; ++i) {
            let option = source[i];
            if (option !== undefined && option !== null) {
                result.push(option);
            }
        }
        return root.finalizeOptionList(result);
    }

    function appendCommonRuntimeOptions(options: var) : void {
        root.appendRequiredRuntimeOptions(options);
        root.addOption(options, 572); // course editor/making mode is not supported here.
        root.addOption(options, 622); // ghost battle is not supported from select.
        root.addOption(options, 624); // rival compare is not supported from select.
        if (root.hostGameplayScreen) {
            let gaugeTrophy1 = host.gameplayGaugeTrophyOption(1);
            let gaugeTrophy2 = host.gameplayGaugeTrophyOption(2);
            root.addOption(options, gaugeTrophy1);
            root.addOption(options, gaugeTrophy2);
        }
    }

    function appendRequiredRuntimeOptions(options: var) : void {
        let vars = root.hostMainGeneralVars;
        root.addRuntimeOption(options, vars && vars.bgaSize === 1 ? 31 : 30);
        if (root.hostGameplayScreen) {
            root.addRuntimeOption(options, host.gameplayAutoplayActive() ? 33 : 32);
        } else {
            root.addRuntimeOption(options, 32); // autoplay off unless a launch button explicitly requests it.
        }
        if (host.effectiveScreenKey === "decide") {
            root.addRuntimeOption(options, 33); // LR2 decide reports both autoplay states true.
        }
        let ghostPosition = vars ? Math.max(0, Math.min(3, vars.ghostPosition || 0)) : 0;
        root.addRuntimeOption(options, 34 + ghostPosition);
        root.addRuntimeOption(options, vars && vars.scoreGraphEnabled === false ? 38 : 39);
        root.addRuntimeOption(options, vars && vars.bgaOn === false ? 40 : 41);
        if (root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 42, 45)) {
            root.addRuntimeOption(options, host.gaugeColorOption(1));
            root.addRuntimeOption(options, host.gaugeColorOption(2));
        }
        root.addGaugeExOption(options, 1);
        root.addGaugeExOption(options, 2);
        root.addRuntimeOption(options, 46); // difficulty filter enabled.
        root.addRuntimeOption(options, host.isLoggedIn() ? 51 : 50);
        root.addRuntimeOption(options, 52); // extra mode off.
        root.addRuntimeOption(options, 54); // 1P autoscratch/assist off.
        root.addRuntimeOption(options, 56); // 2P autoscratch/assist off.
        root.addRuntimeOption(options, 61);
        root.addRuntimeOption(options, host.clearStatusOption());
        if (root.hostGameplayScreen) {
            // LR2 play skins use 80 while the chart is still loading, then
            // switch to 81 when timer 40 (READY / load complete) fires.
            root.addRuntimeOption(options, host.gameplayReadySkinTime >= 0 ? 81 : 80);
        } else {
            root.addRuntimeOption(options, 80); // select/decide are already loaded when the skin is shown.
        }
        root.addRuntimeOption(options, host.gameplayReplayActive() ? 84 : 82);
    }

    function addGaugeExOption(options: var, side: var) : void {
        if (!host.lr2SkinUsesBeatorajaSemantics) {
            return;
        }
        if (!root.runtimeOptionUsed(root.runtimeUsedOptions, side === 2 ? 1047 : 1046)) {
            return;
        }
        let gauge = host.activeGaugeNameForSide(side);
        if (host.gaugeNameUsesBeatorajaExOption(gauge)) {
            root.addOption(options, side === 2 ? 1047 : 1046);
        }
    }

    function appendChartOptions(options: var, chartData: var, fallbackItem: var) : var {
        let usedOptions = root.runtimeUsedOptions;
        let usesStageFileOption = root.runtimeOptionRangeUsed(usedOptions, 190, 191);
        let usesBannerOption = root.runtimeOptionRangeUsed(usedOptions, 192, 193);
        let usesBackBmpOption = root.runtimeOptionRangeUsed(usedOptions, 194, 195);
        let usesBgaOption = root.runtimeOptionRangeUsed(usedOptions, 170, 171);
        let usesLongNoteOption = root.runtimeOptionRangeUsed(usedOptions, 172, 173);
        let usesAttachedTextOption = root.runtimeOptionRangeUsed(usedOptions, 174, 175);
        let usesBpmRangeOption = root.runtimeOptionRangeUsed(usedOptions, 176, 177);
        let usesRandomOption = root.runtimeOptionRangeUsed(usedOptions, 178, 179);
        let usesHighLevelOption = root.runtimeOptionRangeUsed(usedOptions, 185, 186);
        let usesDifficultyOption = root.runtimeOptionRangeUsed(usedOptions, 150, 155);
        let keymode = root.chartKeymode(chartData, fallbackItem);
        let suppressJudgeOption = selectContext.isFolderLikeForLamp(fallbackItem);
        root.appendChartKeymodeOptions(options, keymode);

        if (!chartData) {
            if (usesStageFileOption) {
                root.addOption(options, 190);
            }
            if (usesBannerOption) {
                root.addOption(options, 192);
            }
            if (usesBackBmpOption) {
                root.addOption(options, 194);
            }
            if (usesBgaOption) {
                root.addOption(options, 170);
            }
            if (usesLongNoteOption) {
                root.addOption(options, 172);
            }
            if (usesAttachedTextOption) {
                root.addOption(options, 174);
            }
            if (usesBpmRangeOption) {
                root.addOption(options, 176);
            }
            if (usesRandomOption) {
                root.addOption(options, 178);
            }
            root.setSelectChartDetailRuntimeOptionRange(options,
                                                        180,
                                                        184,
                                                        suppressJudgeOption ? 0 : selectContext.judgeOption(null, fallbackItem));
            root.appendReplayOptions(options, null);
            if (usesDifficultyOption) {
                root.addOption(options, 150);
            }
            return;
        }
        if (usesStageFileOption) {
            root.addOption(options, chartData.stageFile ? 191 : 190);
        }
        if (usesBannerOption) {
            root.addOption(options, chartData.banner ? 193 : 192);
        }
        if (usesBackBmpOption) {
            root.addOption(options, chartData.backBmp ? 195 : 194);
        }
        if (usesBgaOption) {
            root.addOption(options, selectContext.hasBga(chartData) ? 171 : 170);
        }
        if (usesLongNoteOption) {
            root.addOption(options, selectContext.hasLongNote(chartData) ? 173 : 172);
        }
        if (usesAttachedTextOption) {
            root.addOption(options, selectContext.hasAttachedText(chartData) ? 175 : 174);
        }
        if (usesBpmRangeOption) {
            root.addOption(options, (chartData.maxBpm || 0) !== (chartData.minBpm || 0) ? 177 : 176);
        }
        if (root.runtimeOptionUsed(usedOptions, 1177) && host.chartHasBpmStop(chartData)) {
            root.addOption(options, 1177);
        }
        if (usesRandomOption) {
            root.addOption(options, chartData.isRandom ? 179 : 178);
        }
        root.setSelectChartDetailRuntimeOptionRange(options,
                                                    180,
                                                    184,
                                                    suppressJudgeOption ? 0 : selectContext.judgeOption(chartData, fallbackItem));
        if (usesHighLevelOption) {
            root.addOption(options, selectContext.highLevelOption(chartData));
        }
        root.appendReplayOptions(options, chartData);

        if (usesDifficultyOption) {
            let difficulty = selectContext.entryDifficulty(chartData);
            root.addOption(options, difficulty >= 1 && difficulty <= 5 ? 150 + difficulty : 150);
        }

    }

    function replayOptionForSlot(slot: var, available: var) : var {
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

    function selectedReplayOptionForSlot(slot: var) : var {
        return 1205 + Math.max(0, Math.min(3, slot));
    }

    function replaySlotAvailable(chartData: var, slot: var) : var {
        if (!chartData || host.effectiveScreenKey !== "select") {
            return !!chartData && slot === 0 && selectContext.hasReplay(chartData);
        }
        return !!selectContext.replayScoreForType(chartData, slot);
    }

    function appendReplayOptions(options: var, chartData: var) : var {
        if (!root.selectReplayOptionsUsed()) {
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

    function normalizedKeymode(keymode: var) : var {
        let value = Number(keymode || 0);
        return isFinite(value) ? value : 0;
    }

    function chartDataForSelection(chartData: var, fallbackItem: var) : var {
        if (selectContext.chartDataForItem) {
            let chart = selectContext.chartDataForItem(chartData);
            if (chart) {
                return chart;
            }
            chart = selectContext.chartDataForItem(fallbackItem);
            if (chart) {
                return chart;
            }
        }
        return chartData || null;
    }

    function chartDataKeymode(chart: var) : var {
        if (!chart) {
            return 0;
        }
        return root.normalizedKeymode(chart.keymode);
    }

    function chartKeymode(chartData: var, fallbackItem: var) : var {
        return root.chartDataKeymode(root.chartDataForSelection(chartData, fallbackItem));
    }

    function keymodeOptionFor(keymode: var, baseOption: var) : var {
        switch (root.normalizedKeymode(keymode)) {
        case 7:
            return baseOption;
        case 5:
            return baseOption + 1;
        case 14:
            return baseOption + 2;
        case 10:
            return baseOption + 3;
        case 24:
            return 1160;
        case 48:
            return 1161;
        case 9:
            return baseOption + 4;
        default:
            return 0;
        }
    }

    function appendChartKeymodeOption(options: var, keymode: var, baseOption: var) : void {
        let option = root.keymodeOptionFor(keymode, baseOption);
        if (host.effectiveScreenKey === "select") {
            root.addSelectChartDetailRuntimeOption(options, option);
        } else if (option > 0) {
            root.addOption(options, option);
        }
    }

    function effectiveChartDetailKeymode(keymode: var) : var {
        keymode = root.normalizedKeymode(keymode);
        if (!host.spToDpActive() && !host.battleModeActive()) {
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

    function appendChartKeymodeOptions(options: var, keymode: var) : void {
        keymode = root.normalizedKeymode(keymode);
        if (keymode <= 0) {
            return;
        }
        let effectiveKeymode = root.effectiveChartDetailKeymode(keymode);
        // LR2 select MODE labels use the chart's raw keymode. The 165..169
        // family still advertises the effective layout for battle/SP-to-DP.
        let modeLabelKeymode = host.effectiveScreenKey === "select" ? keymode : effectiveKeymode;
        root.appendChartKeymodeOption(options, modeLabelKeymode, 160);
        root.appendChartKeymodeOption(options, effectiveKeymode, 165);
    }

    function chartKeymodeForStatus(item: var, selectedChart: var) : var {
        return root.chartKeymode(selectedChart, item);
    }

    function appendEntryStatusOptions(options: var, item: var, selectedChart: var, scoreSummary: var) : var {
        if (!root.selectEntryStatusOptionsUsed()) {
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
        if (folderLike) {
            return;
        }
        let clearType = "NOPLAY";
        let clearOption = 100;
        let lamp = 0;
        let lr2Lamp = 0;
        let rank = 0;
        if (selectContext.isRankingEntry(item)) {
            clearType = item.bestClearType;
            clearOption = selectContext.beatorajaClearOptionForClearType(clearType);
            lamp = selectContext.clearTypeLamp(clearType);
            lr2Lamp = selectContext.collapsedClearTypeLamp(clearType);
            let points = Number(item.bestPoints || 0);
            let maxPoints = Number(item.maxPoints || 0);
            if (maxPoints > 0) {
                rank = Math.floor(points * 9 / maxPoints);
                if (rank > 7) {
                    rank = 8;
                }
                if (rank < 2 && points > 0) {
                    rank = 1;
                }
            }
        } else {
            let summary = scoreSummary || selectContext.scoreSummaryForItem(item);
            clearType = summary.clearType;
            clearOption = selectContext.beatorajaClearOptionForClearType(clearType);
            lamp = summary.lamp;
            lr2Lamp = selectContext.collapsedClearTypeLamp(clearType);
            rank = summary.rank;
        }
        let hasExactBeatorajaLamp = clearOption >= 1100
            && root.runtimeOptionUsed(root.runtimeUsedOptions, clearOption);
        if (!hasExactBeatorajaLamp && lr2Lamp >= 0 && lr2Lamp <= 5) {
            root.addOption(options, 100 + lr2Lamp);
        }
        root.addOption(options, clearOption);

        if (lamp > 0 && rank >= 1) {
            root.addOption(options, 208 - Math.min(rank, 8));
            root.addOption(options, 118 - Math.min(rank, 8));
        }
    }

    function appendCourseOptions(options: var, item: var) : var {
        if (!selectContext.isCourse(item) || !root.selectCourseDetailOptionsUsed()) {
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

    function appendDifficultyBarOptions(options: var, difficultyModel: var, selectedChart: var) : var {
        if (!root.selectDifficultyBarOptionsUsed()) {
            return;
        }
        let includeLamps = root.selectDifficultyLampOptionsUsed();
        let chartData = root.chartDataForSelection(selectedChart, selectedChart);
        let keymode = root.chartDataKeymode(chartData);
        if (difficultyModel) {
            let optionIds = difficultyModel.optionIdsForKeymode(keymode, includeLamps);
            for (let optionId of optionIds) {
                root.addOption(options, optionId);
            }
            return;
        }

        let difficultyState = chartData && selectContext.difficultyStateForChart
            ? selectContext.difficultyStateForChart(chartData)
            : null;
        let counts = difficultyState ? difficultyState.counts || [] : [];
        let levels = difficultyState ? difficultyState.levels || [] : [];
        let lamps = difficultyState ? difficultyState.lamps || [] : [];
        let flashThreshold = keymode === 5 || keymode === 10 ? 9 : 12;
        for (let diff = 1; diff <= 5; ++diff) {
            let count = counts.length > diff ? counts[diff] || 0 : 0;
            let level = levels.length > diff ? levels[diff] || 0 : 0;
            if (count > 0) {
                root.addOption(options, 504 + diff);
                root.addOption(options, level > flashThreshold ? 74 + diff : 69 + diff);
            } else {
                root.addOption(options, 499 + diff);
            }
            if (count === 1) {
                root.addOption(options, 509 + diff);
            } else if (count > 1) {
                root.addOption(options, 514 + diff);
            }
            if (includeLamps) {
                let lamp = lamps.length > diff ? lamps[diff] || 0 : 0;
                root.addOption(options, 510 + diff * 10 + lamp);
            }
        }
    }

    function appendSelectItemTypeOptions(options: var, item: var) : void {
        let chartData = root.chartDataForSelection(item, null);
        let chartLike = !!chartData
            && !selectContext.isFolderLikeForLamp(item)
            && !selectContext.isCourse(item);
        if (selectContext.isChart(item)
                || selectContext.isEntry(item)
                || selectContext.isRankingEntry(item)
                || chartLike) {
            root.addOption(options, 2);
            if (selectContext.isPlayableBar(item)) {
                root.addOption(options, 5);
            }
        } else if (selectContext.isCourse(item)) {
            root.addOption(options, 3);
            if (selectContext.isPlayableBar(item)) {
                root.addOption(options, 5);
            }
            root.addOption(options, 290);
        } else {
            root.addOption(options, 1);
        }
    }

    function appendSelectedChartModeOptions(options: var, chartData: var, fallbackItem: var) : void {
        let keymode = root.chartKeymode(chartData, fallbackItem);
        root.appendChartKeymodeOptions(options, keymode);
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

    function appendGameplayLaneCoverOptions(options: var, side: var) : var {
        if (side !== 1) {
            return;
        }
        if (!root.runtimeOptionRangeUsed(root.runtimeUsedOptions, 271, 273)) {
            return;
        }
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

    function appendJudgementExistOptions(options: var, resultOrScore: var) : void {
        let usedOptions = root.runtimeUsedOptions;
        if (root.runtimeOptionUsed(usedOptions, 2241)
                && host.judgementCountForExist(resultOrScore, Judgement.Perfect) > 0) {
            root.addOption(options, 2241);
        }
        if (root.runtimeOptionUsed(usedOptions, 2242)
                && host.judgementCountForExist(resultOrScore, Judgement.Great) > 0) {
            root.addOption(options, 2242);
        }
        if (root.runtimeOptionUsed(usedOptions, 2243)
                && host.judgementCountForExist(resultOrScore, Judgement.Good) > 0) {
            root.addOption(options, 2243);
        }
        if (root.runtimeOptionUsed(usedOptions, 2244)
                && host.judgementCountForExist(resultOrScore, Judgement.Bad) > 0) {
            root.addOption(options, 2244);
        }
        if (root.runtimeOptionUsed(usedOptions, 2245)
                && host.judgementCountForExist(resultOrScore, Judgement.Poor) > 0) {
            root.addOption(options, 2245);
        }
        if (root.runtimeOptionUsed(usedOptions, 2246)
                && host.judgementCountForExist(resultOrScore, Judgement.EmptyPoor) > 0) {
            root.addOption(options, 2246);
        }
    }

    function appendGameplaySideOptions(options: var, side: var) : void {
        let score = host.gameplayScore(side);
        let usedOptions = root.runtimeUsedOptions;
        if (root.runtimeOptionRangeUsed(usedOptions, 118, 122)) {
            root.addOption(options, host.gameplayGaugeOption(side));
        }
        if (root.runtimeOptionRangeUsed(usedOptions, 126, 131)) {
            root.addOption(options, host.gameplayLaneOption(score));
        }
        if (root.runtimeOptionRangeUsed(usedOptions, 134, 137)) {
            root.addOption(options, host.gameplayLaneCoverOption(side));
        }
        root.appendGameplayLaneCoverOptions(options, side);

        let currentRankFirst = side === 2 ? 210 : 200;
        if (root.runtimeOptionRangeUsed(usedOptions, currentRankFirst, currentRankFirst + 7)) {
            let currentRank = host.gameplayRankOption(score, currentRankFirst, true);
            if (currentRank >= 0) {
                root.addOption(options, currentRank);
            }
        }
        if (side === 1) {
            if (root.runtimeOptionRangeUsed(usedOptions, 220, 227)) {
                let totalRank = host.gameplayRankOption(score, 220, false);
                if (totalRank >= 0) {
                    root.addOption(options, totalRank);
                }
            }
            if (root.runtimeOptionRangeUsed(usedOptions, 300, 308)) {
                root.addOption(options, host.gameplayExactRankOption(score, 300));
            }
        } else {
            if (root.runtimeOptionRangeUsed(usedOptions, 310, 318)) {
                root.addOption(options, host.gameplayExactRankOption(score, 310));
            }
        }
        let gaugeRangeFirst = side === 2 ? 250 : 230;
        if (root.runtimeOptionRangeUsed(usedOptions, gaugeRangeFirst, gaugeRangeFirst + 10)) {
            host.addGameplayGaugeRangeOption(options, score, gaugeRangeFirst);
        }
        if (side === 1 && root.runtimeOptionUsed(usedOptions, 1240) && host.gameplayGaugeQualified(score)) {
            root.addOption(options, 1240);
        }
        let judgementFirst = side === 2 ? 261 : 241;
        let judgementOption = -1;
        if (root.runtimeOptionRangeUsed(usedOptions, judgementFirst, judgementFirst + 5)) {
            judgementOption = host.gameplayJudgementOption(side, judgementFirst);
            if (judgementOption >= 0) {
                root.addOption(options, judgementOption);
            }
        }
        let timingEarlyOption = side === 2 ? 1262 : 1242;
        let timingLateOption = timingEarlyOption + 1;
        if (root.runtimeOptionUsed(usedOptions, timingEarlyOption)
                || root.runtimeOptionUsed(usedOptions, timingLateOption)) {
            if (judgementOption < 0) {
                judgementOption = host.gameplayJudgementOption(side, judgementFirst);
            }
            let timing = side === 2 ? host.gameplayLastJudgeTiming2 : host.gameplayLastJudgeTiming1;
            if (timing !== 0 && judgementOption >= 0 && judgementOption !== judgementFirst) {
                root.addOption(options, timing > 0 ? timingEarlyOption : timingLateOption);
            }
        }
        let poorBgaOption = side === 2 ? host.gameplayPoorBgaOption2 : host.gameplayPoorBgaOption1;
        if (root.runtimeOptionUsed(usedOptions, poorBgaOption)) {
            root.addOption(options, poorBgaOption);
        }
    }

    function appendGameplayRuntimeOptions(options: var) : void {
        host.gameplayRevision;
        let chartData = host.gameplayChartData();
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendChartOptions(options, chartData);
        root.appendGameplaySideOptions(options, 1);
        if (host.gameplayLanePlayer(2)) {
            root.appendGameplaySideOptions(options, 2);
        }
        root.appendJudgementExistOptions(options, host.gameplayScore(1));
        if (host.gameplayLaneCoverChangingOptionActive()) {
            root.addOption(options, 270);
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

    function resultRankOptionForPoints(points: var, totalNotes: var, baseOption: var) : var {
        let maxPoints = Math.max(0, totalNotes || 0) * 2;
        if (maxPoints <= 0 || points <= 0) {
            return baseOption + 8;
        }
        let rank = Math.floor(points * 9 / maxPoints);
        if (rank >= 8) {
            return baseOption;
        }
        if (rank <= 1) {
            return baseOption + 7;
        }
        return baseOption + (8 - rank);
    }

    function resultTargetRankOption(side: var, baseOption: var) : var {
        let targetScore = host.resultTargetSavedScore(side);
        let targetResult = targetScore && targetScore.result ? targetScore.result : null;
        if (targetResult) {
            return host.resultRankOptionForResult(targetResult, baseOption);
        }
        return root.resultRankOptionForPoints(
            host.resultTargetPoints(side),
            host.resultTotalNotes(host.resultData(side)),
            baseOption);
    }

    function appendResultRuntimeOptions(options: var) : void {
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
        } else {
            root.addOption(options, root.resultTargetRankOption(1, 310));
        }
        root.addOption(options, host.resultRankOptionForResult(host.resultOldBestResult(1), 320));
        root.addOption(options, host.resultRankOptionForResult(current1, 340));

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

    function appendCurrentSelectOptions(options: var, item: var, selectedChart: var, state: var) : void {
        state = state !== undefined ? state : selectContext.selectedState;
        let stateCurrent = state === selectContext.selectedState && selectContext.selectedStateCurrent;
        let selectedItem = item || (state ? state.item : null) || selectContext.focusedItem;
        let explicitChartData = selectedChart !== undefined ? selectedChart : (state ? state.chartData : null);
        let chartData = root.chartDataForSelection(explicitChartData, selectedItem);
        let canUseState = stateCurrent
            && state
            && (selectedItem === state.item || selectedItem === state.chartData)
            && chartData === state.chartData;
        let summary = canUseState ? state.summary : null;
        let difficultyModel = canUseState ? state.difficultyModel : null;

        root.appendSelectItemTypeOptions(options, selectedItem);
        root.appendSelectedChartModeOptions(options, chartData, selectedItem);
        root.appendEntryStatusOptions(options, selectedItem, chartData, summary);
        root.appendCourseOptions(options, selectedItem);
        root.appendRankingStatusOptions(options);

        if (chartData) {
            root.appendDifficultyBarOptions(options, difficultyModel, chartData);
        }

        root.appendChartOptions(options, chartData, selectedItem);
        if (root.selectScoreOptionIdsUsed()) {
            let scoreOptionIds = canUseState ? state.scoreOptionIds : null;
            if (!scoreOptionIds) {
                scoreOptionIds = selectContext.scoreOptionIds(selectedItem);
            }
            for (let optionId of scoreOptionIds) {
                root.addOption(options, optionId);
            }
        }
    }

    function appendDecideOptions(options: var) : void {
        let chartData = host.chart && host.chart.chartData
            ? host.chart.chartData
            : selectContext.selectedStateChartData;
        root.appendSelectItemTypeOptions(options, chartData);
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendEntryStatusOptions(options, chartData, chartData);
        root.appendChartOptions(options, chartData);
        if (root.selectScoreOptionIdsUsed()) {
            for (let optionId of selectContext.scoreOptionIds(chartData)) {
                root.addOption(options, optionId);
            }
        }
    }

    function appendParserActiveOptions(result: var) : void {
        root.addOption(result, 0);
        let staticOptions = skinModel.effectiveActiveOptions && skinModel.effectiveActiveOptions.length
            ? skinModel.effectiveActiveOptions
            : host.parseActiveOptions;
        let length = staticOptions ? staticOptions.length || 0 : 0;
        for (let i = 0; i < length; ++i) {
            let option = staticOptions[i];
            if (!Lr2SkinOptionRules.isRuntimeOwnedOption(option)) {
                root.addOption(result, option);
            }
        }
    }

    function appendStaticSelectOptions(result: var) : void {
        root.appendParserActiveOptions(result);

        root.addOption(result, 20);  // no side panel active
        root.addOption(result, 46);  // difficulty filter enabled
        root.addOption(result, 52);  // non-extra mode
        root.addOption(result, 572); // not course-select mode
        root.addOption(result, 622); // no ghost battle
        root.addOption(result, 624); // no rival score
    }

    function buildBarActiveOptions() : var {
        let result = root.cloneOptionList([]);
        root.appendStaticSelectOptions(result);
        root.addOption(result, selectContext.rankingMode ? 621 : 620);
        return result;
    }

    function appendRankingStatusOptions(result: var) : var {
        if (!root.selectRankingStatusOptionsUsed()) {
            return;
        }
        root.addOption(result, rankingState.currentStatusOption);
        let rankingCount = rankingState.currentPlayerCount;
        if (rankingCount === 0) {
            root.addOption(result, 610);
        }
        for (let threshold = 1; threshold <= 6; ++threshold) {
            if (rankingCount > threshold - 1) {
                root.addOption(result, 610 + threshold);
            }
        }
    }

    function buildBaseActiveOptions(barOptions: var) : var {
        let result = host.effectiveScreenKey === "select"
            ? root.cloneOptionList(barOptions || host.barActiveOptions)
            : root.finalizeOptionList([]);
        if (host.effectiveScreenKey !== "select") {
            root.appendParserActiveOptions(result);
            return result;
        }

        root.appendStaticSelectOptions(result);
        root.addOption(result, 622); // not ghost battle
        root.addOption(result, 624); // not rival compare

        return result;
    }

    function buildSelectCommonActiveOptions(baseOptions: var) : var {
        let result = root.cloneOptionList(baseOptions);
        root.appendCommonRuntimeOptions(result);
        return result;
    }

    function buildSelectRequiredRuntimeActiveOptions() : var {
        let result = root.cloneOptionList([]);
        root.appendRequiredRuntimeOptions(result);
        return result;
    }

    function buildSelectGeneratedRuntimeActiveOptions() : var {
        let stateCurrent = selectContext.selectedStateCurrent;
        let state = stateCurrent ? selectContext.selectedState : null;
        let item = stateCurrent ? state.item : selectContext.focusedItem;
        let chartData = stateCurrent ? state.chartData : selectContext.focusedChartData;
        let result = root.copyActiveOptions([]);
        root.appendCurrentSelectOptions(result, item, chartData, state);
        return result;
    }

    function buildSelectRuntimeActiveOptions(commonOptions: var) : var {
        let result = root.copyActiveOptions(commonOptions);
        root.appendCurrentSelectOptions(result);
        return result;
    }

    function buildRuntimeActiveOptions(baseOptions: var) : var {
        if (host.effectiveScreenKey === "select") {
            return root.buildSelectRuntimeActiveOptions(root.buildSelectCommonActiveOptions(baseOptions));
        }

        let result = root.cloneOptionList(baseOptions);
        if (host.effectiveScreenKey === "decide") {
            root.appendCommonRuntimeOptions(result);
            root.appendDecideOptions(result);
        } else if (root.hostGameplayScreen) {
            root.appendCommonRuntimeOptions(result);
            root.appendGameplayRuntimeOptions(result);
        } else if (root.hostResultScreen) {
            root.appendCommonRuntimeOptions(result);
            root.appendResultRuntimeOptions(result);
        } else {
            root.appendChartOptions(result, host.chart && host.chart.chartData ? host.chart.chartData : null);
        }

        return result;
    }
}
