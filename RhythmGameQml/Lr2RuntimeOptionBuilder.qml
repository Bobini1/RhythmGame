pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var host
    required property var selectContext
    required property var skinModel
    property var selectRuntimeActiveOptionsCache: ({})
    property int selectRuntimeActiveOptionsCacheSize: 0
    readonly property int selectRuntimeActiveOptionsCacheLimit: 256

    function addOption(options: var, option: var) : void {
        host.addOption(options, option);
    }

    function removeOptionRange(options: var, first: var, last: var) : void {
        if (!options) {
            return;
        }
        for (let i = options.length - 1; i >= 0; --i) {
            let option = Math.abs(options[i]);
            if (option >= first && option <= last) {
                options.splice(i, 1);
            }
        }
        options.__lookup = undefined;
        options.__key = undefined;
    }

    function finalizeOptionList(options: var) : var {
        return host.finalizeOptionList(options);
    }

    function runtimeUsedOptionLookup() : var {
        return host.usedOptionFilterActive ? host.usedOptionLookup : null;
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
        for (let i = 0; i < options.length; ++i) {
            if (lookup[Math.abs(options[i])]) {
                return true;
            }
        }
        return false;
    }

    function copyActiveOptions(options: var) : var {
        let result = [];
        let source = options || [];
        let lookup = {};
        let used = host.usedOptionFilterActive ? host.usedOptionLookup : null;
        for (let i = 0; i < source.length; ++i) {
            let option = source[i];
            if (option === undefined || option === null) {
                continue;
            }
            let absOption = Math.abs(option);
            if ((used && !used[absOption])
                    || lookup[option] === true) {
                continue;
            }
            result.push(option);
            lookup[option] = true;
        }
        result.__lookup = lookup;
        return result;
    }

    function clearSelectRuntimeActiveOptionsCache() : void {
        selectRuntimeActiveOptionsCache = ({});
        selectRuntimeActiveOptionsCacheSize = 0;
    }

    function optionListKey(values: var) : var {
        return host.numberArrayKey(values || []);
    }

    function selectItemKindKey(item: var) : var {
        if (selectContext.isRankingEntry(item)) return "ranking";
        if (selectContext.isCourse(item)) return "course";
        if (selectContext.isChart(item)) return "chart";
        if (selectContext.isEntry(item)) return "entry";
        if (selectContext.isFolderLikeForLamp(item)) return "folder";
        return item ? "other" : "none";
    }

    function selectedModeKey(chartData: var) : var {
        let keymode = chartData ? (chartData.keymode || 0) : 0;
        return String(keymode)
            + ":" + String(root.keymodeAfterOptions(keymode))
            + ":" + String(host.spToDpActive() ? 1 : 0)
            + ":" + String(host.battleModeActive() ? 1 : 0);
    }

    function chartOptionSignature(chartData: var) : var {
        let usedOptions = root.runtimeUsedOptionLookup();
        if (!chartData) {
            return "nochart";
        }

        let parts = [
            chartData.stageFile ? 1 : 0,
            chartData.banner ? 1 : 0,
            chartData.backBmp ? 1 : 0,
            selectContext.hasBga(chartData) ? 1 : 0,
            selectContext.hasLongNote(chartData) ? 1 : 0,
            (chartData.maxBpm || 0) !== (chartData.minBpm || 0) ? 1 : 0,
            chartData.isRandom ? 1 : 0,
            selectContext.judgeOption(chartData),
            selectContext.highLevelOption(chartData),
            selectContext.entryDifficulty(chartData),
            chartData.keymode || 0,
            root.keymodeAfterOptions(chartData.keymode || 0)
        ];
        if (root.runtimeOptionRangeUsed(usedOptions, 174, 175)) {
            parts.push(selectContext.hasAttachedText(chartData) ? 1 : 0);
        }
        if (root.runtimeOptionUsed(usedOptions, 1177)) {
            parts.push(host.chartHasBpmStop(chartData) ? 1 : 0);
        }
        return parts.join(":");
    }

    function entryStatusSignature(item: var, chartData: var, state: var) : var {
        if (!host.selectUsesEntryStatusOptions()) {
            return "";
        }
        if (!item) {
            return "none";
        }
        if (selectContext.isRankingEntry(item)) {
            return "ranking:" + String(item.bestClearType || "")
                + ":" + String(item.bestPoints || 0)
                + ":" + String(item.maxPoints || 0);
        }
        if (selectContext.isFolderLikeForLamp(item)) {
            return "folder";
        }
        if (!selectContext.isChart(item) && !selectContext.isEntry(item)) {
            return "other";
        }
        if (!chartData || (chartData.keymode || 0) <= 0) {
            return "no-keymode";
        }
        let summary = state && state.summary ? state.summary : selectContext.scoreSummaryForItem(item);
        return "score:" + String(summary ? summary.clearType || "" : "")
            + ":" + String(summary ? summary.lamp || 0 : 0)
            + ":" + String(summary ? summary.rank || 0 : 0);
    }

    function courseSignature(item: var) : var {
        if (!host.selectUsesCourseDetailOptions() || !selectContext.isCourse(item) || !item.loadCharts) {
            return "";
        }
        let stages = item.loadCharts();
        let parts = [Math.min(10, stages.length)];
        for (let stage = 0; stage < Math.min(5, stages.length); ++stage) {
            parts.push(selectContext.entryDifficulty(stages[stage]));
        }
        return parts.join(":");
    }

    function difficultyBarSignature(difficultyState: var, selectedChart: var) : var {
        if (!host.selectUsesDifficultyBarOptions() || !selectedChart) {
            return "";
        }
        let counts = difficultyState && difficultyState.counts ? difficultyState.counts : [];
        let levels = difficultyState && difficultyState.levels ? difficultyState.levels : [];
        let lamps = difficultyState && difficultyState.lamps ? difficultyState.lamps : [];
        let keymode = selectedChart ? (selectedChart.keymode || 0) : 0;
        let flashThreshold = keymode === 5 || keymode === 10 ? 9 : 12;
        let parts = [keymode, flashThreshold];
        for (let diff = 1; diff <= 5; ++diff) {
            parts.push(counts[diff] || 0);
            parts.push((levels[diff] || 0) > flashThreshold ? 1 : 0);
            parts.push(lamps[diff] || 0);
        }
        return parts.join(":");
    }

    function replaySignature(chartData: var) : var {
        if (!host.selectUsesReplayOptions()) {
            return "";
        }
        let parts = [host.lr2ReplayType || 0];
        for (let slot = 0; slot < 4; ++slot) {
            parts.push(root.replaySlotAvailable(chartData, slot) ? 1 : 0);
        }
        return parts.join(":");
    }

    function scoreOptionIdsSignature(item: var, state: var) : var {
        if (!host.selectUsesScoreOptionIds()) {
            return "";
        }
        let stateCurrent = state && state.scoreRevision === selectContext.scoreRevision
            && state.listRevision === selectContext.listRevision;
        let scoreOptionIds = stateCurrent && state ? state.scoreOptionIds : null;
        if (!scoreOptionIds) {
            let summary = stateCurrent && state ? state.summary : null;
            scoreOptionIds = summary
                ? selectContext.scoreOptionIdsFromSummary(summary)
                : selectContext.scoreOptionIds(item);
            if (stateCurrent && state) {
                state.scoreOptionIds = scoreOptionIds;
            }
        }
        return root.optionListKey(scoreOptionIds);
    }

    function selectStateOptionsSignature(state: var) : var {
        let item = state ? state.item : selectContext.focusedItem;
        let chartData = state ? state.chartData : null;
        let difficultyState = state ? state.difficultyState : null;
        return [
            root.selectItemKindKey(item),
            root.selectedModeKey(chartData),
            root.chartOptionSignature(chartData),
            root.entryStatusSignature(item, chartData, state),
            root.courseSignature(item),
            root.difficultyBarSignature(difficultyState, chartData),
            root.replaySignature(chartData),
            root.scoreOptionIdsSignature(item, state)
        ].join("|");
    }

    function selectRuntimeActiveOptionsCacheKey(commonOptions: var, state: var) : var {
        state = state !== undefined ? state : selectContext.selectedState();
        let commonKey = host.selectCommonActiveOptionsKey !== undefined
            ? host.selectCommonActiveOptionsKey
            : host.numberArrayKey(commonOptions || []);
        let rankingKey = "";
        if (host.selectUsesRankingStatusOptions()) {
            rankingKey = String(host.lr2RankingStatusOption()) + ":" + String(host.lr2RankingPlayerCount());
        }
        let featureKey = (host.selectUsesReplayOptions() ? "1" : "0")
            + (host.selectUsesScoreOptionIds() ? "1" : "0")
            + (host.selectUsesEntryStatusOptions() ? "1" : "0")
            + (host.selectUsesDifficultyBarOptions() ? "1" : "0")
            + (host.selectUsesCourseDetailOptions() ? "1" : "0")
            + (host.selectUsesRankingStatusOptions() ? "1" : "0");
        return commonKey
            + "|" + featureKey
            + "|" + root.selectStateOptionsSignature(state)
            + "|" + String(selectContext.rankingMode ? 1 : 0)
            + "|" + String(host.selectPanel || 0)
            + "|" + String(host.lr2ReplayType || 0)
            + "|" + rankingKey;
    }

    function cachedSelectRuntimeActiveOptions(cacheKey: var) : var {
        let cached = selectRuntimeActiveOptionsCache[cacheKey];
        return cached !== undefined ? cached : null;
    }

    function storeSelectRuntimeActiveOptions(cacheKey: var, options: var) : var {
        if (selectRuntimeActiveOptionsCacheSize >= selectRuntimeActiveOptionsCacheLimit) {
            root.clearSelectRuntimeActiveOptionsCache();
        }
        if (selectRuntimeActiveOptionsCache[cacheKey] === undefined) {
            selectRuntimeActiveOptionsCacheSize += 1;
        }
        selectRuntimeActiveOptionsCache[cacheKey] = options;
        return options;
    }

    function appendCommonRuntimeOptions(options: var) : void {
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
        if (host.isGameplayScreen()) {
            root.addOption(options, host.gameplayGaugeTrophyOption(1));
            root.addOption(options, host.gameplayGaugeTrophyOption(2));
        }
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

    function appendPanelOptions(options: var) : void {
        root.addOption(options, host.selectPanel > 0 ? 20 + host.selectPanel : 20);
    }

    function appendChartOptions(options: var, chartData: var) : var {
        let usedOptions = root.runtimeUsedOptionLookup();
        let allowStageFileOption = host.effectiveScreenKey !== "decide";
        let usesStageFileOption = allowStageFileOption
            && root.runtimeOptionRangeUsed(usedOptions, 190, 191);
        let usesBannerOption = root.runtimeOptionRangeUsed(usedOptions, 192, 193);
        let usesBackBmpOption = root.runtimeOptionRangeUsed(usedOptions, 194, 195);
        let usesBgaOption = root.runtimeOptionRangeUsed(usedOptions, 170, 171);
        let usesLongNoteOption = root.runtimeOptionRangeUsed(usedOptions, 172, 173);
        let usesAttachedTextOption = root.runtimeOptionRangeUsed(usedOptions, 174, 175);
        let usesBpmRangeOption = root.runtimeOptionRangeUsed(usedOptions, 176, 177);
        let usesRandomOption = root.runtimeOptionRangeUsed(usedOptions, 178, 179);
        let usesJudgeOption = root.runtimeOptionRangeUsed(usedOptions, 180, 183);
        let usesHighLevelOption = root.runtimeOptionRangeUsed(usedOptions, 185, 186);
        let usesDifficultyOption = root.runtimeOptionRangeUsed(usedOptions, 150, 155);
        let usesKeymodeOption = root.runtimeOptionRangeUsed(usedOptions, 160, 169)
            || root.runtimeAnyOptionUsed(usedOptions, [1160, 1161]);

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
            root.appendReplayOptions(options, null);
            if (usesDifficultyOption) {
                root.removeOptionRange(options, 150, 155);
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
        if (usesJudgeOption) {
            root.addOption(options, selectContext.judgeOption(chartData));
        }
        if (usesHighLevelOption) {
            root.addOption(options, selectContext.highLevelOption(chartData));
        }
        root.appendReplayOptions(options, chartData);

        if (usesDifficultyOption) {
            let difficulty = selectContext.entryDifficulty(chartData);
            root.removeOptionRange(options, 150, 155);
            root.addOption(options, difficulty >= 1 && difficulty <= 5 ? 150 + difficulty : 150);
        }

        if (usesKeymodeOption) {
            let keymode = chartData.keymode || 0;
            root.appendKeymodeOption(options, keymode, 160);
            root.appendKeymodeOption(options, root.keymodeAfterOptions(keymode), 165);
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

    function keymodeOptionFor(keymode: var, baseOption: var) : var {
        switch (keymode) {
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
            return 164;
        default:
            return 0;
        }
    }

    function appendKeymodeOption(options: var, keymode: var, baseOption: var) : void {
        let option = root.keymodeOptionFor(keymode, baseOption);
        if (option > 0) {
            root.addOption(options, option);
        }
    }

    function keymodeAfterOptions(keymode: var) : var {
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

    function chartKeymodeForStatus(item: var, selectedChart: var) : var {
        if (selectedChart && selectedChart.keymode) {
            return selectedChart.keymode;
        }
        if ((selectContext.isChart(item) || selectContext.isEntry(item)) && item.keymode) {
            return item.keymode;
        }
        return 0;
    }

    function appendEntryStatusOptions(options: var, item: var, selectedChart: var, scoreSummary: var) : var {
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
        if (folderLike) {
            return;
        }
        let clearOption = 100;
        let lamp = 0;
        let rank = 0;
        if (selectContext.isRankingEntry(item)) {
            clearOption = selectContext.beatorajaClearOptionForClearType(item.bestClearType);
            lamp = selectContext.clearTypeLamp(item.bestClearType);
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
            clearOption = selectContext.beatorajaClearOptionForClearType(summary.clearType);
            lamp = summary.lamp;
            rank = summary.rank;
        }
        let hasExactBeatorajaLamp = clearOption >= 1100
            && root.runtimeOptionUsed(root.runtimeUsedOptionLookup(), clearOption);
        if (!hasExactBeatorajaLamp && lamp >= 0 && lamp <= 5) {
            root.addOption(options, 100 + lamp);
        }
        root.addOption(options, clearOption);

        if (lamp > 0 && rank >= 1) {
            root.addOption(options, 208 - Math.min(rank, 8));
            root.addOption(options, 118 - Math.min(rank, 8));
        }
    }

    function appendCourseOptions(options: var, item: var) : var {
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

    function appendDifficultyBarOptions(options: var, difficultyState: var, selectedChart: var) : var {
        if (!host.selectUsesDifficultyBarOptions()) {
            return;
        }
        let counts = difficultyState && difficultyState.counts ? difficultyState.counts : [];
        let levels = difficultyState && difficultyState.levels ? difficultyState.levels : [];
        let lamps = difficultyState && difficultyState.lamps ? difficultyState.lamps : [];
        let keymode = selectedChart ? (selectedChart.keymode || 0) : 0;
        let flashThreshold = keymode === 5 || keymode === 10 ? 9 : 12;
        for (let diff = 1; diff <= 5; ++diff) {
            let diffCount = counts[diff] || 0;
            if (diffCount > 0) {
                root.addOption(options, 504 + diff);
                root.addOption(options, (levels[diff] || 0) > flashThreshold ? 74 + diff : 69 + diff);
            } else {
                root.addOption(options, 499 + diff);
            }

            if (diffCount === 1) {
                root.addOption(options, 509 + diff);
            } else if (diffCount > 1) {
                root.addOption(options, 514 + diff);
            }
            root.addOption(options, 510 + diff * 10 + (lamps[diff] || 0));
        }
    }

    function appendSelectItemTypeOptions(options: var, item: var) : void {
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

    function appendSelectedChartModeOptions(options: var, chartData: var) : void {
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

    function appendGameplayLaneCoverOptions(options: var, side: var) : var {
        if (side !== 1) {
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

    function appendGameplaySideOptions(options: var, side: var) : void {
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

    function appendResultRuntimeOptions(options: var) : void {
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

    function appendCurrentSelectOptions(options: var, item: var, selectedChart: var, state: var) : void {
        state = state !== undefined ? state : selectContext.selectedState();
        let stateCurrent = state && state.scoreRevision === selectContext.scoreRevision
            && state.listRevision === selectContext.listRevision;
        let selectedItem = item || (state ? state.item : null);
        let chartData = selectedChart !== undefined ? selectedChart : (state ? state.chartData : null);
        let canUseState = stateCurrent
            && state
            && (selectedItem === state.item || selectedItem === state.chartData)
            && chartData === state.chartData;
        let summary = canUseState ? state.summary : null;
        let difficultyState = canUseState ? state.difficultyState : null;

        root.appendSelectItemTypeOptions(options, selectedItem);
        root.appendSelectedChartModeOptions(options, chartData);
        root.appendEntryStatusOptions(options, selectedItem, chartData, summary);
        root.appendCourseOptions(options, selectedItem);
        root.appendRankingStatusOptions(options);

        if (chartData) {
            root.appendDifficultyBarOptions(options, difficultyState, chartData);
        }

        root.appendChartOptions(options, chartData);
        if (host.selectUsesScoreOptionIds()) {
            let scoreOptionIds = canUseState ? state.scoreOptionIds : null;
            if (!scoreOptionIds) {
                scoreOptionIds = summary
                    ? selectContext.scoreOptionIdsFromSummary(summary)
                    : selectContext.scoreOptionIds(selectedItem);
                if (canUseState) {
                    state.scoreOptionIds = scoreOptionIds;
                }
            }
            for (let optionId of scoreOptionIds) {
                root.addOption(options, optionId);
            }
        }
    }

    function appendDecideOptions(options: var) : void {
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

    function runtimeOwnsOptionPair(option: var) : var {
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

    function appendParserActiveOptions(result: var) : void {
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
        let result = root.finalizeOptionList([]);
        root.appendStaticSelectOptions(result);
        root.addOption(result, selectContext.rankingMode ? 621 : 620);
        return result;
    }

    function appendRankingStatusOptions(result: var) : var {
        if (!host.selectUsesRankingStatusOptions()) {
            return;
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
    }

    function buildBaseActiveOptions(barOptions: var) : var {
        let result = host.effectiveScreenKey === "select"
            ? root.finalizeOptionList((barOptions || host.barActiveOptions).slice())
            : root.finalizeOptionList([]);
        if (host.effectiveScreenKey !== "select") {
            root.appendParserActiveOptions(result);
            return result;
        }

        root.addOption(result, 622); // not ghost battle
        root.addOption(result, 624); // not rival compare
        root.appendPanelOptions(result);

        return result;
    }

    function buildSelectCommonActiveOptions(baseOptions: var) : var {
        let result = root.finalizeOptionList((baseOptions || []).slice());
        root.appendCommonRuntimeOptions(result);
        return result;
    }

    function buildSelectRuntimeActiveOptions(commonOptions: var) : var {
        let state = selectContext.selectedState();
        let chartData = state ? state.chartData : null;
        let item = selectContext.focusedItem;
        let cacheKey = root.selectRuntimeActiveOptionsCacheKey(commonOptions, state);
        let cached = root.cachedSelectRuntimeActiveOptions(cacheKey);
        if (cached) {
            return cached;
        }
        let result = root.copyActiveOptions(commonOptions);
        root.appendCurrentSelectOptions(result, item, chartData, state);
        return root.storeSelectRuntimeActiveOptions(cacheKey, result);
    }

    function buildRuntimeActiveOptions(baseOptions: var) : var {
        if (host.effectiveScreenKey === "select") {
            return root.buildSelectRuntimeActiveOptions(root.buildSelectCommonActiveOptions(baseOptions));
        }

        let result = root.finalizeOptionList(baseOptions.slice());
        if (host.effectiveScreenKey === "decide") {
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
