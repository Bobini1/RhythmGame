pragma ValueTypeBehavior: Addressable
import RhythmGameQml

import QtQuick

Item {
    id: root

    property var items: []
    property var folderContents: []
    property var historyStack: []
    property var scores: ({})
    property var folderLampByKey: ({})
    property var folderScoreCountsByKey: ({})
    property var pendingFolderLampByKey: ({})
    property var pendingFolderScoreCountsByKey: ({})
    property var pendingFolderDistributionByKey: ({})
    property int pendingFolderLampPublishCount: 0
    property bool folderLampPublishQueued: false
    property bool selectItemScoreSummaryPublishQueued: false
    property var chartGroupsByFolderKeymode: ({})
    property var chartDifficultyByPath: ({})
    property var selectedDifficultyStateCache: ({})
    property var selectedDifficultyGroupStateCache: ({})
    property var playerStats: ({
        playCount: 0,
        clearCount: 0,
        failCount: 0,
        perfectCount: 0,
        greatCount: 0,
        goodCount: 0,
        badCount: 0,
        poorCount: 0,
        maxCombo: 0
    })
    property var previewFiles: ({})
    property alias currentIndex: nativeNavigation.currentIndex
    property alias targetIndex: nativeNavigation.targetIndex
    property alias visualIndex: visualState.visualIndex
    property alias targetVisualIndex: nativeNavigation.targetVisualIndex
    readonly property int listTopbarFixed: visualState.rawFixed
    readonly property int listCalculatedBarFixed: visualState.fixed
    property alias oldBarFixed: nativeNavigation.oldBarFixed
    property alias nowBarFixed: nativeNavigation.nowBarFixed
    property int pendingWheelSteps: 0
    property alias selectedOffset: nativeNavigation.selectedOffset
    property alias barMoveStartMs: nativeNavigation.barMoveStartMs
    property alias barMoveEndMs: nativeNavigation.barMoveEndMs
    property bool componentReady: false
    property bool updatesActive: true
    property alias suppressVisualIndexPublish: nativeNavigation.suppressVisualIndexPublish
    property alias scrollDirection: nativeNavigation.scrollDirection
    property int listGeneration: 0
    property int scoreGeneration: 0
    property int folderLampRequestToken: 0
    property alias suppressNextSelectionSound: nativeNavigation.suppressNextSelectionSound
    property bool scrollFixedPointDragging: false
    property string searchText: ""
    property var attachedTextByDirectory: ({})
    property var hasAttachedTextByDirectory: ({})
    property int difficultyFilter: 0
    property var generalVars: null
    property int localSelectKeymodeFilter: SelectKeymodeFilter.All
    property int localSelectSortMode: SelectSortMode.Title
    readonly property int selectKeymodeFilter: generalVars ? generalVars.selectKeymodeFilter : localSelectKeymodeFilter
    readonly property int selectSortMode: generalVars ? generalVars.selectSortMode : localSelectSortMode
    readonly property int keyFilter: keyFilterFrameForSelectKeymodeFilter(selectKeymodeFilter, beatorajaKeyFilterOrderActive())
    readonly property int sortMode: legacySortModeForSelectSortMode(selectSortMode)
    property int sortSourceFrameCount: 5
    property var barTitleTypes: []
    property var barLampVariants: []
    property bool useBeatorajaBarTextTypes: false
    property bool useBeatorajaSelectOptions: false
    property bool scoreOptionIdsUsed: true
    property bool difficultyStateUsed: true
    property bool difficultyLampStateUsed: true
    property int realItemCount: 0
    property int barRowCount: 0
    property int barCenter: 0
    readonly property int barLabelLongNote: 0
    readonly property int barLabelRandom: 1
    readonly property int barLabelMine: 2
    property alias visibleBarSlotOffset: visibleBarModel.slotOffset
    property alias visibleBarTextCells: visibleBarModel.cells
    property alias lastSyncedCursorBaseIndex: nativeNavigation.lastSyncedCursorBaseIndex
    property bool rankingMode: false
    property var rankingBaseItem: null
    property string rankingStatsMd5: ""
    property var rankingClearCounts: ({})
    property int rankingPlayerRank: 0
    property int rankingPlayerCount: 0
    property int rankingTotalPlayCount: 0
    property var rankingSavedItems: []
    property var rankingSavedFolderContents: []
    property int rankingSavedRealItemCount: 0
    property int rankingSavedCurrentIndex: 0
    property real rankingSavedVisualIndex: 0
    property int rankingSavedSelectedOffset: 0
    readonly property var emptyScoreList: []
    readonly property var emptyScoreOptionIds: []
    readonly property var emptyDifficultyNumbers: [0, 0, 0, 0, 0, 0]
    readonly property var emptyScoreCounts: ({
        play: 0,
        clear: 0,
        fail: 0,
        noplay: 0,
        assist: 0,
        lightAssist: 0,
        easy: 0,
        normal: 0,
        hard: 0,
        exhard: 0,
        fc: 0,
        perfect: 0,
        max: 0,
        minBadPoor: 0
    })
    readonly property var emptyFolderScoreCounts: ({
        total: 0,
        play: 0,
        clear: 0,
        fail: 0,
        noplay: 0,
        assist: 0,
        lightAssist: 0,
        easy: 0,
        normal: 0,
        hard: 0,
        exhard: 0,
        fc: 0,
        perfect: 0,
        max: 0
    })
    readonly property var emptyFolderGraphLamps: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    readonly property var emptyFolderGraphRanks: [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ]
    readonly property var emptyDifficultyState: ({
        key: "empty",
        counts: emptyDifficultyNumbers,
        levels: emptyDifficultyNumbers,
        lamps: emptyDifficultyNumbers,
        selectedDifficulty: 0
    })
    property alias navigationController: nativeNavigation
    property var visualChartWrapper: null
    readonly property var selectedState: selectedDetailState
    readonly property var selectedStateItem: selectedDetailState.item
    readonly property var selectedStateChartData: selectedDetailState.chartData
    readonly property bool selectedStateScoreCurrent: selectedDetailState.scoreGeneration === scoreGeneration
    readonly property bool selectedStateListCurrent: selectedDetailState.listGeneration === listGeneration
    readonly property bool selectedStateCurrent: selectedStateScoreCurrent && selectedStateListCurrent
    readonly property string selectedPreviewAudioSource: {
        let chartData = selectedStateChartData;
        return chartData ? (previewFiles[chartData.chartDirectory] || "") : "";
    }
    property string visualStageFileSource: ""
    property string visualBackBmpSource: ""
    property string visualBannerSource: ""
    property bool stageFileSourceUsed: true
    property bool backBmpSourceUsed: true
    property bool bannerSourceUsed: true
    onScoreOptionIdsUsedChanged: {
        selectedDetailState.clearScoreSummaryCache();
        queueSelectItemScoreSummaries();
    }
    onDifficultyStateUsedChanged: clearSelectedDifficultyStateCache()
    onDifficultyLampStateUsedChanged: clearSelectedDifficultyStateCache()
    onStageFileSourceUsedChanged: refreshVisualChartAssetSources()
    onBackBmpSourceUsedChanged: refreshVisualChartAssetSources()
    onBannerSourceUsedChanged: refreshVisualChartAssetSources()
    function clearSelectedDifficultyStateCache() : void {
        selectedDifficultyStateCache = ({});
        selectedDifficultyGroupStateCache = ({});
    }

    function clearScoreLoadedStateCaches() : void {
        selectedDetailState.clearScoreSummaryCache();
        clearSelectedDifficultyStateCache();
    }

    function chartAssetUrl(chartData: var, fileName: var) : var {
        if (!chartData || !fileName || !chartData.chartDirectory) {
            return "";
        }
        let dir = String(chartData.chartDirectory).replace(/\\/g, "/");
        if (dir[0] !== "/") {
            dir = "/" + dir;
        }
        return "file://" + dir + String(fileName).replace(/\.[^/.]+$/, "");
    }

    function refreshVisualChartAssetSources(chartData: var) : void {
        chartData = chartData !== undefined ? chartData : visualChartWrapper;
        let nextStageFileSource = stageFileSourceUsed
            ? chartAssetUrl(chartData, chartData ? chartData.stageFile : "")
            : "";
        if (visualStageFileSource !== nextStageFileSource) {
            visualStageFileSource = nextStageFileSource;
        }
        let nextBackBmpSource = backBmpSourceUsed
            ? chartAssetUrl(chartData, chartData ? chartData.backBmp : "")
            : "";
        if (visualBackBmpSource !== nextBackBmpSource) {
            visualBackBmpSource = nextBackBmpSource;
        }
        let nextBannerSource = bannerSourceUsed
            ? chartAssetUrl(chartData, chartData ? chartData.banner : "")
            : "";
        if (visualBannerSource !== nextBannerSource) {
            visualBannerSource = nextBannerSource;
        }
    }

    readonly property int lr2SpeedFirst: 300
    readonly property int lr2SpeedNext: 70
    readonly property int lr2WheelDuration: 200
    readonly property int lr2ClickDuration: 200
    readonly property int lr2ScrollUp: 1
    readonly property int lr2ScrollDown: 2
    readonly property var lr2SortOrder: [0, 1, 2, 3, 4]
    readonly property var beatorajaSortOrder: [2, 5, 6, 7, 1, 3, 4, 8]
    readonly property var lr2KeyFilterOrder: [0, 1, 2, 3, 4, 5, 6]
    readonly property var beatorajaKeyFilterOrder: [0, 1, 2, 3, 4]
    readonly property var clearTypePriorities: ["NOPLAY", "FAILED", "AEASY", "LIGHTASSIST", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"]

    readonly property int count: items.length
    readonly property int logicalCount: realItemCount > 0 ? realItemCount : count
    readonly property int focusedIndex: logicalCount > 0 ? currentIndex : 0
    readonly property var focusedItem: logicalCount > 0 ? selectItemModel.currentItem : null
    readonly property var current: focusedItem
    readonly property var focusedChartData: {
        if (rankingMode && rankingBaseItem) {
            return rankingBaseItem;
        }
        return focusedItem instanceof ChartData || focusedItem instanceof entry ? focusedItem : null;
    }
    readonly property var focusedSelectionTarget: isRankingEntry(focusedItem)
        ? focusedItem
        : (focusedChartData || focusedItem)
    readonly property string focusedSelectionKey: {
        return selectItemModel.currentKey || entrySelectionKey(focusedItem, focusedIndex);
    }
    readonly property string focusedSelectionTargetKey: focusedSelectionTarget !== focusedItem
        ? entrySelectionKey(focusedSelectionTarget, focusedIndex)
        : ""
    readonly property int visualDirection: scrollDirection === lr2ScrollDown ? 1 : (scrollDirection === lr2ScrollUp ? -1 : 0)
    readonly property bool visualMoveActive: Math.abs(nowBarFixed - visualState.rawFixed) > 0
    readonly property real normalizedVisualIndex: logicalCount > 0 ? visualState.fixed / 1000.0 : 0
    property alias visualStateObject: visualState

    Lr2SelectVisualState {
        id: visualState
        logicalCount: root.logicalCount
        scrollDirection: root.scrollDirection
        scrollDownDirection: root.lr2ScrollDown
    }

    Lr2SelectItemModel {
        id: selectItemModel
        currentIndex: root.currentIndex
        items: root.normalizedSelectModelItems(root.items)
    }

    Lr2SelectBarModel {
        id: visibleBarModel
        sourceModel: selectItemModel
        logicalCount: root.logicalCount
        rowCountLimit: Math.max(0, root.barRowCount || 0)
        centerRow: root.barCenter
        currentIndex: visualState.baseIndex
    }

    Lr2SelectDetailState {
        id: selectedDetailState
    }

    Lr2SelectNavigationController {
        id: nativeNavigation
        visualState: visualState
        updatesActive: root.updatesActive
        rankingMode: root.rankingMode
        logicalCount: root.logicalCount
        lr2SpeedFirst: root.lr2SpeedFirst
        lr2SpeedNext: root.lr2SpeedNext
        lr2ScrollUp: root.lr2ScrollUp
        lr2ScrollDown: root.lr2ScrollDown
        onFocusedStateRefreshRequested: {
            nativeNavigation.completeFocusedStateRefresh(root.refreshSelectedScoreState());
        }
        onEntryChangeSoundsRequested: function(count) {
            root.entryChangeSoundsRequested(count);
        }
    }

    onUpdatesActiveChanged: {
        if (!componentReady) {
            return;
        }
        if (!updatesActive) {
            pendingWheelSteps = 0;
            pendingWheelStepTimer.stop();
        } else {
            publishPendingFolderLamps();
            nativeNavigation.publishCursorBaseIndex(true);
        }
    }

    onBarLampVariantsChanged: {
        clearSelectedDifficultyStateCache();
        queueSelectItemScoreSummaries();
        refreshFolderLampSkinData();
    }
    onBarTitleTypesChanged: queueSelectItemScoreSummaries()
    onUseBeatorajaBarTextTypesChanged: queueSelectItemScoreSummaries()
    onUseBeatorajaSelectOptionsChanged: {
        selectedDetailState.clearScoreSummaryCache();
        clearSelectedDifficultyStateCache();
        refreshSkinSemanticData();
    }

    signal openedFolder()
    signal entryChangeSoundsRequested(int count)

    function markListContentsChanged() : void {
        listGeneration += 1;
        nativeNavigation.invalidateFocusedState();
        queueSelectItemScoreSummaries();
    }

    function refreshFolderLampSkinData() : void {
        if (componentReady) {
            refreshFolderLamps();
        }
    }

    function refreshSkinSemanticData() : void {
        if (componentReady) {
            nativeNavigation.invalidateFocusedState();
            nativeNavigation.refreshFocusedState();
            queueSelectItemScoreSummaries();
            refreshFolderLamps();
        }
    }

    function wrapBarFixed(value: var) : var {
        let span = logicalCount * 1000;
        if (span <= 0) {
            return 0;
        }
        return ((value % span) + span) % span;
    }

    function clampBarFixed(value: var) : var {
        let maxFixed = Math.max(0, logicalCount * 1000 - 1);
        return Math.max(0, Math.min(maxFixed, Math.round(value)));
    }

    function lr2ChangeValueByTime(from: var, to: var, start: var, end: var, now: var) : var {
        if (end <= start || now >= end) {
            return to;
        }
        if (now <= start) {
            return from;
        }
        return Math.trunc(from + (to - from) * ((now - start) / (end - start)));
    }

    function setVisualIndexRaw(index: var) : void {
        suppressVisualIndexPublish = true;
        visualState.jumpTo(index);
        suppressVisualIndexPublish = false;
    }

    function setVisualIndexImmediate(index: var) : void {
        let fixed = Math.round(index * 1000);
        oldBarFixed = fixed;
        nowBarFixed = fixed;
        setVisualIndexRaw(index);
        targetVisualIndex = index;
        barMoveStartMs = 0;
        barMoveEndMs = 0;
        scrollDirection = 0;
        nativeNavigation.publishCursorBaseIndex();
    }

    function beginVisualMove(durationMs: var, now: var) : var {
        if (now === undefined) {
            now = Date.now();
        }
        barMoveStartMs = now;
        barMoveEndMs = now + Math.max(1, durationMs);
        setVisualIndexRaw(listTopbarFixed / 1000.0);
        targetVisualIndex = nowBarFixed / 1000.0;
        suppressVisualIndexPublish = true;
        visualState.startAnimation(oldBarFixed / 1000.0,
                                   targetVisualIndex,
                                   Math.max(1, durationMs),
                                   now);
        suppressVisualIndexPublish = false;
        return nativeNavigation.publishCursorBaseIndex();
    }

    function normalizedVisualIndexFor(index: var) : var {
        return normalizedVisualIndexForCount(index, logicalCount);
    }

    function normalizedVisualIndexForCount(index: var, itemCount: var) : var {
        if (itemCount <= 0) {
            return 0;
        }
        return ((index % itemCount) + itemCount) % itemCount;
    }

    function baseIndexForVisual(index: var) : var {
        return Math.floor(normalizedVisualIndexFor(index));
    }

    function visualRemainderFor(index: var) : var {
        let normalized = normalizedVisualIndexFor(index);
        return normalized - Math.floor(normalized);
    }

    function cursorBaseIndexForVisual(index: var) : var {
        return cursorBaseIndexForFixed(Math.floor(normalizedVisualIndexFor(index) * 1000));
    }

    function cursorBaseIndexForFixed(fixed: var) : var {
        let normalized = wrapBarFixed(fixed);
        let base = Math.floor(normalized / 1000);
        // OpenLR2 renders with floor(listCalculatedBar), but GetSongCursor()
        // advances by one while scrolling down and the fixed-point remainder is
        // non-zero. This keeps the selected item identity in lockstep with LR2
        // without changing the stationary glow row.
        if (normalized % 1000 !== 0 && scrollDirection === lr2ScrollDown) {
            base += 1;
        }
        return base;
    }

    function updateVisualIndex(now: var) : var {
        if (!updatesActive) {
            return;
        }
        visualState.advanceAnimation(now);
        if (pendingWheelSteps !== 0) {
            let steps = pendingWheelSteps;
            pendingWheelSteps = 0;
            nativeNavigation.applyLr2ScrollDelta(-steps, lr2WheelDuration, now, visualState.rawFixed);
        }
    }

    function queueWheelSteps(steps: var) : var {
        if (!updatesActive) {
            return;
        }
        pendingWheelSteps += steps;
        pendingWheelStepTimer.restart();
    }

    Timer {
        id: pendingWheelStepTimer
        interval: 0
        repeat: false
        onTriggered: root.updateVisualIndex(Date.now())
    }

    Component.onCompleted: {
        componentReady = true;
        if (updatesActive) {
            nativeNavigation.publishCursorBaseIndex(true);
        }
    }

    function nearestVisualIndex(index: var, anchor: var) : var {
        if (logicalCount === 0) {
            return 0;
        }
        let target = index;
        while (target - anchor > logicalCount / 2) {
            target -= logicalCount;
        }
        while (target - anchor < -logicalCount / 2) {
            target += logicalCount;
        }
        return target;
    }

    function isChart(item: var) : var {
        return item instanceof ChartData;
    }

    function isEntry(item: var) : var {
        return item instanceof entry;
    }

    function isCourse(item: var) : var {
        return item instanceof course;
    }

    function isPlayableChartLike(item: var) : var {
        if (isRankingEntry(item)) {
            item = rankingBaseItem;
        }
        return isChart(item) && !!item.path;
    }

    function isMissingTableEntry(item: var) : var {
        return isEntry(item);
    }

    function courseStages(item: var) : var {
        return isCourse(item) && item.loadCharts ? item.loadCharts() : [];
    }

    function isPlayableCourse(item: var) : var {
        let stages = courseStages(item);
        if (stages.length <= 0) {
            return false;
        }
        for (let stage of stages) {
            if (!isPlayableChartLike(stage)) {
                return false;
            }
        }
        return true;
    }

    function isPlayableBar(item: var) : var {
        return isPlayableChartLike(item) || isPlayableCourse(item);
    }

    function isLevel(item: var) : var {
        return item instanceof level;
    }

    function isTable(item: var) : var {
        return item instanceof table;
    }

    function isLoadedTable(item: var) : var {
        return isTable(item) && item.status === table.Loaded;
    }

    function isRankingEntry(item: var) : var {
        return !!item && item.__lr2RankingEntry === true;
    }

    function classCoursesForTable(tableItem: var) : var {
        let result = [];
        let courseLists = tableItem && tableItem.courses ? [...tableItem.courses] : [];
        for (let i = 0; i < courseLists.length; ++i) {
            let courseList = courseLists[i];
            if (isCourse(courseList)) {
                result.push(courseList);
                continue;
            }
            let courses = courseList ? [...courseList] : [];
            for (let j = 0; j < courses.length; ++j) {
                let courseItem = courses[j];
                if (isCourse(courseItem)) {
                    result.push(courseItem);
                }
            }
        }
        return result;
    }

    function isFolderLikeForLamp(item) {
        return typeof item === "string" || isTable(item) || isLevel(item);
    }

    function folderLampKey(item) {
        if (typeof item === "string") {
            return "folder:" + item;
        }
        if (isTable(item)) {
            return "table:" + (item.url || item.name || "");
        }
        if (isLevel(item)) {
            let parent = historyStack.length > 0 ? historyStack[historyStack.length - 1] : null;
            let parentKey = isTable(parent) ? (parent.url || parent.name || "") : "";
            return "level:" + parentKey + ":" + (item.name || "");
        }
        return "";
    }

    function normalizedNumberArray(values: var, fallback: var, wanted: var) : var {
        let source = values || fallback || [];
        let count = wanted > 0 ? wanted : source.length;
        let result = [];
        for (let i = 0; i < count; ++i) {
            result.push(Number(source[i] || 0));
        }
        return result;
    }

    function lr2CompatibleFolderDistribution(distribution: var) : var {
        if (root.useBeatorajaSelectOptions) {
            return distribution;
        }
        let lamps = distribution.lamps.slice();
        lamps[1] = (lamps[1] || 0) + (lamps[2] || 0) + (lamps[3] || 0);
        lamps[2] = 0;
        lamps[3] = 0;
        lamps[6] = (lamps[6] || 0) + (lamps[7] || 0);
        lamps[7] = 0;
        return {
            lamps: lamps,
            ranks: distribution.ranks
        };
    }

    function normalizedFolderDistribution(distribution: var) : var {
        let lamps = distribution
            ? (distribution.lamps !== undefined ? distribution.lamps : distribution["lamps"])
            : null;
        let ranks = distribution
            ? (distribution.ranks !== undefined ? distribution.ranks : distribution["ranks"])
            : null;
        return lr2CompatibleFolderDistribution({
            lamps: normalizedNumberArray(lamps, emptyFolderGraphLamps, 11),
            ranks: normalizedNumberArray(ranks, emptyFolderGraphRanks, 28)
        });
    }

    function folderScoreCountsFromSummaryCounts(counts) {
        if (!counts) {
            return emptyFolderScoreCounts;
        }
        if (counts.NOPLAY === undefined
                && counts.FAILED === undefined
                && counts.AEASY === undefined
                && counts.LIGHTASSIST === undefined
                && counts.LIGHT_ASSIST === undefined) {
            return counts;
        }

        let noplay = counts.NOPLAY || 0;
        let fail = counts.FAILED || 0;
        let assist = counts.AEASY || 0;
        let lightAssist = counts.LIGHTASSIST || counts.LIGHT_ASSIST || 0;
        let easy = counts.EASY || 0;
        let normal = counts.NORMAL || 0;
        let hard = counts.HARD || 0;
        let exhard = counts.EXHARD || 0;
        let fc = counts.FC || 0;
        let perfect = counts.PERFECT || 0;
        let max = counts.MAX || 0;
        if (!root.useBeatorajaSelectOptions) {
            fail += assist + lightAssist;
            assist = 0;
            lightAssist = 0;
            hard += exhard;
            exhard = 0;
        }
        let play = fail + assist + lightAssist + easy + normal + hard + exhard + fc + perfect + max;
        let clear = assist + lightAssist + easy + normal + hard + exhard + fc + perfect + max;
        return {
            total: noplay + play,
            play: play,
            clear: clear,
            fail: fail,
            noplay: noplay,
            assist: assist,
            lightAssist: lightAssist,
            easy: easy,
            normal: normal,
            hard: hard,
            exhard: exhard,
            fc: fc,
            perfect: perfect,
            max: max
        };
    }

    function lr2CompatibleFolderLamp(lamp: var, counts: var) : var {
        let value = Math.max(0, lamp || 0);
        if (value === 0 || !counts) {
            return value;
        }
        let assist = (counts.AEASY || 0)
            + (counts.LIGHTASSIST || 0)
            + (counts.LIGHT_ASSIST || 0);
        return assist > 0 ? 1 : value;
    }

    function exactFolderLampFromDistribution(distribution: var) : var {
        let lamps = distribution
            ? (distribution.lamps !== undefined ? distribution.lamps : distribution["lamps"])
            : null;
        if (!lamps) {
            return -1;
        }
        let limit = Math.min(11, lamps.length || 0);
        for (let i = 0; i < limit; ++i) {
            if (Number(lamps[i] || 0) > 0) {
                return i;
            }
        }
        return 0;
    }

    function exactFolderLampFromCounts(counts: var) : var {
        if (!counts) {
            return -1;
        }
        let order = [
            "NOPLAY", "FAILED", "AEASY", "LIGHTASSIST", "EASY", "NORMAL",
            "HARD", "EXHARD", "FC", "PERFECT", "MAX"
        ];
        for (let i = 0; i < order.length; ++i) {
            let key = order[i];
            let value = Number(counts[key] || 0);
            if (key === "LIGHTASSIST") {
                value += Number(counts.LIGHT_ASSIST || 0);
            }
            if (value > 0) {
                return i;
            }
        }
        return 0;
    }

    function clearTypeForExactFolderLamp(lamp: var) : var {
        switch (Math.max(0, Math.floor(lamp || 0))) {
        case 1:
            return "FAILED";
        case 2:
            return "AEASY";
        case 3:
            return "LIGHTASSIST";
        case 4:
            return "EASY";
        case 5:
            return "NORMAL";
        case 6:
            return "HARD";
        case 7:
            return "EXHARD";
        case 8:
            return "FC";
        case 9:
            return "PERFECT";
        case 10:
            return "MAX";
        default:
            return "NOPLAY";
        }
    }

    function beatorajaFolderLamp(lamp: var, counts: var, distribution: var) : var {
        let exactLamp = exactFolderLampFromDistribution(distribution);
        if (exactLamp < 0) {
            exactLamp = exactFolderLampFromCounts(counts);
        }
        if (exactLamp >= 0) {
            return clearTypeBarLamp(clearTypeForExactFolderLamp(exactLamp),
                                    root.barLampVariants);
        }
        return Math.max(0, lamp || 0);
    }

    function folderBarLamp(lamp: var, counts: var, distribution: var) : var {
        return root.useBeatorajaSelectOptions
            ? beatorajaFolderLamp(lamp, counts, distribution)
            : lr2CompatibleFolderLamp(lamp, counts);
    }

    function refreshFolderLamps() {
        let db = Rg.profileList?.mainProfile?.scoreDb;
        if (!db) {
            clearFolderLampState();
            return;
        }

        let requestToken = ++folderLampRequestToken;
        clearFolderLampState();
        for (let item of folderContents) {
            if (!isFolderLikeForLamp(item)) {
                continue;
            }

            let key = folderLampKey(item);
            if (!key) {
                continue;
            }

            db.getScoreSummary(item).then((result) => {
                if (requestToken !== folderLampRequestToken) {
                    return;
                }
                queueFolderLampSummaryResult(key, result);
            });
        }
    }

    function clearFolderLampState() {
        pendingFolderLampByKey = ({});
        pendingFolderScoreCountsByKey = ({});
        pendingFolderDistributionByKey = ({});
        pendingFolderLampPublishCount = 0;
        folderLampPublishQueued = false;
        folderLampByKey = ({});
        folderScoreCountsByKey = ({});
        selectItemModel.clearFolderSummaries();
    }

    function queueFolderLampSummaryResult(key, result) {
        let summary = result || ({});
        let counts = summary.counts !== undefined ? summary.counts : summary["counts"];
        let distribution = summary.distribution !== undefined
            ? summary.distribution
            : summary["distribution"];
        let lamp = summary.lamp !== undefined ? summary.lamp : summary["lamp"];
        pendingFolderLampByKey[key] = folderBarLamp(lamp, counts, distribution);
        pendingFolderScoreCountsByKey[key] = folderScoreCountsFromSummaryCounts(counts);
        pendingFolderDistributionByKey[key] = normalizedFolderDistribution(distribution);
        pendingFolderLampPublishCount += 1;
        if (updatesActive && !folderLampPublishQueued) {
            folderLampPublishQueued = true;
            Qt.callLater(() => {
                folderLampPublishQueued = false;
                if (updatesActive) {
                    publishPendingFolderLamps();
                }
            });
        }
    }

    function publishPendingFolderLamps() {
        if (pendingFolderLampPublishCount <= 0) {
            return;
        }

        folderLampByKey = Object.assign({}, folderLampByKey, pendingFolderLampByKey);
        folderScoreCountsByKey = Object.assign({}, folderScoreCountsByKey, pendingFolderScoreCountsByKey);
        for (let key in pendingFolderLampByKey) {
            selectItemModel.setFolderSummary(key,
                                             pendingFolderLampByKey[key] || 0,
                                             pendingFolderScoreCountsByKey[key],
                                             pendingFolderDistributionByKey[key]);
        }
        pendingFolderLampByKey = ({});
        pendingFolderScoreCountsByKey = ({});
        pendingFolderDistributionByKey = ({});
        pendingFolderLampPublishCount = 0;
    }

    function queueSelectItemScoreSummaries() : void {
        if (selectItemScoreSummaryPublishQueued) {
            return;
        }
        selectItemScoreSummaryPublishQueued = true;
        Qt.callLater(() => {
            selectItemScoreSummaryPublishQueued = false;
            publishSelectItemScoreSummaries();
        });
    }

    function publishSelectItemScoreSummaries() : void {
        selectItemModel.clearScoreSummaries();
        for (let item of folderContents) {
            if (!isChart(item) && !isEntry(item) && !isCourse(item)) {
                continue;
            }

            let id = entryIdentifier(item);
            if (!id) {
                continue;
            }

            let summary = scoreSummaryForItem(item);
            let lamp = clearTypeBarLamp(summary.clearType, root.barLampVariants);
            selectItemModel.setScoreSummary(String(id), lamp, summary.rank, summary.scoreRate);
        }
    }

    Component.onDestruction: {
        Rg.profileList?.mainProfile?.scoreDb?.cancelPending();
    }

    function addToMinimumCount(input: var) : var {
        if (input.length === 0) {
            input.push(null);
            return;
        }
        let wanted = 32;
        let length = input.length;
        if (length >= wanted) {
            return;
        }
        let limit = Math.ceil(wanted / length) * length;
        for (let i = length; i < limit; i++) {
            input.push(input[i % length] || null);
        }
    }

    function keyFilterMatches(item: var) : var {
        if (!isChart(item) || selectKeymodeFilter === SelectKeymodeFilter.All) {
            return true;
        }
        let keymode = item.keymode || 0;
        switch (selectKeymodeFilter) {
        case SelectKeymodeFilter.Single:
            return keymode === 5 || keymode === 7;
        case SelectKeymodeFilter.Double:
            return keymode === 10 || keymode === 14;
        case SelectKeymodeFilter.K5:
            return keymode === 5;
        case SelectKeymodeFilter.K7:
            return keymode === 7;
        case SelectKeymodeFilter.K10:
            return keymode === 10;
        case SelectKeymodeFilter.K14:
            return keymode === 14;
        default:
            return true;
        }
    }

    function chartFilterMatches(item: var) : var {
        return keyFilterMatches(item);
    }

    function difficultyFilteredCharts(input: var) : var {
        if (difficultyFilter === 0) {
            return input;
        }

        let groupOrder = [];
        let groups = {};
        let passthrough = [];
        for (let item of input) {
            if (!isChart(item)) {
                passthrough.push(item);
                continue;
            }

            let group = root.ensureChartGroup(groups, item);
            if (group.length === 0) {
                groupOrder.push({
                    folderKey: root.chartFolderKey(item),
                    keymodeKey: root.chartKeymodeKey(item)
                });
            }
            group.push(item);
        }

        let result = passthrough.slice();
        for (let groupRef of groupOrder) {
            let group = groups[groupRef.folderKey][groupRef.keymodeKey];
            let exact = [];
            let lower = [];
            let higher = [];
            let unknown = [];
            let lowerDifficulty = 0;
            let higherDifficulty = 0;
            for (let chart of group) {
                let difficulty = entryDifficulty(chart);
                if (difficulty === difficultyFilter) {
                    exact.push(chart);
                } else if (difficulty > 0 && difficulty < difficultyFilter) {
                    if (difficulty > lowerDifficulty) {
                        lower = [chart];
                        lowerDifficulty = difficulty;
                    } else if (difficulty === lowerDifficulty) {
                        lower.push(chart);
                    }
                } else if (difficulty > difficultyFilter) {
                    if (higherDifficulty === 0 || difficulty < higherDifficulty) {
                        higher = [chart];
                        higherDifficulty = difficulty;
                    } else if (difficulty === higherDifficulty) {
                        higher.push(chart);
                    }
                } else {
                    unknown.push(chart);
                }
            }
            if (exact.length > 0) {
                result.push(...exact);
            } else if (lower.length > 0) {
                result.push(...lower);
            } else if (higher.length > 0) {
                result.push(...higher);
            } else {
                result.push(...unknown);
            }
        }
        return result;
    }

    function compareByTitle(a: var, b: var) : var {
        let titleDiff = compareByNameOnly(a, b);
        if (titleDiff !== 0) {
            return titleDiff;
        }
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return 0;
        }
        return compareByDifficulty(a, b);
    }

    function compareByArtist(a: var, b: var) : var {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let artistDiff = entryArtist(a).localeCompare(entryArtist(b));
        if (artistDiff !== 0) {
            return artistDiff;
        }
        return compareByTitle(a, b);
    }

    function compareNumberWithMissing(aValue: var, bValue: var) : var {
        let aMissing = aValue === null || aValue === undefined || !isFinite(aValue);
        let bMissing = bValue === null || bValue === undefined || !isFinite(bValue);
        if (aMissing && bMissing) {
            return 0;
        }
        if (aMissing) {
            return 1;
        }
        if (bMissing) {
            return -1;
        }
        return aValue - bValue;
    }

    function entryMaxBpmForSort(item: var) : var {
        if (!isChart(item) && !isEntry(item)) {
            return null;
        }
        let bpm = item.maxBpm || item.mainBpm || item.minBpm || 0;
        return bpm > 0 ? bpm : null;
    }

    function entryLengthForSort(item: var) : var {
        if (!isChart(item) && !isEntry(item)) {
            return null;
        }
        return (item.length || 0) > 0 ? item.length : null;
    }

    function entryScoreRateForSort(item: var) : var {
        let summary = scoreSummaryForItem(item);
        return summary.bestScore ? summary.scoreRate : null;
    }

    function entryMissCountForSort(item: var) : var {
        let summary = scoreSummaryForItem(item);
        if (!summary.bestScore && summary.scoreCounts.play <= 0) {
            return null;
        }
        return summary.scoreCounts.minBadPoor >= 0 ? summary.scoreCounts.minBadPoor : null;
    }

    function entryTotalNotesForSort(item: var) : var {
        if (!isChart(item) && !isEntry(item)) {
            return null;
        }
        return (item.total || 0) > 0 ? item.total : null;
    }

    function compareByBpm(a: var, b: var) : var {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryMaxBpmForSort(a), entryMaxBpmForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByTitle(a, b);
    }

    function compareByLength(a: var, b: var) : var {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryLengthForSort(a), entryLengthForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByTitle(a, b);
    }

    function compareByDifficulty(a: var, b: var) : var {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let levelDiff = entryPlayLevel(a) - entryPlayLevel(b);
        if (levelDiff !== 0) {
            return levelDiff;
        }
        return compareByNameOnly(a, b);
    }

    function compareByClear(a: var, b: var) : var {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let aScores = entryScores(a);
        let bScores = entryScores(b);
        let aMissing = !aScores || aScores.length === 0;
        let bMissing = !bScores || bScores.length === 0;
        if (aMissing !== bMissing) {
            return aMissing ? 1 : -1;
        }
        let lampDiff = entryLamp(a) - entryLamp(b);
        if (lampDiff !== 0) {
            return lampDiff;
        }
        return compareByDifficulty(a, b);
    }

    function compareByScore(a: var, b: var) : var {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryScoreRateForSort(a), entryScoreRateForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByDifficulty(a, b);
    }

    function compareByMissCount(a: var, b: var) : var {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryMissCountForSort(a), entryMissCountForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByDifficulty(a, b);
    }

    function compareByTotalNotes(a: var, b: var) : var {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryTotalNotesForSort(a), entryTotalNotesForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByTitle(a, b);
    }

    function isSongLikeForSort(item: var) : var {
        return isRankingEntry(item) || isChart(item) || isEntry(item);
    }

    function compareByNameOnly(a: var, b: var) : var {
        return entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
    }

    function legacySortModeForSelectSortMode(mode: var) : var {
        switch (mode) {
        case SelectSortMode.Directory:
            return 0;
        case SelectSortMode.Level:
            return 1;
        case SelectSortMode.Title:
            return 2;
        case SelectSortMode.ClearLamp:
            return 3;
        case SelectSortMode.ScoreRate:
            return 4;
        case SelectSortMode.Artist:
            return 5;
        case SelectSortMode.Bpm:
            return 6;
        case SelectSortMode.Length:
            return 7;
        case SelectSortMode.MissCount:
            return 8;
        case SelectSortMode.TotalNotes:
            return 9;
        default:
            return 2;
        }
    }

    function selectSortModeForLegacySortMode(mode: var) : var {
        switch (mode) {
        case 0:
            return SelectSortMode.Directory;
        case 1:
            return SelectSortMode.Level;
        case 2:
            return SelectSortMode.Title;
        case 3:
            return SelectSortMode.ClearLamp;
        case 4:
            return SelectSortMode.ScoreRate;
        case 5:
            return SelectSortMode.Artist;
        case 6:
            return SelectSortMode.Bpm;
        case 7:
            return SelectSortMode.Length;
        case 8:
            return SelectSortMode.MissCount;
        case 9:
            return SelectSortMode.TotalNotes;
        default:
            return SelectSortMode.Title;
        }
    }

    function setSelectSortMode(mode: var) : void {
        if (generalVars) {
            generalVars.selectSortMode = mode;
        } else {
            localSelectSortMode = mode;
        }
    }

    function activeSortMode() : var {
        return legacySortModeForSelectSortMode(selectSortMode);
    }

    function compareCharts(a: var, b: var) : var {
        switch (activeSortMode()) {
        case 1:
            return compareByDifficulty(a, b);
        case 2:
            return compareByTitle(a, b);
        case 3:
            return compareByClear(a, b);
        case 4:
            return compareByScore(a, b);
        case 5:
            return compareByArtist(a, b);
        case 6:
            return compareByBpm(a, b);
        case 7:
            return compareByLength(a, b);
        case 8:
            return compareByMissCount(a, b);
        case 9:
            return compareByTotalNotes(a, b);
        default:
            return 0;
        }
    }

    function activeSortUsesScores() : var {
        switch (activeSortMode()) {
        case 3: // clear lamp
        case 4: // score rate
        case 8: // miss count
            return true;
        default:
            return false;
        }
    }

    function scoreLoadPreferredItem() : var {
        if (currentIndex !== 0
                || targetIndex !== 0
                || selectedOffset !== 0
                || visualMoveActive) {
            return focusedItem;
        }
        return null;
    }

    function handleScoresLoaded() : void {
        scoreGeneration += 1;
        clearScoreLoadedStateCaches();
        nativeNavigation.invalidateFocusedState();
        nativeNavigation.refreshFocusedState();
        if (activeSortUsesScores()) {
            sortOrFilterChanged(scoreLoadPreferredItem());
        }
        queueSelectItemScoreSummaries();
    }

    function sortOrderForSourceCount(sourceCount: var) : var {
        return root.useBeatorajaSelectOptions ? beatorajaSortOrder : lr2SortOrder;
    }

    function beatorajaKeyFilterOrderActive() : var {
        return root.useBeatorajaSelectOptions;
    }

    function keyFilterOrder(sourceCount: var) : var {
        let order = beatorajaKeyFilterOrderActive()
            ? beatorajaKeyFilterOrder
            : lr2KeyFilterOrder;
        let maxFrame = Math.floor(Number(sourceCount || 0)) - 1;
        if (maxFrame < 0) {
            return order;
        }

        let result = [];
        for (let frame of order) {
            if (frame <= maxFrame) {
                result.push(frame);
            }
        }
        return result.length > 0 ? result : [0];
    }

    function keyFilterFrameForSelectKeymodeFilter(filter: var, beatorajaOrder: var) : var {
        if (beatorajaOrder) {
            switch (filter) {
            case SelectKeymodeFilter.All: return 0;
            case SelectKeymodeFilter.K5: return 1;
            case SelectKeymodeFilter.K7: return 2;
            case SelectKeymodeFilter.K10: return 3;
            case SelectKeymodeFilter.K14: return 4;
            default: return -1;
            }
        }
        switch (filter) {
        case SelectKeymodeFilter.All: return 0;
        case SelectKeymodeFilter.Single: return 1;
        case SelectKeymodeFilter.K7: return 2;
        case SelectKeymodeFilter.K5: return 3;
        case SelectKeymodeFilter.Double: return 4;
        case SelectKeymodeFilter.K14: return 5;
        case SelectKeymodeFilter.K10: return 6;
        default: return -1;
        }
    }

    function selectKeymodeFilterForKeyFilterFrame(frame: var, beatorajaOrder: var) : var {
        if (beatorajaOrder) {
            switch (frame) {
            case 1: return SelectKeymodeFilter.K5;
            case 2: return SelectKeymodeFilter.K7;
            case 3: return SelectKeymodeFilter.K10;
            case 4: return SelectKeymodeFilter.K14;
            default: return SelectKeymodeFilter.All;
            }
        }
        switch (frame) {
        case 1: return SelectKeymodeFilter.Single;
        case 2: return SelectKeymodeFilter.K7;
        case 3: return SelectKeymodeFilter.K5;
        case 4: return SelectKeymodeFilter.Double;
        case 5: return SelectKeymodeFilter.K14;
        case 6: return SelectKeymodeFilter.K10;
        default: return SelectKeymodeFilter.All;
        }
    }

    function setSelectKeymodeFilter(filter: var) : void {
        if (generalVars) {
            generalVars.selectKeymodeFilter = filter;
        } else {
            localSelectKeymodeFilter = filter;
        }
    }

    function keyFilterFrameForSourceCount(sourceCount: var) : var {
        let order = keyFilterOrder(sourceCount);
        let frame = keyFilterFrameForSelectKeymodeFilter(selectKeymodeFilter, beatorajaKeyFilterOrderActive());
        return order.indexOf(frame) >= 0 ? frame : -1;
    }

    function adjustKeyFilter(delta: var, sourceCount: var) : void {
        let order = keyFilterOrder(sourceCount);
        let currentFrame = keyFilterFrameForSelectKeymodeFilter(selectKeymodeFilter, beatorajaKeyFilterOrderActive());
        let index = order.indexOf(currentFrame);
        if (index < 0) {
            index = delta < 0 ? order.length - 1 : 0;
        } else {
            index = ((index + delta) % order.length + order.length) % order.length;
        }
        setSelectKeymodeFilter(
            selectKeymodeFilterForKeyFilterFrame(order[index], beatorajaKeyFilterOrderActive()));
    }

    function resetSortSourceFrameCount() : void {
        setSortSourceFrameCount(root.useBeatorajaSelectOptions ? 8 : 5);
    }

    function setSortSourceFrameCount(sourceCount: var) : var {
        let normalized = root.useBeatorajaSelectOptions ? 8 : 5;
        if (sortSourceFrameCount === normalized) {
            return;
        }
        sortSourceFrameCount = normalized;
        if (folderContents.length > 0) {
            sortOrFilterChanged();
        }
    }

    function observeSortSourceFrameCount(sourceCount: var) : void {
        setSortSourceFrameCount(sourceCount);
    }

    function sortFrameForSourceCount(sourceCount: var) : var {
        let order = sortOrderForSourceCount(sourceCount);
        let frame = order.indexOf(activeSortMode());
        if (frame >= 0) {
            return frame;
        }
        return -1;
    }

    function adjustSortMode(delta: var, sourceCount: var) : void {
        setSortSourceFrameCount(sourceCount);
        let order = sortOrderForSourceCount(sourceCount);
        let frame = order.indexOf(activeSortMode());
        if (frame < 0) {
            frame = delta < 0 ? order.length - 1 : 0;
        } else {
            frame = ((frame + delta) % order.length + order.length) % order.length;
        }
        setSelectSortMode(selectSortModeForLegacySortMode(order[frame]));
    }

    function selectEntrySortBucket(item: var) : var {
        if (typeof item === "string") {
            return 0;
        }
        if (isTable(item) || isLevel(item)) {
            return 1;
        }
        if (isCourse(item)) {
            return 2;
        }
        if (isRankingEntry(item)) {
            return 3;
        }
        if (isChart(item) || isEntry(item)) {
            return 4;
        }
        return 5;
    }

    function sortEntryBucket(entries: var) : var {
        if (activeSortMode() === 0 || entries.length <= 1) {
            return entries;
        }
        let result = entries.slice();
        result.sort(compareCharts);
        return result;
    }

    function sortFilter(input: var) : var {
        let buckets = [[], [], [], [], [], []];
        for (let item of input) {
            if (isChart(item) || isEntry(item)) {
                if (!chartFilterMatches(item)) {
                    continue;
                }
            }
            buckets[selectEntrySortBucket(item)].push(item);
        }

        buckets[4] = difficultyFilteredCharts(buckets[4]);

        let result = [];
        for (let i = 0; i < buckets.length; ++i) {
            result.push(...sortEntryBucket(buckets[i]));
        }

        if (result.length === 0) {
            result.push(null);
        }
        return result;
    }

    function sortOrFilterChanged(preferredItem: var) : var {
        if (rankingMode) {
            hideRanking();
        }
        if (!folderContents.length) {
            return;
        }
        let old = preferredItem === undefined ? focusedItem : preferredItem;
        let sortedFiltered = sortFilter(folderContents);
        realItemCount = sortedFiltered.length;
        addToMinimumCount(sortedFiltered);
        items = sortedFiltered;
        let currentIdx = sortedFiltered.findIndex((item) => sameEntry(item, old));
        currentIndex = currentIdx >= 0 ? currentIdx : 0;
        targetIndex = currentIndex;
        selectedOffset = 0;
        lastSyncedCursorBaseIndex = -1;
        setVisualIndexImmediate(currentIndex);
        markListContentsChanged();
        nativeNavigation.touchSelection();
    }

    function refreshPreviewFiles() : void {
        let dirs = [];
        for (let item of folderContents) {
            if (isChart(item)) {
                dirs.push(item.chartDirectory);
            }
        }
        previewFiles = Rg.previewFilePathFetcher.getPreviewFilePaths(dirs);
    }

    function scanAttachedTextFileForDirectory(dir: var) : var {
        if (!dir) {
            return "";
        }

        let files = Rg.fileQuery.getSelectableFilesForDirectory(dir);
        for (let file of files) {
            if (String(file).toLowerCase().endsWith(".txt")) {
                let separator = dir.endsWith("/") || dir.endsWith("\\") ? "" : "/";
                return dir + separator + file;
            }
        }
        return "";
    }

    function refreshAttachedTextIndex(input: var) : void {
        let byDirectory = {};
        let hasTextByDirectory = {};
        let seen = {};
        for (let item of input || []) {
            if (!item || !item.chartDirectory || seen[item.chartDirectory]) {
                continue;
            }
            seen[item.chartDirectory] = true;
            let file = scanAttachedTextFileForDirectory(item.chartDirectory);
            byDirectory[item.chartDirectory] = file;
            hasTextByDirectory[item.chartDirectory] = file.length > 0;
        }
        attachedTextByDirectory = byDirectory;
        hasAttachedTextByDirectory = hasTextByDirectory;
    }

    function folderContentsNeedFullScores() : var {
        for (let item of folderContents) {
            if (isChart(item) || isEntry(item) || isCourse(item) || isRankingEntry(item)) {
                return true;
            }
        }
        return false;
    }

    function refreshScores() : var {
        let folder = historyStack.length > 0 ? historyStack[historyStack.length - 1] : "";
        let scoreDb = Rg.profileList.mainProfile.scoreDb;
        scoreDb.cancelPending();
        if (!folderContentsNeedFullScores()) {
            scores = ({});
            handleScoresLoaded();
            refreshPreviewFiles();
            refreshFolderLamps();
            return;
        }
        if (folder === "SEARCH") {
            let md5s = [];
            for (let item of folderContents) {
                if (isChart(item) && item.md5) {
                    md5s.push(item.md5);
                }
            }
            scoreDb.getScoresForMd5(md5s).then((result) => {
                scores = result.scores;
                handleScoresLoaded();
            });
            refreshPreviewFiles();
            refreshFolderLamps();
            return;
        }
        scoreDb.getScores(folder).then((result) => {
            if (result && result.courseScores !== undefined) {
                let newScores = result.scores.scores;
                for (let [key, value] of Object.entries(result.courseScores.scores)) {
                    newScores[key] = value;
                }
                scores = newScores;
            } else {
                scores = result.scores;
            }
            handleScoresLoaded();
        });
        refreshPreviewFiles();
        refreshFolderLamps();
    }

    function refreshPlayerStats() : var {
        let db = Rg.profileList?.mainProfile?.scoreDb;
        if (!db) {
            return;
        }
        db.getTotalStats().then((result) => {
            playerStats = result || playerStats;
        });
    }

    function open(item: var, initialItem: var) : var {
        let folder;
        if (isTable(item)) {
            folder = [...item.levels, ...classCoursesForTable(item)];
        } else if (isLevel(item)) {
            folder = item.loadCharts();
        } else if (typeof item === "string") {
            folder = [];
            if (item === "") {
                let tables = Rg.tables.getList();
                for (let t of tables) {
                    if (isLoadedTable(t)) {
                        folder.push(t);
                    }
                }
            }
            let songFolders = Rg.songFolderFactory.open(item);
            folder.push(...songFolders);
        } else {
            return [];
        }

        folderContents = [...folder];
        rebuildFolderIndexes(folderContents);
        refreshAttachedTextIndex(folderContents);
        folder = sortFilter(folder);
        realItemCount = folder.length;
        addToMinimumCount(folder);
        items = folder;
        let initialIndex = initialItem === undefined
            ? 0
            : folder.findIndex((folderItem) => sameEntry(folderItem, initialItem));
        currentIndex = initialIndex >= 0 ? initialIndex : 0;
        targetIndex = currentIndex;
        selectedOffset = 0;
        lastSyncedCursorBaseIndex = -1;
        setVisualIndexImmediate(currentIndex);
        refreshScores();
        refreshPlayerStats();
        openedFolder();
        markListContentsChanged();
        nativeNavigation.touchSelection();
        return folder;
    }

    function search(query: var) : var {
        let results = Rg.songFolderFactory.search(query || "");
        if (!results.length) {
            console.info("Search returned no results");
            return;
        }
        folderContents = [...results];
        rebuildFolderIndexes(folderContents);
        refreshAttachedTextIndex(folderContents);
        results = sortFilter(results);
        realItemCount = results.length;
        addToMinimumCount(results);
        if (historyStack[historyStack.length - 1] !== "SEARCH") {
            historyStack = historyStack.concat(["SEARCH"]);
        }
        items = results;
        currentIndex = 0;
        targetIndex = 0;
        selectedOffset = 0;
        lastSyncedCursorBaseIndex = -1;
        setVisualIndexImmediate(0);
        refreshScores();
        refreshPlayerStats();
        openedFolder();
        markListContentsChanged();
        nativeNavigation.touchSelection();
    }

    function openRoot() : void {
        hideRanking();
        historyStack = [""];
        open("");
    }

    function normalizeIndex(index: var) : var {
        if (logicalCount === 0) {
            return 0;
        }
        return ((index % logicalCount) + logicalCount) % logicalCount;
    }

    function setCurrentIndex(index: var) : var {
        if (logicalCount === 0) {
            currentIndex = 0;
            targetIndex = 0;
            selectedOffset = 0;
            lastSyncedCursorBaseIndex = -1;
            setVisualIndexImmediate(0);
            nativeNavigation.touchSelection();
            return;
        }
        let normalized = normalizeIndex(index);
        if (currentIndex === normalized && selectedOffset === 0) {
            return;
        }
        let now = Date.now();
        updateVisualIndex(now);
        selectedOffset = 0;
        lastSyncedCursorBaseIndex = -1;
        let targetVisual = nearestVisualIndex(normalized, listTopbarFixed / 1000.0);
        oldBarFixed = listTopbarFixed;
        nowBarFixed = Math.round(targetVisual * 1000);
        scrollDirection = nowBarFixed < listTopbarFixed ? lr2ScrollUp : lr2ScrollDown;
        targetIndex = normalized;
        beginVisualMove(lr2ClickDuration, now);
        nativeNavigation.commitLogicalSelection(normalized);
    }

    function setScrollIndexImmediate(index: var) : var {
        if (logicalCount === 0) {
            return;
        }
        let normalized = normalizeIndex(Math.round(index));
        selectedOffset = 0;
        lastSyncedCursorBaseIndex = -1;
        currentIndex = normalized;
        targetIndex = normalized;
        setVisualIndexImmediate(normalized);
        nativeNavigation.touchSelection();
    }

    function beginScrollFixedPointDrag() : void {
        scrollDirection = 0;
        scrollFixedPointDragging = true;
    }

    function setBarFixedImmediate(fixedValue: var) : var {
        let clamped = clampBarFixed(fixedValue);
        oldBarFixed = clamped;
        nowBarFixed = clamped;
        setVisualIndexRaw(clamped / 1000.0);
        barMoveStartMs = 0;
        barMoveEndMs = 0;
        return nativeNavigation.publishCursorBaseIndex();
    }

    function dragScrollFixedPoint(fixedValue: var) : var {
        if (logicalCount === 0) {
            return;
        }

        let clamped = clampBarFixed(fixedValue);
        scrollDirection = clamped < listCalculatedBarFixed ? lr2ScrollUp
            : (clamped > listCalculatedBarFixed ? lr2ScrollDown : scrollDirection);
        selectedOffset = 0;
        lastSyncedCursorBaseIndex = -1;
        setBarFixedImmediate(clamped);
    }

    function setScrollFixedPoint(fixedValue: var, durationMs: var, snapToEntry: var) : var {
        if (logicalCount === 0) {
            return;
        }

        let clamped = clampBarFixed(fixedValue);
        let rounded = Math.floor(clamped / 1000);
        if (clamped % 1000 > 499) {
            rounded += 1;
        }

        let now = Date.now();
        let shouldSnap = snapToEntry === undefined ? true : !!snapToEntry;
        let oldOffset = selectedOffset;
        selectedOffset = 0;
        lastSyncedCursorBaseIndex = -1;
        if (!shouldSnap) {
            setBarFixedImmediate(clamped);
            return;
        }
        setBarFixedImmediate(clamped);
        oldBarFixed = listTopbarFixed;
        nowBarFixed = Math.round(nearestVisualIndex(rounded, clamped / 1000.0) * 1000);
        scrollDirection = nowBarFixed < listTopbarFixed ? lr2ScrollUp
            : (nowBarFixed > listTopbarFixed ? lr2ScrollDown : scrollDirection);
        let nextIndex = normalizeIndex(rounded);
        targetIndex = nextIndex;
        let selectionTouched = beginVisualMove(durationMs !== undefined ? durationMs : 100, now);
        let focusTouched = nativeNavigation.commitLogicalSelection(nextIndex);
        if (!selectionTouched && !focusTouched && oldOffset !== selectedOffset) {
            nativeNavigation.touchSelection();
        }
    }

    function finishScrollFixedPoint(durationMs: var) : var {
        if (logicalCount === 0) {
            scrollFixedPointDragging = false;
            return;
        }

        let now = Date.now();
        let wasDragging = scrollFixedPointDragging;
        scrollFixedPointDragging = false;
        updateVisualIndex(now);

        let rounded = Math.floor(listCalculatedBarFixed / 1000);
        if (listCalculatedBarFixed % 1000 > 499) {
            rounded += 1;
        }
        let oldOffset = selectedOffset;
        selectedOffset = 0;
        lastSyncedCursorBaseIndex = -1;
        oldBarFixed = listTopbarFixed;
        nowBarFixed = Math.round(nearestVisualIndex(rounded, listTopbarFixed / 1000.0) * 1000);
        scrollDirection = nowBarFixed < listTopbarFixed ? lr2ScrollUp
            : (nowBarFixed > listTopbarFixed ? lr2ScrollDown : scrollDirection);
        let nextIndex = normalizeIndex(rounded);
        targetIndex = nextIndex;
        let selectionTouched = beginVisualMove(durationMs !== undefined ? durationMs : 100, now);
        let focusTouched = nativeNavigation.commitLogicalSelection(nextIndex);
        if (!selectionTouched && !focusTouched && (wasDragging || oldOffset !== selectedOffset)) {
            nativeNavigation.touchSelection();
        }
    }

    function scrollBy(entries: var, durationMs: var) : void {
        nativeNavigation.scrollBy(entries, durationMs === undefined ? -1 : durationMs);
    }

    function scrollByKey(entries: var, repeated: var) : void {
        nativeNavigation.scrollByKey(entries, !!repeated);
    }

    function selectVisibleRow(row: var, barCenter: var) : var {
        if (logicalCount === 0) {
            return;
        }
        selectedOffset = row - barCenter;
        lastSyncedCursorBaseIndex = -1;
        currentIndex = normalizeIndex(visualState.baseIndex + selectedOffset);
        targetIndex = currentIndex;
        nativeNavigation.touchSelection();
    }

    function activationItem() : var {
        if (logicalCount === 0) {
            return null;
        }
        return focusedItem;
    }

    function selectedRow(barCenter: var) : var {
        return barCenter + selectedOffset;
    }

    function visualSelectedRow(barCenter: var) : var {
        return selectedRow(barCenter);
    }

    function visualRowForIndex(index: var, barCenter: var) : var {
        if (logicalCount === 0) {
            return barCenter;
        }
        let offset = normalizeIndex(index) - normalizeIndex(visualState.baseIndex);
        if (offset > logicalCount / 2) {
            offset -= logicalCount;
        }
        if (offset < -logicalCount / 2) {
            offset += logicalCount;
        }
        return barCenter + offset;
    }

    function currentVisualRow(barCenter: var) : var {
        return visualRowForIndex(currentIndex, barCenter);
    }

    function decrementViewIndex(repeated: var) : void {
        nativeNavigation.decrementViewIndex(!!repeated);
    }

    function incrementViewIndex(repeated: var) : void {
        nativeNavigation.incrementViewIndex(!!repeated);
    }

    function goBack() : var {
        if (rankingMode) {
            hideRanking();
            return;
        }
        if (historyStack.length <= 1) {
            return;
        }
        let last = historyStack[historyStack.length - 1];
        historyStack = historyStack.slice(0, historyStack.length - 1);
        open(historyStack[historyStack.length - 1], last);
    }

    function goForward(item: var, autoplay: var, replay: var, replayScore: var) : var {
        if (isRankingEntry(item)) {
            item = rankingBaseItem;
        }
        if (isChart(item)) {
            let useReplay = !!replay && !!replayScore;
            if (Rg.profileList.battleActive) {
                globalRoot.openChart(item.path, Rg.profileList.battleProfiles.player1Profile, !!autoplay, useReplay, replayScore || null, Rg.profileList.battleProfiles.player2Profile, !!autoplay, false, null);
            } else {
                globalRoot.openChart(item.path, Rg.profileList.mainProfile, !!autoplay, useReplay, replayScore || null, null, false, false, null);
            }
            return;
        }
        if (isCourse(item)) {
            let useReplay = !!replay && !!replayScore;
            if (Rg.profileList.battleActive) {
                globalRoot.openCourse(item, Rg.profileList.battleProfiles.player1Profile, !!autoplay, useReplay, replayScore || null, Rg.profileList.battleProfiles.player2Profile, !!autoplay, false, null);
            } else {
                globalRoot.openCourse(item, Rg.profileList.mainProfile, !!autoplay, useReplay, replayScore || null, null, false, false, null);
            }
            return;
        }
        if (isEntry(item) || item === null || item === undefined) {
            return;
        }
        historyStack = historyStack.concat([item]);
        open(item);
    }

    function openReplayResult(item: var, replayScore: var) : var {
        if (!replayScore) {
            return false;
        }
        if (isRankingEntry(item)) {
            item = rankingBaseItem;
        }
        if (isCourse(item)) {
            globalRoot.openCourseResult(
                [replayScore],
                [Rg.profileList.mainProfile],
                courseStages(item),
                item);
            return true;
        }
        if (isChart(item)) {
            globalRoot.openResult([replayScore], [Rg.profileList.mainProfile], item);
            return true;
        }
        return false;
    }

    function sameEntry(a: var, b: var) : var {
        if (isRankingEntry(a) || isRankingEntry(b)) {
            return isRankingEntry(a) && isRankingEntry(b)
                && a.rankingIndex === b.rankingIndex
                && a.sourceMd5 === b.sourceMd5;
        }
        if (isChart(a) && isChart(b)) {
            return a.path === b.path;
        }
        if (typeof a === "string" && typeof b === "string") {
            return a === b;
        }
        if (isLevel(a) && isLevel(b)) {
            return a.name === b.name;
        }
        if (isTable(a) && isTable(b)) {
            return a.name === b.name;
        }
        if (isCourse(a) && isCourse(b)) {
            return a.name === b.name;
        }
        return false;
    }

    function entryDisplayName(item: var, includeSubtitle: var) : var {
        if (!item) {
            return "";
        }
        if (isRankingEntry(item)) {
            return item.title || "";
        }
        if (isChart(item) || isEntry(item)) {
            let title = item.title || "";
            let subtitle = item.subtitle || "";
            let prefix = isMissingTableEntry(item) && !root.useBeatorajaBarTextTypes ? "(missing) " : "";
            return prefix + (includeSubtitle && subtitle ? title + " " + subtitle : title);
        }
        if (isLevel(item)) {
            let parent = historyStack.length > 0 ? historyStack[historyStack.length - 1] : null;
            let symbol = isTable(parent) ? (parent.symbol || "") : "";
            return symbol + (item.name || "");
        }
        if (isCourse(item) || isTable(item)) {
            return item.name || "";
        }
        if (typeof item === "string") {
            let normalized = item.replace(/\\/g, "/").replace(/\/$/, "");
            let slash = normalized.lastIndexOf("/");
            return slash >= 0 ? normalized.slice(slash + 1) : normalized;
        }
        return "";
    }

    function entryMainTitle(item: var) : var {
        if (isRankingEntry(item)) {
            return item.title || "";
        }
        if (isChart(item) || isEntry(item)) {
            let prefix = isMissingTableEntry(item) && !root.useBeatorajaBarTextTypes ? "(missing) " : "";
            return prefix + (item.title || "");
        }
        return entryDisplayName(item, false);
    }

    function currentFolderDisplayName() : var {
        let item = historyStack.length > 0 ? historyStack[historyStack.length - 1] : null;
        if (!item || item === "SEARCH") {
            return "";
        }
        if (isLevel(item)) {
            let parent = historyStack.length > 1 ? historyStack[historyStack.length - 2] : null;
            let symbol = isTable(parent) ? (parent.symbol || "") : "";
            return symbol + (item.name || "");
        }
        return entryDisplayName(item, false);
    }

    function currentDirectoryBreadcrumb() : var {
        let parts = [];
        for (let item of historyStack) {
            if (!item || item === "SEARCH") {
                continue;
            }
            let name = entryDisplayName(item, false);
            if (name.length > 0) {
                parts.push(name);
            }
        }
        return parts.length > 0 ? parts.join(" > ") + " >" : "";
    }

    function currentTableItem() : var {
        for (let i = historyStack.length - 1; i >= 0; --i) {
            if (isTable(historyStack[i])) {
                return historyStack[i];
            }
        }
        let item = selectedStateItem;
        return isTable(item) ? item : null;
    }

    function currentTableName() : var {
        let tableItem = currentTableItem();
        return tableItem ? (tableItem.name || "") : "";
    }

    function currentTableLevelName() : var {
        let item = historyStack.length > 0 ? historyStack[historyStack.length - 1] : null;
        if (isLevel(item)) {
            return item.name || "";
        }
        let selected = selectedStateItem;
        return isLevel(selected) ? (selected.name || "") : "";
    }

    function currentTableFullName() : var {
        let tableName = currentTableName();
        let levelName = currentTableLevelName();
        if (tableName.length > 0 && levelName.length > 0) {
            return tableName + " " + levelName;
        }
        return tableName.length > 0 ? tableName : levelName;
    }

    function entrySubtitle(item: var) : var {
        return isChart(item) || isEntry(item) ? (item.subtitle || "") : "";
    }

    function entryGenre(item: var) : var {
        return isChart(item) ? (item.genre || "") : "";
    }

    function entryArtist(item: var) : var {
        return isChart(item) || isEntry(item) ? (item.artist || "") : "";
    }

    function entrySubartist(item: var) : var {
        return isChart(item) || isEntry(item) ? (item.subartist || "") : "";
    }

    function entryBodyType(item: var) : var {
        if (isRankingEntry(item)) {
            return 0;
        }
        if (isChart(item)) {
            return 0;
        }
        if (isEntry(item)) {
            return 4;
        }
        if (isTable(item) || isLevel(item)) {
            return 2;
        }
        if (isCourse(item)) {
            return isPlayableCourse(item) ? 3 : 4;
        }
        return 1;
    }

    function rankingEntryRank(entry: var) : var {
        let points = Number(entry.bestPoints || 0);
        let maxPoints = Number(entry.maxPoints || 0);
        if (maxPoints <= 0) {
            return 0;
        }

        let rank = Math.floor(points * 9 / maxPoints);
        if (rank > 7) {
            rank = 8;
        }
        if (rank < 2 && points > 0) {
            rank = 1;
        }
        return rank;
    }

    function barTitleTypeDefined(type: var) : var {
        let normalized = Math.floor(type || 0);
        let types = root.barTitleTypes || [];
        for (let value of types) {
            if (Math.floor(value || 0) === normalized) {
                return true;
            }
        }
        return false;
    }

    function barTitleTypeWithFallback(type: var, fallback: var) : var {
        let normalized = Math.max(0, Math.floor(type || 0));
        if (normalized <= 1 || root.barTitleTypeDefined(normalized)) {
            return normalized;
        }
        return Math.max(0, Math.floor(fallback || 0));
    }

    function beatorajaEntryTitleType(item: var) : var {
        if (isRankingEntry(item) || isChart(item)) {
            return root.barTitleTypeWithFallback(2, 0);
        }
        if (isEntry(item)) {
            return root.barTitleTypeWithFallback(8, 0);
        }
        if (typeof item === "string") {
            return root.barTitleTypeWithFallback(4, 0);
        }
        if (isTable(item) || isLevel(item)) {
            return root.barTitleTypeWithFallback(6, 0);
        }
        if (isCourse(item)) {
            return root.barTitleTypeWithFallback(isPlayableCourse(item) ? 7 : 8, 0);
        }
        return 0;
    }

    function entryTitleType(item: var) : var {
        if (root.useBeatorajaBarTextTypes) {
            return root.beatorajaEntryTitleType(item);
        }
        return 0;
    }

    function entryLabelMask(item: var) : var {
        if (!isChart(item)) {
            return 0;
        }

        let mask = 0;
        if (hasLongNote(item)) {
            mask |= 1 << barLabelLongNote;
        }
        if (item.isRandom) {
            mask |= 1 << barLabelRandom;
        }
        if ((item.mineCount || 0) > 0) {
            mask |= 1 << barLabelMine;
        }
        return mask;
    }

    function normalizedSelectModelItem(item: var, fallbackIndex: var) : var {
        let rankingEntry = isRankingEntry(item);
        let folderLikeForLamp = isFolderLikeForLamp(item);
        return {
            rawItem: item,
            folderKey: folderLikeForLamp ? folderLampKey(item) : "",
            displayText: entryDisplayName(item, true),
            titleType: entryTitleType(item),
            bodyType: entryBodyType(item),
            title: entryMainTitle(item),
            difficulty: entryDifficulty(item),
            lamp: rankingEntry ? clearTypeBarLamp(item.bestClearType, root.barLampVariants) : 0,
            scoreRank: rankingEntry ? rankingEntryRank(item) : 0,
            labelMask: entryLabelMask(item),
            __lr2RankingEntry: rankingEntry
        };
    }

    function normalizedSelectModelItems(sourceItems: var) : var {
        let result = [];
        let list = sourceItems || [];
        for (let i = 0; i < list.length; ++i) {
            result.push(normalizedSelectModelItem(list[i], i));
        }
        return result;
    }

    function keyFilterLabel() : var {
        switch (selectKeymodeFilter) {
        case SelectKeymodeFilter.Single: return "SINGLE";
        case SelectKeymodeFilter.Double: return "DOUBLE";
        case SelectKeymodeFilter.K5: return "5KEYS";
        case SelectKeymodeFilter.K7: return "7KEYS";
        case SelectKeymodeFilter.K10: return "10KEYS";
        case SelectKeymodeFilter.K14: return "14KEYS";
        default: return "ALL";
        }
    }

    function sortLabel() : var {
        switch (activeSortMode()) {
        case 1: return "LEVEL";
        case 2: return "TITLE";
        case 3: return "CLEAR LAMP";
        case 4: return "SCORE RATE";
        case 5: return "ARTIST";
        case 6: return "BPM";
        case 7: return "LENGTH";
        case 8: return "MISS COUNT";
        case 9: return "TOTAL NOTES";
        default: return "DIRECTORY";
        }
    }

    function difficultyFilterLabel() : var {
        switch (difficultyFilter) {
        case 1: return "EASY";
        case 2: return "STANDARD";
        case 3: return "HARD";
        case 4: return "EXPERT";
        case 5: return "ULTIMATE";
        default: return "ALL";
        }
    }

    function entryPlayLevel(item: var) : var {
        if (isRankingEntry(item)) {
            return item.level || 0;
        }
        if (isChart(item)) {
            return item.playLevel || 0;
        }
        if (isEntry(item)) {
            let parsed = parseInt(item.level);
            return isNaN(parsed) ? 0 : parsed;
        }
        return 0;
    }

    function rawChartDifficulty(item: var) : var {
        return isChart(item) ? (item.difficulty || 0) : 0;
    }

    function entryDifficulty(item: var) : var {
        if (!isChart(item)) {
            return 0;
        }
        let difficulty = chartDifficultyByPath[item.path || ""];
        if (difficulty !== undefined) {
            return difficulty;
        }
        return Math.max(0, rawChartDifficulty(item));
    }

    function chartNoteCount(item: var) : var {
        if (!isChart(item)) {
            return 0;
        }
        return (item.normalNoteCount || 0)
            + (item.scratchCount || 0)
            + (item.lnCount || 0)
            + (item.bssCount || 0)
            + (item.mineCount || 0);
    }

    function chartPlayableNoteCount(item: var) : var {
        if (!isChart(item)) {
            return 0;
        }
        return (item.normalNoteCount || 0)
            + (item.scratchCount || 0)
            + (item.lnCount || 0)
            + (item.bssCount || 0);
    }

    function chartLengthSeconds(item: var) : var {
        return isChart(item) ? Math.floor(Math.max(0, item.length || 0) / 1000000000) : -1;
    }

    function chartDensityValue(item: var, propertyName: var, afterDot: var) : var {
        if (!isChart(item)) {
            return -1;
        }
        let density = Math.max(0, item[propertyName] || 0);
        return afterDot ? Math.floor(density * 100) % 100 : Math.floor(density);
    }

    function chartDensityWhole(item: var) : var {
        return chartDensityValue(item, "avgDensity", false);
    }

    function chartDensityAfterDot(item: var) : var {
        return chartDensityValue(item, "avgDensity", true);
    }

    function entryIdentifier(item: var) : var {
        if (isRankingEntry(item)) {
            return "";
        }
        if (isChart(item) || isEntry(item)) {
            return item.md5 || "";
        }
        if (isCourse(item)) {
            return item.identifier || "";
        }
        return "";
    }

    function entrySelectionKey(item: var, fallbackIndex: var) : var {
        if (!item) {
            return "empty";
        }
        if (isRankingEntry(item)) {
            return "ranking:" + (item.sourceMd5 || "") + ":" + (item.rankingIndex || 0);
        }
        if (isChart(item)) {
            return "chart:" + (item.md5
                || item.sha256
                || item.path
                || ("index:" + fallbackIndex));
        }
        if (isEntry(item)) {
            return "entry:" + (item.md5
                || item.path
                || ("index:" + fallbackIndex));
        }
        if (isCourse(item)) {
            return "course:" + (item.identifier || item.name || ("index:" + fallbackIndex));
        }
        if (isTable(item)) {
            return "table:" + (item.url || item.name || ("index:" + fallbackIndex));
        }
        if (isLevel(item)) {
            return "level:" + (folderLampKey(item) || item.name || ("index:" + fallbackIndex));
        }
        if (typeof item === "string") {
            return "folder:" + item;
        }
        return "item:" + (entryDisplayName(item, true) || ("index:" + fallbackIndex));
    }

    function entryScoresForIdentifier(id: var) : var {
        if (!id) {
            return emptyScoreList;
        }
        let key = String(id);
        return scores[key]
            || scores[key.toUpperCase()]
            || scores[key.toLowerCase()]
            || emptyScoreList;
    }

    function entryScores(item: var) : var {
        return entryScoresForIdentifier(entryIdentifier(item));
    }

    function normalizedClearType(clear: var) : var {
        let value = String(clear || "NOPLAY").toUpperCase();
        switch (value) {
        case "":
            return "NOPLAY";
        case "ASSIST":
        case "ASSISTEASY":
        case "ASSIST_EASY":
            return "AEASY";
        case "LIGHT_ASSIST":
        case "LIGHTASSISTEASY":
        case "LIGHT_ASSIST_EASY":
            return "LIGHTASSIST";
        case "EX_HARD":
            return "EXHARD";
        case "EXHARD_DAN":
        case "EX_HARD_DAN":
            return "EXHARDDAN";
        case "FAILED":
        case "AEASY":
        case "LIGHTASSIST":
        case "EASY":
        case "NORMAL":
        case "HARD":
        case "EXHARD":
        case "EXHARDDAN":
        case "FC":
        case "PERFECT":
        case "MAX":
        case "NOPLAY":
            return value;
        default:
            return value;
        }
    }

    function skinCompatibleClearType(clear: var) : var {
        let value = normalizedClearType(clear);
        if (root.useBeatorajaSelectOptions) {
            return value;
        }
        switch (value) {
        case "AEASY":
        case "LIGHTASSIST":
            return "FAILED";
        case "EXHARD":
        case "EXHARDDAN":
            return "HARD";
        default:
            return value;
        }
    }

    function clearTypePriority(clear: var) : var {
        switch (skinCompatibleClearType(clear)) {
        case "FAILED":
            return 1;
        case "AEASY":
            return 2;
        case "LIGHTASSIST":
            return 3;
        case "EASY":
            return 4;
        case "NORMAL":
            return 5;
        case "HARD":
            return 6;
        case "EXHARD":
            return 7;
        case "FC":
            return 8;
        case "PERFECT":
            return 9;
        case "MAX":
            return 10;
        default:
            return 0;
        }
    }

    function rankForScoreRate(rate: var) : var {
        if (rate >= 1.0) return 9;
        if (rate >= 8 / 9) return 8;
        if (rate >= 7 / 9) return 7;
        if (rate >= 6 / 9) return 6;
        if (rate >= 5 / 9) return 5;
        if (rate >= 4 / 9) return 4;
        if (rate >= 3 / 9) return 3;
        if (rate >= 2 / 9) return 2;
        return rate > 0 ? 1 : 0;
    }

    function badPoorForScore(score: var) : var {
        let counts = score?.result?.judgementCounts || [];
        return judgementCount(counts, Judgement.Bad)
            + judgementCount(counts, Judgement.Poor)
            + judgementCount(counts, Judgement.EmptyPoor);
    }

    function buildScoreSummary(scoreList: var) : var {
        if (!scoreList || scoreList.length === 0) {
            return {
                scoreList: emptyScoreList,
                bestScore: null,
                bestStats: null,
                scoreCounts: emptyScoreCounts,
                clearType: "NOPLAY",
                lamp: 0,
                rank: 0,
                scoreRate: 0
            };
        }

        let counts = {
            play: 0,
            clear: 0,
            fail: 0,
            noplay: 0,
            assist: 0,
            lightAssist: 0,
            easy: 0,
            normal: 0,
            hard: 0,
            exhard: 0,
            fc: 0,
            perfect: 0,
            max: 0,
            minBadPoor: 0
        };
        let bestScore = null;
        let bestRate = -1;
        let bestClearType = "NOPLAY";
        let bestClearPriority = 0;
        let minBadPoor = -1;

        for (let score of scoreList) {
            if (!score || !score.result) {
                continue;
            }

            ++counts.play;
            let clearType = clearTypeOf(score);
            let clearPriority = clearTypePriority(clearType);
            if (clearPriority > bestClearPriority) {
                bestClearPriority = clearPriority;
                bestClearType = clearType;
            }

            if (clearType !== "FAILED" && clearType !== "NOPLAY") {
                ++counts.clear;
            }
            switch (clearType) {
            case "FAILED":
                ++counts.fail;
                break;
            case "AEASY":
                ++counts.assist;
                break;
            case "LIGHTASSIST":
                ++counts.lightAssist;
                break;
            case "EASY":
                ++counts.easy;
                break;
            case "NORMAL":
                ++counts.normal;
                break;
            case "HARD":
                ++counts.hard;
                break;
            case "EXHARD":
                ++counts.exhard;
                break;
            case "FC":
                ++counts.fc;
                break;
            case "PERFECT":
                ++counts.perfect;
                break;
            case "MAX":
                ++counts.max;
                break;
            default:
                ++counts.noplay;
                break;
            }

            let badPoor = badPoorForScore(score);
            minBadPoor = minBadPoor < 0 ? badPoor : Math.min(minBadPoor, badPoor);

            let maxPoints = score.result.maxPoints || 0;
            if (maxPoints > 0) {
                let rate = (score.result.points || 0) / maxPoints;
                if (rate > bestRate) {
                    bestRate = rate;
                    bestScore = score;
                }
            }
        }

        counts.minBadPoor = Math.max(0, minBadPoor);
        let scoreRate = Math.max(0, bestRate);
        return {
            scoreList: scoreList,
            bestScore: bestScore,
            bestStats: bestScore ? statsForScore(bestScore, false) : null,
            scoreCounts: counts,
            clearType: bestClearType,
            lamp: clearTypeLamp(bestClearType),
            rank: rankForScoreRate(scoreRate),
            scoreRate: scoreRate
        };
    }

    function scoreSummaryForItem(item: var) : var {
        let state = selectedState;
        if ((item === state.item || item === state.chartData)
                && selectedStateCurrent
                && state.summary) {
            return state.summary;
        }
        let id = entryIdentifier(item);
        if (id) {
            return selectedDetailState.cachedScoreSummaryForIdentifier(
                        String(id),
                        scoreGeneration,
                        entryScores(item),
                        root.useBeatorajaSelectOptions,
                        root.scoreOptionIdsUsed);
        }
        return buildScoreSummary(entryScores(item));
    }

    function clearTypeOf(score: var) : var {
        return skinCompatibleClearType(score?.result?.clearType || "NOPLAY");
    }

    function collapsedClearTypeLamp(clear: var) : var {
        switch (normalizedClearType(clear)) {
        case "FAILED":
            return 1;
        case "AEASY":
        case "LIGHTASSIST":
        case "EASY":
            return 2;
        case "NORMAL":
            return 3;
        case "HARD":
        case "EXHARD":
            return 4;
        case "FC":
        case "PERFECT":
        case "MAX":
            return 5;
        default:
            return 0;
        }
    }

    function clearTypeLamp(clear: var) : var {
        return collapsedClearTypeLamp(skinCompatibleClearType(clear));
    }

    function hasBarLampVariant(variants: var, variant: var) : var {
        return variants && variants.indexOf(variant) !== -1;
    }

    function usesExtendedBarLampVariants(variants: var) : var {
        if (!variants) {
            return false;
        }
        for (let i = 0; i < variants.length; ++i) {
            if (variants[i] > 5) {
                return true;
            }
        }
        return false;
    }

    function clearTypeBarLamp(clear: var, variants: var) : var {
        clear = skinCompatibleClearType(clear);
        if (!root.useBeatorajaSelectOptions || !usesExtendedBarLampVariants(variants)) {
            return collapsedClearTypeLamp(clear);
        }
        switch (clear) {
        case "FAILED":
            return 1;
        case "AEASY":
            return hasBarLampVariant(variants, 9) ? 9 : 2;
        case "LIGHTASSIST":
            return hasBarLampVariant(variants, 10) ? 10 : 2;
        case "EASY":
            return 2;
        case "NORMAL":
            return 3;
        case "HARD":
            return 4;
        case "EXHARD":
            return hasBarLampVariant(variants, 5) ? 5 : 4;
        case "FC":
            return 6;
        case "PERFECT":
            return hasBarLampVariant(variants, 7) ? 7 : 6;
        case "MAX":
            return hasBarLampVariant(variants, 8) ? 8 : 6;
        default:
            return 0;
        }
    }

    function isClearedScore(score: var) : var {
        let clearType = clearTypeOf(score);
        return clearType !== "FAILED" && clearType !== "NOPLAY";
    }

    function scoreListByNewest(scoreList: var) : var {
        let result = [];
        for (let score of scoreList) {
            if (score && score.result) {
                result.push(score);
            }
        }
        result.sort((a, b) => (b.result.unixTimestamp || 0) - (a.result.unixTimestamp || 0));
        return result;
    }

    function getClearType(scoreList: var) : var {
        let clearType = "NOPLAY";
        let priority = 0;
        for (let score of scoreList) {
            let next = clearTypeOf(score);
            let nextPriority = clearTypePriority(next);
            if (nextPriority > priority) {
                priority = nextPriority;
                clearType = next;
            }
        }
        return clearType;
    }

    function bestScoreByPoints(scoreList: var) : var {
        let best = null;
        let bestRate = -1;
        for (let score of scoreList) {
            let maxPoints = score?.result?.maxPoints || 0;
            if (maxPoints <= 0) {
                continue;
            }
            let rate = (score.result.points || 0) / maxPoints;
            if (rate > bestRate) {
                bestRate = rate;
                best = score;
            }
        }
        return best;
    }

    function scoreWithBestClear(scoreList: var) : var {
        let bestClearType = getClearType(scoreList);
        let best = null;
        for (let score of scoreListByNewest(scoreList)) {
            if (clearTypeOf(score) === bestClearType) {
                best = score;
                break;
            }
        }
        return best;
    }

    function scoreWithBestCombo(scoreList: var) : var {
        let best = null;
        let bestCombo = -1;
        for (let score of scoreList) {
            let combo = score?.result?.maxCombo || 0;
            if (combo > bestCombo) {
                bestCombo = combo;
                best = score;
            }
        }
        return best;
    }

    function replayScoreForType(item: var, replayType: var) : var {
        let scoreList = entryScores(item);
        switch (replayType) {
        case 0:
            return scoreListByNewest(scoreList)[0] || null;
        case 1:
            return bestScoreByPoints(scoreList);
        case 2:
            return scoreWithBestClear(scoreList);
        case 3:
            return scoreWithBestCombo(scoreList);
        default:
            return null;
        }
    }

    function judgementCount(counts: var, judgement: var) : var {
        return judgement >= 0 && judgement < counts.length ? (counts[judgement] || 0) : 0;
    }

    function emptyJudgeTimingCounts() : var {
        return {
            early: [0, 0, 0, 0, 0, 0],
            late: [0, 0, 0, 0, 0, 0]
        };
    }

    function timingBucketForJudgement(judgement: var) : var {
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

    function judgeTimingCountsForScore(score: var) : var {
        let counts = emptyJudgeTimingCounts();
        let replay = score && score.replayData ? score.replayData : null;
        if (replay && replay.earlyTimingCounts && replay.lateTimingCounts) {
            counts.early = replay.earlyTimingCounts;
            counts.late = replay.lateTimingCounts;
            return counts;
        }
        let events = replay && replay.hitEvents ? replay.hitEvents : [];
        for (let hit of events || []) {
            let judgement = hit && hit.points && hit.points.judgement !== undefined
                ? hit.points.judgement
                : -1;
            let bucket = timingBucketForJudgement(judgement);
            if (bucket < 0) {
                continue;
            }
            let side = hitDeviationNanos(hit) < 0 ? counts.early : counts.late;
            side[bucket] = (side[bucket] || 0) + 1;
        }
        return counts;
    }

    function timingCount(counts: var, bucket: var, early: var) : var {
        if (bucket < 0 || !counts) {
            return 0;
        }
        let source = early ? counts.early : counts.late;
        return source && bucket < source.length ? (source[bucket] || 0) : 0;
    }

    function totalTimingCount(counts: var, early: var) : var {
        let total = 0;
        for (let bucket = 1; bucket <= 5; ++bucket) {
            total += timingCount(counts, bucket, early);
        }
        return total;
    }

    function statsForScore(score: var, includeTiming: var) : var {
        if (!score || !score.result) {
            return null;
        }
        let counts = score.result.judgementCounts || [];
        let pg = judgementCount(counts, Judgement.Perfect);
        let gr = judgementCount(counts, Judgement.Great);
        let gd = judgementCount(counts, Judgement.Good);
        let bd = judgementCount(counts, Judgement.Bad);
        let poor = judgementCount(counts, Judgement.Poor);
        let miss = judgementCount(counts, Judgement.EmptyPoor);
        let pr = poor + miss;
        let timingCounts = includeTiming ? judgeTimingCountsForScore(score) : null;
        return {
            pg: pg,
            gr: gr,
            gd: gd,
            bd: bd,
            poor: poor,
            miss: miss,
            pr: pr,
            totalJudgements: Math.max(1, pg + gr + gd + bd + pr),
            comboBreak: bd + poor,
            badPoor: bd + pr,
            maxCombo: score.result.maxCombo || 0,
            score: score.result.points || 0,
            exscore: score.result.points || 0,
            maxPoints: score.result.maxPoints || 0,
            early: timingCounts ? timingCounts.early : null,
            late: timingCounts ? timingCounts.late : null,
            totalEarly: timingCounts ? totalTimingCount(timingCounts, true) : 0,
            totalLate: timingCounts ? totalTimingCount(timingCounts, false) : 0
        };
    }

    function bestStats(scoreList: var) : var {
        return buildScoreSummary(scoreList).bestStats;
    }

    function ensureStatsTiming(stats: var, score: var) : var {
        if (!stats || stats.early || !score) {
            return stats;
        }
        let timingCounts = judgeTimingCountsForScore(score);
        return Object.assign({}, stats, {
            early: timingCounts.early,
            late: timingCounts.late,
            totalEarly: totalTimingCount(timingCounts, true),
            totalLate: totalTimingCount(timingCounts, false)
        });
    }

    function ensureSelectedBestStatsTiming() : var {
        let state = selectedState;
        let summary = state.summary;
        return ensureStatsTiming(state.bestStats, summary.bestScore);
    }

    function scoreCounts(scoreList: var) : var {
        return buildScoreSummary(scoreList).scoreCounts;
    }

    function percentInteger(value: var, total: var) : var {
        return total > 0 ? Math.floor(Math.max(0, value || 0) * 100 / total) : 0;
    }

    function percentAfterDot(value: var, total: var) : var {
        return total > 0 ? Math.floor(Math.max(0, value || 0) * 10000 / total) % 100 : 0;
    }

    function scorePrintValue(stats: var, chart: var) : var {
        if (!stats) {
            return 0;
        }
        let totalNotes = chartPlayableNoteCount(chart);
        if (totalNotes <= 0) {
            totalNotes = Math.floor((stats.maxPoints || 0) / 2);
        }
        if (totalNotes <= 0) {
            return 0;
        }
        let keymode = chart ? (chart.keymode || 0) : 0;
        if (keymode === 5 || keymode === 10) {
            return Math.floor((100000 * stats.pg
                               + 100000 * stats.gr
                               + 50000 * stats.gd) / totalNotes);
        }
        if (keymode === 7 || keymode === 14) {
            return Math.floor((150000 * stats.pg
                               + 100000 * stats.gr
                               + 20000 * stats.gd) / totalNotes)
                + Math.floor(50000 * (stats.maxCombo || 0) / totalNotes);
        }
        return Math.floor((1000000 * stats.pg
                           + 700000 * stats.gr
                           + 400000 * stats.gd) / totalNotes);
    }

    function nextRankDelta(stats: var, chart: var) : var {
        if (!stats) {
            return 0;
        }

        let scoreMaxPoints = stats.maxPoints || 0;
        let targetMaxPoints = chartPlayableNoteCount(chart) * 2;
        if (targetMaxPoints <= 0) {
            targetMaxPoints = scoreMaxPoints;
        }
        if (scoreMaxPoints <= 0 || targetMaxPoints <= 0) {
            return 0;
        }

        let rate = Math.max(0, stats.exscore || 0) / scoreMaxPoints;
        for (let rank = 0; rank < 27; ++rank) {
            let qualified = rate >= rank / 27;
            if (rank % 3 === 0 && !qualified) {
                return Math.round(rank * targetMaxPoints / 27 - rate * targetMaxPoints);
            }
        }
        return Math.round(targetMaxPoints - rate * targetMaxPoints);
    }

    function currentFolderScoreCountsOrNull() {
        let key = folderLampKey(selectedState.item);
        return key && folderScoreCountsByKey[key] !== undefined
            ? folderScoreCountsByKey[key]
            : null;
    }

    function currentFolderScoreCounts() {
        return currentFolderScoreCountsOrNull() || emptyFolderScoreCounts;
    }

    function appendScoreClearOptionIds(clearType: var, ids: var) : void {
        // Historical score flags are beatoraja trophy options. The exact
        // selected-bar clear options are added from the current summary only.
        switch (skinCompatibleClearType(clearType)) {
        case "AEASY":
        case "LIGHTASSIST":
            ids.push(124);
            break;
        case "EASY":
            ids.push(121);
            break;
        case "NORMAL":
            ids.push(118);
            break;
        case "HARD":
            ids.push(119);
            break;
        case "EXHARD":
            ids.push(125);
            break;
        }
    }

    function appendScoreOptionIds(score: var, ids: var) : var {
        if (!score || !score.result) {
            return;
        }

        switch (score.result.noteOrderAlgorithm) {
        case NoteOrderAlgorithm.Mirror:
            ids.push(127);
            break;
        case NoteOrderAlgorithm.Random:
        case NoteOrderAlgorithm.RandomPlus:
            ids.push(128);
            break;
        case NoteOrderAlgorithm.SRandom:
        case NoteOrderAlgorithm.SRandomPlus:
            ids.push(129);
            break;
        case NoteOrderAlgorithm.RRandom:
            ids.push(1128);
            ids.push(130);
            break;
        default:
            ids.push(126);
            break;
        }

        if (score.result.dpOptions === DpOptions.Battle) {
            let keymode = score.result.keymode || 0;
            ids.push(keymode === 5 || keymode === 7 ? 145 : 144);
        }
    }

    function scoreOptionIds(item: var) : var {
        let state = selectedState;
        if ((item === state.item || item === state.chartData)
                && selectedStateScoreCurrent) {
            return state.scoreOptionIds || emptyScoreOptionIds;
        }
        return scoreOptionIdsFromSummary(scoreSummaryForItem(item));
    }

    function scoreOptionIdsFromSummary(summary: var) : var {
        if (summary && summary.optionIds !== undefined) {
            return summary.optionIds || emptyScoreOptionIds;
        }
        let scoreList = summary ? summary.scoreList : emptyScoreList;
        if (!scoreList || scoreList.length === 0) {
            return emptyScoreOptionIds;
        }
        let ids = [];
        for (let score of scoreList) {
            appendScoreClearOptionIds(clearTypeOf(score), ids);
            appendScoreOptionIds(score, ids);
        }
        ids = [...new Set(ids)];
        ids.sort((a, b) => a - b);
        return ids;
    }

    function entryLamp(item) {
        if (isRankingEntry(item)) {
            return clearTypeLamp(item.bestClearType);
        }
        if (isFolderLikeForLamp(item)) {
            let key = folderLampKey(item);
            return key && folderLampByKey[key] !== undefined ? folderLampByKey[key] : 0;
        }
        return scoreSummaryForItem(item).lamp;
    }

    function entryClearType(item: var) : var {
        if (isRankingEntry(item)) {
            return skinCompatibleClearType(item.bestClearType || "NOPLAY");
        }
        if (isFolderLikeForLamp(item)) {
            return "NOPLAY";
        }
        return scoreSummaryForItem(item).clearType;
    }

    function beatorajaClearOptionForClearType(clearType: var) : var {
        switch (skinCompatibleClearType(clearType)) {
        case "FAILED":
            return 101;
        case "AEASY":
            return 1100;
        case "LIGHTASSIST":
            return 1101;
        case "EASY":
            return 102;
        case "NORMAL":
            return 103;
        case "HARD":
            return 104;
        case "EXHARD":
            return 1102;
        case "FC":
            return 105;
        case "PERFECT":
            return 1103;
        case "MAX":
            return 1104;
        default:
            return 100;
        }
    }

    function beatorajaClearOption(item: var) : var {
        return beatorajaClearOptionForClearType(entryClearType(item));
    }

    function entryRank(item: var) : var {
        if (isRankingEntry(item)) {
            let points = Number(item.bestPoints || 0);
            let maxPoints = Number(item.maxPoints || 0);
            if (maxPoints <= 0) {
                return 0;
            }
            let rank = Math.floor(points * 9 / maxPoints);
            if (rank > 7) {
                rank = 8;
            }
            if (rank < 2 && points > 0) {
                rank = 1;
            }
            return rank;
        }
        return scoreSummaryForItem(item).rank;
    }

    function entryScoreRate(item: var) : var {
        return scoreSummaryForItem(item).scoreRate;
    }

    function refreshSelectedScoreState() : var {
        let item = focusedItem;
        let nextChartData = focusedChartData;
        let targetItem = focusedSelectionTarget;
        let itemKey = focusedSelectionKey;
        let targetItemKey = focusedSelectionTargetKey;
        if (selectedDetailState.selectedIdentityMatches(
                itemKey,
                targetItemKey,
                rankingMode,
                scoreGeneration,
                listGeneration,
                root.useBeatorajaSelectOptions,
                root.scoreOptionIdsUsed,
                root.difficultyStateUsed,
                root.difficultyLampStateUsed)) {
            return false;
        }
        let difficultyState = root.difficultyStateUsed ? difficultyStateForChart(nextChartData) : null;
        let targetId = entryIdentifier(targetItem);
        let targetScores = targetId ? entryScoresForIdentifier(targetId) : emptyScoreList;
        let changed = selectedDetailState.refreshSelectedFromQmlIdentityForIdentifier(
                itemKey,
                targetItemKey,
                rankingMode,
                scoreGeneration,
                listGeneration,
                item,
                nextChartData,
                targetScores,
                targetId ? String(targetId) : "",
                difficultyState ? (difficultyState.selectedDifficulty || 0) : 0,
                difficultyState ? (difficultyState.counts || emptyDifficultyNumbers) : emptyDifficultyNumbers,
                difficultyState ? (difficultyState.levels || emptyDifficultyNumbers) : emptyDifficultyNumbers,
                difficultyState && root.difficultyLampStateUsed ? (difficultyState.lamps || emptyDifficultyNumbers) : emptyDifficultyNumbers,
                root.useBeatorajaSelectOptions,
                root.scoreOptionIdsUsed,
                root.difficultyStateUsed,
                root.difficultyLampStateUsed);
        if (changed) {
            let chartData = nextChartData;
            let nextChartWrapper = chartData || null;
            if (visualChartWrapper !== nextChartWrapper) {
                visualChartWrapper = nextChartWrapper;
            }
            refreshVisualChartAssetSources(chartData);
        }
        return changed;
    }

    function rankingClearCountValue() : var {
        if (!rankingStatsAvailable()) {
            return 0;
        }
        let total = 0;
        for (let i = 0; i < arguments.length; ++i) {
            let clearType = arguments[i];
            total += rankingClearCounts && rankingClearCounts[clearType]
                ? rankingClearCounts[clearType]
                : 0;
        }
        return total;
    }

    function rankingClearPercent() : var {
        if (!rankingStatsAvailable() || rankingPlayerCount <= 0) {
            return 0;
        }
        let total = 0;
        for (let i = 0; i < arguments.length; ++i) {
            let clearType = arguments[i];
            total += rankingClearCounts && rankingClearCounts[clearType]
                ? rankingClearCounts[clearType]
                : 0;
        }
        return Math.floor(total * 100 / rankingPlayerCount);
    }

    function rankingClearPercentAfterDot() : var {
        if (!rankingStatsAvailable() || rankingPlayerCount <= 0) {
            return 0;
        }
        let total = 0;
        for (let i = 0; i < arguments.length; ++i) {
            let clearType = arguments[i];
            total += rankingClearCounts && rankingClearCounts[clearType]
                ? rankingClearCounts[clearType]
                : 0;
        }
        return Math.floor(total * 1000 / rankingPlayerCount) % 10;
    }

    function normalizedMd5(value: var) : var {
        return value ? String(value).toLowerCase() : "";
    }

    function rankingStatsAvailable() : var {
        let chart = selectedStateChartData;
        let currentMd5 = chart ? normalizedMd5(chart.md5) : "";
        return currentMd5.length > 0
            && currentMd5 === normalizedMd5(rankingStatsMd5)
            && rankingPlayerCount > 0;
    }

    function sameRankingClearCounts(a: var, b: var) : var {
        let left = a || {};
        let right = b || {};
        for (let key in left) {
            if (Number(left[key] || 0) !== Number(right[key] || 0)) {
                return false;
            }
        }
        for (let key in right) {
            if (Number(left[key] || 0) !== Number(right[key] || 0)) {
                return false;
            }
        }
        return true;
    }

    function skinCompatibleRankingClearCounts(counts: var) : var {
        if (!counts || root.useBeatorajaSelectOptions) {
            return counts || {};
        }
        let result = {};
        for (let key of Object.keys(counts)) {
            let mapped = skinCompatibleClearType(key);
            result[mapped] = (result[mapped] || 0) + (counts[key] || 0);
        }
        return result;
    }

    function setRankingStats(md5: var, clearCounts: var, playerCount: var, totalPlayCount: var, playerRank: var) : var {
        let nextMd5 = normalizedMd5(md5);
        let nextClearCounts = skinCompatibleRankingClearCounts(clearCounts || {});
        let nextPlayerRank = Math.max(0, Number(playerRank || 0));
        let nextPlayerCount = Math.max(0, Number(playerCount || 0));
        let nextTotalPlayCount = Math.max(0, Number(totalPlayCount || nextPlayerCount));
        if (rankingStatsMd5 === nextMd5
                && rankingPlayerRank === nextPlayerRank
                && rankingPlayerCount === nextPlayerCount
                && rankingTotalPlayCount === nextTotalPlayCount
                && sameRankingClearCounts(rankingClearCounts, nextClearCounts)) {
            return false;
        }
        rankingStatsMd5 = nextMd5;
        rankingClearCounts = nextClearCounts;
        rankingPlayerRank = nextPlayerRank;
        rankingPlayerCount = nextPlayerCount;
        rankingTotalPlayCount = nextTotalPlayCount;
        return true;
    }

    function rankingEntryFrom(entry: var, index: var, chart: var) : var {
        return {
            __lr2RankingEntry: true,
            rankingIndex: index,
            sourceMd5: chart ? (chart.md5 || "") : "",
            title: entry.userName || entry.owner || "",
            level: index + 1,
            bestClearType: entry.bestClearType || "NOPLAY",
            bestPoints: Number(entry.bestPoints || 0),
            maxPoints: Number(entry.maxPoints || 0),
            bestCombo: Number(entry.bestCombo || 0),
            bestComboBreaks: Number(entry.bestComboBreaks || 0),
            scoreCount: Number(entry.scoreCount || 0),
            userId: entry.userId || 0
        };
    }

    function showRanking(entries: var, clearCounts: var, playerCount: var, totalPlayCount: var, playerRank: var) : var {
        let chart = selectedStateChartData;
        if (!chart || !entries || entries.length <= 0) {
            return false;
        }
        setRankingStats(chart.md5,
                        clearCounts,
                        playerCount || entries.length,
                        totalPlayCount || playerCount || entries.length,
                        playerRank || 0);
        if (rankingMode) {
            return true;
        }

        rankingSavedItems = items.slice();
        rankingSavedFolderContents = folderContents.slice();
        rankingSavedRealItemCount = realItemCount;
        rankingSavedCurrentIndex = currentIndex;
        rankingSavedVisualIndex = normalizedVisualIndex;
        rankingSavedSelectedOffset = selectedOffset;
        rankingBaseItem = chart;

        let rows = [];
        let limit = Math.min(entries.length, 999);
        for (let i = 0; i < limit; ++i) {
            rows.push(rankingEntryFrom(entries[i], i, chart));
        }
        realItemCount = rows.length;
        addToMinimumCount(rows);
        items = rows;
        folderContents = rows;
        rankingMode = true;
        currentIndex = 0;
        targetIndex = 0;
        selectedOffset = 0;
        setVisualIndexImmediate(0);
        markListContentsChanged();
        nativeNavigation.touchSelection();
        return true;
    }

    function hideRanking() : var {
        if (!rankingMode) {
            return false;
        }
        let restoreItems = rankingSavedItems.slice();
        let restoreFolderContents = rankingSavedFolderContents.slice();
        let restoreCount = rankingSavedRealItemCount;
        let restoreIndex = Math.max(0, Math.min(Math.max(0, restoreCount - 1), rankingSavedCurrentIndex));

        rankingMode = false;
        rankingBaseItem = null;
        items = restoreItems;
        folderContents = restoreFolderContents;
        realItemCount = restoreCount;
        currentIndex = restoreIndex;
        targetIndex = restoreIndex;
        selectedOffset = rankingSavedSelectedOffset;
        setVisualIndexImmediate(rankingSavedVisualIndex);
        markListContentsChanged();
        nativeNavigation.touchSelection();
        return true;
    }

    function chartFolderKey(chart: var) : var {
        if (!chart) {
            return "";
        }
        // LR2 groups difficulty buttons by the song folder, not by the
        // currently selected chart row.
        return chart.chartDirectory || chart.directory || "";
    }

    function chartKeymodeKey(chart: var) : var {
        return String(chart ? (chart.keymode || 0) : 0);
    }

    function ensureChartGroup(groups: var, chart: var) : var {
        let folderKey = root.chartFolderKey(chart);
        let keymodeKey = root.chartKeymodeKey(chart);
        let folderGroups = groups[folderKey];
        if (!folderGroups) {
            folderGroups = {};
            groups[folderKey] = folderGroups;
        }
        let group = folderGroups[keymodeKey];
        if (!group) {
            group = [];
            folderGroups[keymodeKey] = group;
        }
        return group;
    }

    function chartGroupForChart(groups: var, chart: var) : var {
        let folderGroups = groups[root.chartFolderKey(chart)];
        return folderGroups ? (folderGroups[root.chartKeymodeKey(chart)] || []) : [];
    }

    function difficultyCacheForChart(cache: var, chart: var, create: var) : var {
        let folderKey = root.chartFolderKey(chart);
        let keymodeKey = root.chartKeymodeKey(chart);
        let folderCache = cache[folderKey];
        if (!folderCache && create) {
            folderCache = {};
            cache[folderKey] = folderCache;
        }
        if (!folderCache) {
            return null;
        }
        let keymodeCache = folderCache[keymodeKey];
        if (!keymodeCache && create) {
            keymodeCache = {};
            folderCache[keymodeKey] = keymodeCache;
        }
        return keymodeCache || null;
    }

    function chartDifficultyStateItemKey(chart: var) : var {
        if (!chart) {
            return "";
        }
        return chart.path || chart.md5 || chart.sha256 || entrySelectionKey(chart, 0);
    }

    function difficultyHint(chart: var) : var {
        let text = ((chart.title || "") + " " + (chart.subtitle || "")).toLowerCase();
        if (/\binsane\b/.test(text)) return 5;
        if (/\banother\b/.test(text)) return 4;
        if (/\bhyper\b/.test(text)) return 3;
        if (/\bnormal\b/.test(text)) return 2;
        if (/\bbeginner\b|\bbgn\b/.test(text)) return 1;
        if (entryPlayLevel(chart) <= 1) return 1;
        return 0;
    }

    function inferGroupDifficulties(group: var) : var {
        let result = {};
        let hasInvalid = false;
        let allBeginner = group.length > 1;
        let hasNonTrivialLevel = false;
        for (let chart of group) {
            let raw = rawChartDifficulty(chart);
            hasInvalid = hasInvalid || raw < 1 || raw > 5;
            allBeginner = allBeginner && raw === 1;
            hasNonTrivialLevel = hasNonTrivialLevel || entryPlayLevel(chart) > 1;
        }

        // New scans keep missing #DIFFICULTY as invalid. Older scans used to
        // coerce it to BEGINNER; infer those only when the whole group has that
        // shape, so explicit beginner singles stay untouched.
        if (!hasInvalid && !(allBeginner && hasNonTrivialLevel)) {
            for (let chart of group) {
                result[chart.path || ""] = Math.max(0, rawChartDifficulty(chart));
            }
            return result;
        }

        let sorted = group.slice();
        sorted.sort((a, b) => {
            let noteDiff = chartNoteCount(a) - chartNoteCount(b);
            if (noteDiff !== 0) {
                return noteDiff;
            }
            return (a.path || "").localeCompare(b.path || "");
        });

        let inferred = 1;
        for (let chart of sorted) {
            let raw = rawChartDifficulty(chart);
            let hint = allBeginner ? difficultyHint(chart) : 0;
            if (raw >= 1 && raw <= 5 && !allBeginner) {
                inferred = raw;
            } else if (hint > 0) {
                inferred = hint;
            } else {
                inferred += 1;
                if (inferred === 5) {
                    inferred = 4;
                } else if (inferred < 1) {
                    inferred = 2;
                } else if (inferred > 5) {
                    inferred = 5;
                }
            }
            result[chart.path || ""] = inferred;
        }
        return result;
    }

    function rebuildFolderIndexes(sourceItems: var) : void {
        let groups = {};
        let difficulties = {};

        for (let item of sourceItems) {
            if (!isChart(item)) {
                continue;
            }

            root.ensureChartGroup(groups, item).push(item);
        }

        for (let folderGroups of Object.values(groups)) {
            for (let group of Object.values(folderGroups)) {
                Object.assign(difficulties, inferGroupDifficulties(group));
            }
        }

        chartGroupsByFolderKeymode = groups;
        chartDifficultyByPath = difficulties;
        clearSelectedDifficultyStateCache();
    }

    function chartsForCurrentSong() : var {
        return chartsForSong(selectedStateChartData);
    }

    function chartsForSong(chart: var) : var {
        if (!chart) {
            return [];
        }

        let result = chartGroupForChart(chartGroupsByFolderKeymode, chart);
        if (result.length === 0) {
            result.push(chart);
        }
        return result;
    }

    function buildDifficultyGroupState(chart: var) : var {
        let charts = chartsForSong(chart);
        let counts = [0, 0, 0, 0, 0, 0];
        let levels = [0, 0, 0, 0, 0, 0];
        let lamps = [0, 0, 0, 0, 0, 0];
        let includeLamps = difficultyLampStateUsed;
        for (let candidate of charts) {
            let diff = entryDifficulty(candidate);
            if (diff < 1 || diff > 5) {
                continue;
            }
            counts[diff] += 1;
            if (counts[diff] === 1) {
                levels[diff] = candidate.playLevel || 0;
                if (includeLamps) {
                    lamps[diff] = entryLamp(candidate);
                }
            }
        }
        return {
            counts: counts,
            levels: levels,
            lamps: lamps
        };
    }

    function difficultyGroupStateForChart(chart: var) : var {
        let lampScoreGeneration = difficultyLampStateUsed ? scoreGeneration : 0;
        let useBeatorajaLampState = difficultyLampStateUsed && root.useBeatorajaSelectOptions;
        let keymodeCache = difficultyCacheForChart(selectedDifficultyGroupStateCache, chart, false);
        let cached = keymodeCache ? keymodeCache.groupState : null;
        if (cached !== undefined
                && cached !== null
                && cached.lampScoreGeneration === lampScoreGeneration
                && cached.useBeatorajaLampState === useBeatorajaLampState
                && cached.difficultyLampStateUsed === difficultyLampStateUsed) {
            return cached.state;
        }
        let state = buildDifficultyGroupState(chart);
        difficultyCacheForChart(selectedDifficultyGroupStateCache, chart, true).groupState = {
            lampScoreGeneration: lampScoreGeneration,
            useBeatorajaLampState: useBeatorajaLampState,
            difficultyLampStateUsed: difficultyLampStateUsed,
            state: state
        };
        return state;
    }

    function buildSelectedDifficultyState(chart: var) : var {
        let selectedDifficulty = entryDifficulty(chart);
        let groupState = difficultyGroupStateForChart(chart);
        let levels = groupState.levels;
        if (selectedDifficulty >= 1
                && selectedDifficulty <= 5
                && chart) {
            let selectedLevel = chart.playLevel || 0;
            if (levels[selectedDifficulty] !== selectedLevel) {
                levels = levels.slice();
                levels[selectedDifficulty] = selectedLevel;
            }
        }
        return {
            counts: groupState.counts,
            levels: levels,
            lamps: groupState.lamps,
            selectedDifficulty: selectedDifficulty
        };
    }

    function difficultyStateForChart(chart: var) : var {
        if (!chart || !difficultyStateUsed) {
            return emptyDifficultyState;
        }

        let itemKey = chartDifficultyStateItemKey(chart);
        let lampScoreGeneration = difficultyLampStateUsed ? scoreGeneration : 0;
        let useBeatorajaLampState = difficultyLampStateUsed && root.useBeatorajaSelectOptions;
        let keymodeCache = difficultyCacheForChart(selectedDifficultyStateCache, chart, false);
        let cached = keymodeCache ? keymodeCache[itemKey] : null;
        if (cached !== undefined
                && cached !== null
                && cached.lampScoreGeneration === lampScoreGeneration
                && cached.useBeatorajaLampState === useBeatorajaLampState
                && cached.difficultyLampStateUsed === difficultyLampStateUsed) {
            return cached.state;
        }
        let state = buildSelectedDifficultyState(chart);
        difficultyCacheForChart(selectedDifficultyStateCache, chart, true)[itemKey] = {
            lampScoreGeneration: lampScoreGeneration,
            useBeatorajaLampState: useBeatorajaLampState,
            difficultyLampStateUsed: difficultyLampStateUsed,
            state: state
        };
        return state;
    }

    function chartForDifficulty(diff: var) : var {
        if (diff < 1 || diff > 5) {
            return null;
        }
        for (let chart of chartsForCurrentSong()) {
            if (entryDifficulty(chart) === diff) {
                return chart;
            }
        }
        return null;
    }

    function nextChartForDifficulty(diff: var) : var {
        let currentChart = selectedStateChartData;
        let candidates = [];
        for (let chart of chartsForCurrentSong()) {
            if (entryDifficulty(chart) === diff) {
                candidates.push(chart);
            }
        }
        if (candidates.length === 0) {
            return null;
        }
        if (!currentChart) {
            return candidates[0];
        }
        let currentIdx = candidates.findIndex((chart) => sameEntry(chart, currentChart));
        return candidates[(currentIdx + 1) % candidates.length];
    }

    function clickDifficulty(diff: var) : var {
        diff = Math.max(0, Math.min(5, diff || 0));
        if (diff === 0) {
            difficultyFilter = 0;
            sortOrFilterChanged();
            return;
        }

        let currentChart = selectedStateChartData;
        let target = currentChart && entryDifficulty(currentChart) === diff
            ? nextChartForDifficulty(diff)
            : chartForDifficulty(diff);
        difficultyFilter = diff;
        sortOrFilterChanged(target || currentChart);
    }

    function difficultyCount(diff: var) : var {
        let model = selectedState.difficultyModel;
        return diff >= 1 && diff <= 5 && model ? model.countForDifficulty(diff) : 0;
    }

    function difficultyPlayLevel(diff: var) : var {
        let model = selectedState.difficultyModel;
        return diff >= 1 && diff <= 5 && model ? model.playLevelForDifficulty(diff) : 0;
    }

    function levelBarFlashThreshold() : var {
        let chart = selectedStateChartData;
        let keymode = chart ? (chart.keymode || 0) : 0;
        if (keymode === 5 || keymode === 10) {
            return 9;
        }
        if (keymode === 7 || keymode === 14) {
            return 12;
        }
        return 12;
    }

    function difficultyLevelBarOption(diff: var) : var {
        let level = difficultyPlayLevel(diff);
        return level > levelBarFlashThreshold() ? 74 + diff : 69 + diff;
    }

    function difficultyGraphValue(diff: var) : var {
        let level = difficultyPlayLevel(diff);
        if (level <= 0) {
            return 0;
        }
        return level / Math.max(1, levelBarFlashThreshold());
    }

    function difficultyLamp(diff: var) : var {
        let model = selectedState.difficultyModel;
        return diff >= 1 && diff <= 5 && model ? model.lampForDifficulty(diff) : 0;
    }

    function attachedTextFile(chart: var) : string {
        if (!chart || !chart.chartDirectory) {
            return "";
        }

        let dir = chart.chartDirectory;
        return attachedTextByDirectory[dir] || "";
    }

    function hasAttachedText(chart: var) : bool {
        let dir = chart ? chart.chartDirectory : "";
        if (!dir) {
            return false;
        }

        return hasAttachedTextByDirectory[dir] === true;
    }

    function hasReplay(chart: var) : var {
        return entryScores(chart).length > 0;
    }

    function hasBga(chart: var) : var {
        return !!chart && (!!chart.stageFile || !!chart.banner || !!chart.backBmp);
    }

    function hasLongNote(chart: var) : var {
        return !!chart && ((chart.lnCount || 0) + (chart.bssCount || 0)) > 0;
    }

    function chartRankValue(value: var) : var {
        if (!value) {
            return NaN;
        }
        if (value.chartData) {
            return chartRankValue(value.chartData);
        }
        if (value.rawItem) {
            return chartRankValue(value.rawItem);
        }
        if (value.rank === undefined || value.rank === null) {
            return NaN;
        }
        let rank = Number(value.rank);
        return isFinite(rank) ? rank : NaN;
    }

    function judgeRank(chart: var, fallbackItem: var) : var {
        let rank = chartRankValue(chart);
        if (!isFinite(rank) && fallbackItem && !isFolderLikeForLamp(fallbackItem)) {
            rank = chartRankValue(fallbackItem);
        }
        if (!isFinite(rank) && (isChart(chart) || isEntry(chart)
                || isChart(fallbackItem) || isEntry(fallbackItem))) {
            return 75;
        }
        return rank;
    }

    function judgeOption(chart: var, fallbackItem: var) : var {
        if (isMissingTableEntry(chart) || isMissingTableEntry(fallbackItem)) {
            return 180;
        }
        if (isFolderLikeForLamp(chart) || (!chart && isFolderLikeForLamp(fallbackItem))) {
            return 0;
        }
        let rank = judgeRank(chart, fallbackItem);
        if (!isFinite(rank)) {
            return 0;
        }
        let option = 183;
        if (rank <= 25) {
            option = 180;
        } else if (rank <= 50) {
            option = 181;
        } else if (rank <= 75) {
            option = 182;
        }
        return option;
    }

    function highLevelOption(chart: var) : var {
        if (!chart) {
            return 185;
        }
        let keymode = chart.keymode || 0;
        let threshold = 10;
        if (keymode === 7 || keymode === 14) {
            threshold = 13;
        }
        return (chart.playLevel || 0) >= threshold ? 186 : 185;
    }

    function numberValue(num: var) : var {
        return computeNumberValue(num);
    }

    function computeNumberValue(num: var) : var {
        let state = selectedState;
        let chartLoaded = false;
        let chartValue = null;
        let rankingEntryLoaded = false;
        let rankingEntryValue = null;
        let statsLoaded = false;
        let statsValue = null;
        let timingStatsLoaded = false;
        let timingStatsValue = null;
        let countsLoaded = false;
        let countsValue = null;
        let folderCountsLoaded = false;
        let folderCountsValue = null;
        let rankingStatsLoaded = false;
        let rankingStatsValue = false;

        function chart() {
            if (!chartLoaded) {
                chartLoaded = true;
                chartValue = state.chartData;
            }
            return chartValue;
        }

        function rankingEntry() {
            if (!rankingEntryLoaded) {
                rankingEntryLoaded = true;
                rankingEntryValue = isRankingEntry(state.item) ? state.item : null;
            }
            return rankingEntryValue;
        }

        function stats() {
            if (!statsLoaded) {
                statsLoaded = true;
                statsValue = state.bestStats;
            }
            return statsValue;
        }

        function timingStats() {
            if (!timingStatsLoaded) {
                timingStatsLoaded = true;
                timingStatsValue = ensureSelectedBestStatsTiming();
            }
            return timingStatsValue;
        }

        function counts() {
            if (!countsLoaded) {
                countsLoaded = true;
                countsValue = state.scoreCounts || emptyScoreCounts;
            }
            return countsValue;
        }

        function currentFolderCounts() {
            if (!folderCountsLoaded) {
                folderCountsLoaded = true;
                folderCountsValue = currentFolderScoreCountsOrNull();
            }
            return folderCountsValue;
        }

        function folderCounts() {
            return currentFolderCounts() || emptyFolderScoreCounts;
        }

        function hasRankingStats() {
            if (!rankingStatsLoaded) {
                rankingStatsLoaded = true;
                rankingStatsValue = rankingStatsAvailable();
            }
            return rankingStatsValue;
        }

        switch (num) {
        case 30:
            return playerStats.playCount || 0;
        case 31:
            return playerStats.clearCount || 0;
        case 32:
            return playerStats.failCount || 0;
        case 33:
            return playerStats.perfectCount || 0;
        case 34:
            return playerStats.greatCount || 0;
        case 35:
            return playerStats.goodCount || 0;
        case 36:
            return playerStats.badCount || 0;
        case 37:
            return playerStats.poorCount || 0;
        case 333:
            return (playerStats.perfectCount || 0)
                + (playerStats.greatCount || 0)
                + (playerStats.goodCount || 0)
                + (playerStats.badCount || 0);
        case 38:
            return 0;
        case 39:
            return playerStats.maxCombo || 0;
        case 40:
            return 0;
        case 10:
            return 100;
        case 11:
            return 100;
        case 12:
            return Rg.profileList.mainProfile.vars.generalVars.offset || 0;
        case 13:
            return 0;
        case 42:
        case 96:
            return chart() ? (chart().playLevel || 0) : 0;
        case 45:
        case 46:
        case 47:
        case 48:
        case 49:
            return hasDifficulty(num - 44) ? difficultyPlayLevel(num - 44) : -1;
        case 57:
            return 100;
        case 58:
            return 100;
        case 59:
            return 100;
        case 70:
            return rankingEntry() ? Math.round(rankingEntry().bestPoints) : (stats() ? Math.round(stats().score) : 0);
        case 71:
            return rankingEntry() ? Math.round(rankingEntry().bestPoints) : (stats() ? Math.round(stats().exscore) : 0);
        case 72:
            return rankingEntry() ? Math.round(rankingEntry().maxPoints) : (stats() ? Math.round(stats().maxPoints) : 0);
        case 73:
            return rankingEntry() && rankingEntry().maxPoints > 0
                ? Math.round(rankingEntry().bestPoints * 100 / rankingEntry().maxPoints)
                : (stats() && stats().maxPoints > 0 ? Math.round(stats().score * 100 / stats().maxPoints) : 0);
        case 74:
            return rankingEntry() ? Math.round(rankingEntry().maxPoints / 2) : chartPlayableNoteCount(chart());
        case 75:
            return rankingEntry() ? rankingEntry().bestCombo : (stats() ? stats().maxCombo : 0);
        case 76:
            return rankingEntry() ? rankingEntry().bestComboBreaks : counts().minBadPoor;
        case 77:
            return rankingEntry() ? rankingEntry().scoreCount : counts().play;
        case 78:
            return counts().clear;
        case 79:
            return counts().fail;
        case 100:
            return scorePrintValue(stats(), chart());
        case 101:
            return stats() ? Math.round(stats().exscore) : 0;
        case 102:
            return stats() ? percentInteger(stats().exscore, stats().maxPoints) : 0;
        case 103:
            return stats() ? percentAfterDot(stats().exscore, stats().maxPoints) : 0;
        case 104:
        case 105:
            return stats() ? stats().maxCombo : 0;
        case 106:
            return chartPlayableNoteCount(chart());
        case 107:
            return 0;
        case 108:
        case 128:
            return stats() ? Math.round(stats().exscore) : 0;
        case 109:
            return 0;
        case 80:
            return stats() ? stats().pg : 0;
        case 81:
            return stats() ? stats().gr : 0;
        case 82:
            return stats() ? stats().gd : 0;
        case 83:
            return stats() ? stats().bd : 0;
        case 84:
            return stats() ? stats().poor : 0;
        case 85:
            return stats() ? percentInteger(stats().pg, stats().totalJudgements) : 0;
        case 86:
            return stats() ? percentInteger(stats().gr, stats().totalJudgements) : 0;
        case 87:
            return stats() ? percentInteger(stats().gd, stats().totalJudgements) : 0;
        case 88:
            return stats() ? percentInteger(stats().bd, stats().totalJudgements) : 0;
        case 89:
            return stats() ? percentInteger(stats().poor, stats().totalJudgements) : 0;
        case 110:
            return stats() ? stats().pg : 0;
        case 111:
            return stats() ? stats().gr : 0;
        case 112:
            return stats() ? stats().gd : 0;
        case 113:
            return stats() ? stats().bd : 0;
        case 114:
            return stats() ? stats().poor : 0;
        case 115:
            return stats() ? percentInteger(stats().exscore, stats().maxPoints) : 0;
        case 116:
            return stats() ? percentAfterDot(stats().exscore, stats().maxPoints) : 0;
        case 410:
            return timingStats() ? timingCount(timingStats(), 0, true) : 0;
        case 411:
            return timingStats() ? timingCount(timingStats(), 0, false) : 0;
        case 412:
            return timingStats() ? timingCount(timingStats(), 1, true) : 0;
        case 413:
            return timingStats() ? timingCount(timingStats(), 1, false) : 0;
        case 414:
            return timingStats() ? timingCount(timingStats(), 2, true) : 0;
        case 415:
            return timingStats() ? timingCount(timingStats(), 2, false) : 0;
        case 416:
            return timingStats() ? timingCount(timingStats(), 3, true) : 0;
        case 417:
            return timingStats() ? timingCount(timingStats(), 3, false) : 0;
        case 418:
            return timingStats() ? timingCount(timingStats(), 4, true) : 0;
        case 419:
            return timingStats() ? timingCount(timingStats(), 4, false) : 0;
        case 121:
        case 151:
            return 0;
        case 122:
        case 135:
            return 0;
        case 123:
        case 136:
            return 0;
        case 150:
            return stats() ? Math.round(stats().exscore) : 0;
        case 152:
            return stats() ? Math.round(stats().exscore) : 0;
        case 154:
            return nextRankDelta(stats(), chart());
        case 420:
            return stats() ? stats().miss : 0;
        case 421:
            return timingStats() ? timingCount(timingStats(), 5, true) : 0;
        case 422:
            return timingStats() ? timingCount(timingStats(), 5, false) : 0;
        case 423:
            return timingStats() ? timingStats().totalEarly : 0;
        case 424:
            return timingStats() ? timingStats().totalLate : 0;
        case 425:
            return stats() ? stats().comboBreak : 0;
        case 426:
            return stats() ? stats().pr : 0;
        case 427:
            return stats() ? stats().badPoor : 0;
        case 92:
            return hasRankingStats() ? rankingPlayerRank : 0;
        case 93:
            return hasRankingStats() ? rankingPlayerCount : 0;
        case 94:
            return hasRankingStats() ? rankingClearPercent("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : 0;
        case 179:
            return hasRankingStats() ? rankingPlayerRank : 0;
        case 180:
            return hasRankingStats() ? rankingPlayerCount : 0;
        case 181:
            return hasRankingStats() ? rankingClearPercent("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : 0;
        case 182:
            return 0;
        case 200:
            return hasRankingStats() ? rankingPlayerCount : counts().play;
        case 201:
            return hasRankingStats() ? rankingTotalPlayCount : counts().play;
        case 202:
            return hasRankingStats() ? rankingClearCountValue("NOPLAY") : counts().noplay;
        case 203:
            return hasRankingStats() ? rankingClearPercent("NOPLAY") : percentInteger(counts().noplay, counts().play + counts().noplay);
        case 204:
            return hasRankingStats() ? rankingClearCountValue("AEASY") : counts().assist;
        case 205:
            return hasRankingStats() ? rankingClearPercent("AEASY") : percentInteger(counts().assist, counts().play);
        case 206:
            return hasRankingStats() ? rankingClearCountValue("LIGHTASSIST") : counts().lightAssist;
        case 207:
            return hasRankingStats() ? rankingClearPercent("LIGHTASSIST") : percentInteger(counts().lightAssist, counts().play);
        case 208:
            return hasRankingStats() ? rankingClearCountValue("EXHARD") : counts().exhard;
        case 209:
            return hasRankingStats() ? rankingClearPercent("EXHARD") : percentInteger(counts().exhard, counts().play);
        case 210:
            return hasRankingStats() ? rankingClearCountValue("FAILED") : counts().fail;
        case 211:
            return hasRankingStats() ? rankingClearPercent("FAILED") : percentInteger(counts().fail, counts().play);
        case 212:
            return hasRankingStats() ? rankingClearCountValue("EASY") : counts().easy;
        case 213:
            return hasRankingStats() ? rankingClearPercent("EASY") : percentInteger(counts().easy, counts().play);
        case 214:
            return hasRankingStats() ? rankingClearCountValue("NORMAL") : counts().normal;
        case 215:
            return hasRankingStats() ? rankingClearPercent("NORMAL") : percentInteger(counts().normal, counts().play);
        case 216:
            return hasRankingStats() ? rankingClearCountValue("HARD") : counts().hard;
        case 217:
            return hasRankingStats() ? rankingClearPercent("HARD") : percentInteger(counts().hard, counts().play);
        case 218:
            return hasRankingStats() ? rankingClearCountValue("FC") : counts().fc;
        case 219:
            return hasRankingStats() ? rankingClearPercent("FC") : percentInteger(counts().fc, counts().play);
        case 222:
            return hasRankingStats() ? rankingClearCountValue("PERFECT") : counts().perfect;
        case 223:
            return hasRankingStats() ? rankingClearPercent("PERFECT") : percentInteger(counts().perfect, counts().play);
        case 224:
            return hasRankingStats() ? rankingClearCountValue("MAX") : counts().max;
        case 225:
            return hasRankingStats() ? rankingClearPercent("MAX") : percentInteger(counts().max, counts().play);
        case 226:
            return hasRankingStats() ? rankingClearCountValue("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : counts().clear;
        case 227:
            return hasRankingStats() ? rankingClearPercent("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : percentInteger(counts().clear, counts().play);
        case 228:
            return hasRankingStats() ? rankingClearCountValue("FC", "PERFECT", "MAX") : counts().fc + counts().perfect + counts().max;
        case 229:
            return hasRankingStats() ? rankingClearPercent("FC", "PERFECT", "MAX") : percentInteger(counts().fc + counts().perfect + counts().max, counts().play);
        case 230:
            return hasRankingStats() ? rankingClearPercentAfterDot("NOPLAY") : percentAfterDot(counts().noplay, counts().play + counts().noplay);
        case 231:
            return hasRankingStats() ? rankingClearPercentAfterDot("AEASY") : percentAfterDot(counts().assist, counts().play);
        case 232:
            return hasRankingStats() ? rankingClearPercentAfterDot("LIGHTASSIST") : percentAfterDot(counts().lightAssist, counts().play);
        case 233:
            return hasRankingStats() ? rankingClearPercentAfterDot("EXHARD") : percentAfterDot(counts().exhard, counts().play);
        case 234:
            return hasRankingStats() ? rankingClearPercentAfterDot("FAILED") : percentAfterDot(counts().fail, counts().play);
        case 235:
            return hasRankingStats() ? rankingClearPercentAfterDot("EASY") : percentAfterDot(counts().easy, counts().play);
        case 236:
            return hasRankingStats() ? rankingClearPercentAfterDot("NORMAL") : percentAfterDot(counts().normal, counts().play);
        case 237:
            return hasRankingStats() ? rankingClearPercentAfterDot("HARD") : percentAfterDot(counts().hard, counts().play);
        case 238:
            return hasRankingStats() ? rankingClearPercentAfterDot("FC") : percentAfterDot(counts().fc, counts().play);
        case 239:
            return hasRankingStats() ? rankingClearPercentAfterDot("PERFECT") : percentAfterDot(counts().perfect, counts().play);
        case 240:
            return hasRankingStats() ? rankingClearPercentAfterDot("MAX") : percentAfterDot(counts().max, counts().play);
        case 241:
            return hasRankingStats() ? rankingClearPercentAfterDot("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : percentAfterDot(counts().clear, counts().play);
        case 242:
            return hasRankingStats() ? rankingClearPercentAfterDot("FC", "PERFECT", "MAX") : percentAfterDot(counts().fc + counts().perfect + counts().max, counts().play);
        case 90:
        case 290:
            if (isMissingTableEntry(chart())) {
                return 0;
            }
            return chart() && (chart().maxBpm || chart().mainBpm)
                ? Math.round(chart().maxBpm || chart().mainBpm)
                : -1;
        case 91:
        case 291:
            if (isMissingTableEntry(chart())) {
                return 0;
            }
            return chart() && (chart().minBpm || chart().mainBpm)
                ? Math.round(chart().minBpm || chart().mainBpm)
                : -1;
        case 92:
            if (isMissingTableEntry(chart())) {
                return 0;
            }
            return chart() && chart().mainBpm ? Math.round(chart().mainBpm) : -1;
        case 300:
            return currentFolderCounts() ? folderCounts().total : -1;
        case 320:
            return currentFolderCounts() ? folderCounts().noplay : -1;
        case 321:
            return currentFolderCounts() ? folderCounts().fail : -1;
        case 322:
            return currentFolderCounts() ? folderCounts().assist : -1;
        case 323:
            return currentFolderCounts() ? folderCounts().lightAssist : -1;
        case 324:
            return currentFolderCounts() ? folderCounts().easy : -1;
        case 325:
            return currentFolderCounts() ? folderCounts().normal : -1;
        case 326:
            return currentFolderCounts() ? folderCounts().hard : -1;
        case 327:
            return currentFolderCounts() ? folderCounts().exhard : -1;
        case 328:
            return currentFolderCounts() ? folderCounts().fc : -1;
        case 329:
            return currentFolderCounts() ? folderCounts().perfect : -1;
        case 330:
            return currentFolderCounts() ? folderCounts().max : -1;
        case 350:
            if (isMissingTableEntry(chart())) {
                return -1;
            }
            return chart() ? (chart().normalNoteCount || 0) : -1;
        case 351:
            if (isMissingTableEntry(chart())) {
                return -1;
            }
            return chart() ? (chart().lnCount || 0) : -1;
        case 352:
            if (isMissingTableEntry(chart())) {
                return -1;
            }
            return chart() ? (chart().scratchCount || 0) : -1;
        case 353:
            if (isMissingTableEntry(chart())) {
                return -1;
            }
            return chart() ? (chart().bssCount || 0) : -1;
        case 354:
            if (isMissingTableEntry(chart())) {
                return -1;
            }
            return chart() ? (chart().mineCount || 0) : -1;
        case 360:
            return chartDensityValue(chart(), "peakDensity", false);
        case 361:
            return chartDensityValue(chart(), "peakDensity", true);
        case 362:
            return chartDensityValue(chart(), "endDensity", false);
        case 363:
            return chartDensityValue(chart(), "endDensity", true);
        case 364:
            return chartDensityWhole(chart());
        case 365:
            return chartDensityAfterDot(chart());
        case 368:
            if (isMissingTableEntry(chart())) {
                return -1;
            }
            return chart() ? Math.floor(chart().total || 0) : -1;
        case 1163: {
            let seconds = chartLengthSeconds(chart());
            return seconds >= 0 ? Math.floor(seconds / 60) % 60 : -1;
        }
        case 1164: {
            let seconds = chartLengthSeconds(chart());
            return seconds >= 0 ? seconds % 60 : -1;
        }
        default:
            return 0;
        }
    }

    function hasDifficulty(diff: var) : var {
        return difficultyCount(diff) > 0;
    }

    function barGraphValue(type: var) : var {
        if (type === 101) {
            return normalizedVisualIndex;
        }
        return computeBarGraphValue(type);
    }

    function computeBarGraphValue(type: var) : var {
        let state = selectedState;
        let statsLoaded = false;
        let statsValue = null;
        let chartLoaded = false;
        let chartValue = null;
        let stats = () => {
            if (!statsLoaded) {
                statsValue = state.bestStats || {};
                statsLoaded = true;
            }
            return statsValue;
        };
        let chart = () => {
            if (!chartLoaded) {
                chartValue = selectedStateChartData;
                chartLoaded = true;
            }
            return chartValue;
        };
        let normalized = (value, total) => total > 0 ? value / total : 0;
        let chartNotes = () => chartPlayableNoteCount(chart());

        switch (type) {
        case 100:
        case 102:
        case 103:
            return 0;
        case 110:
        case 111:
        case 112:
        case 113:
        case 114:
        case 115:
        case 140:
        case 141:
        case 142:
        case 143:
        case 144:
        case 145:
        case 146:
        case 147:
            return barGraphValue(type - 100);
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            return difficultyGraphValue(type - 4);
        case 40:
            return normalized(stats().pg || 0, stats().totalJudgements || 0);
        case 41:
            return normalized(stats().gr || 0, stats().totalJudgements || 0);
        case 42:
            return normalized(stats().gd || 0, stats().totalJudgements || 0);
        case 43:
            return normalized(stats().bd || 0, stats().totalJudgements || 0);
        case 44:
            return normalized(stats().pr || 0, stats().totalJudgements || 0);
        case 45:
            return normalized(stats().maxCombo || 0, chartNotes());
        case 46:
        case 47:
            return stats() && stats().maxPoints > 0 ? stats().exscore / stats().maxPoints : 0;
        default:
            return 0;
        }
    }
}
