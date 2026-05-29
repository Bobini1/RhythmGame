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
    property var folderDistributionByKey: ({})
    property var pendingFolderLampByKey: ({})
    property var pendingFolderScoreCountsByKey: ({})
    property var pendingFolderDistributionByKey: ({})
    property int pendingFolderLampPublishCount: 0
    property bool folderLampPublishQueued: false
    property var chartGroupsByKey: ({})
    property var chartDifficultyByPath: ({})
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
    property int currentIndex: 0
    property int targetIndex: currentIndex
    property alias visualIndex: visualState.visualIndex
    property real targetVisualIndex: 0
    readonly property int listTopbarFixed: visualState.rawFixed
    readonly property int listCalculatedBarFixed: visualState.fixed
    property int oldBarFixed: 0
    property int nowBarFixed: 0
    property int pendingWheelSteps: 0
    property int selectedOffset: 0
    property double barMoveStartMs: 0
    property double barMoveEndMs: 0
    property bool componentReady: false
    property bool updatesActive: true
    property bool suppressVisualIndexPublish: false
    property int scrollDirection: 0
    property int listRevision: 0
    property int selectionRevision: 0
    property int focusRevision: 0
    property int scoreRevision: 0
    property int folderLampRequestRevision: 0
    property int playerStatsRevision: 0
    property bool suppressNextSelectionSound: false
    property bool scrollFixedPointDragging: false
    property string searchText: ""
    property var attachedTextByDirectory: ({})
    property var hasAttachedTextByDirectory: ({})
    property int difficultyFilter: 0
    property int keyFilter: 0
    property int sortMode: 0
    property int sortSourceFrameCount: 5
    property var barTitleTypes: []
    property int realItemCount: 0
    property int barRowCount: 0
    property int barCenter: 0
    readonly property int barLabelLongNote: 0
    readonly property int barLabelRandom: 1
    readonly property int barLabelMine: 2
    property alias visibleBarSlotOffset: visibleBarModel.slotOffset
    property alias visibleBarEntries: visibleBarModel.entries
    property alias visibleBarTextCells: visibleBarModel.cells
    property int refreshedFocusedIndex: -1
    property int refreshedFocusedScoreRevision: -1
    property int refreshedFocusedListRevision: -1
    property bool refreshedFocusedRankingMode: false
    property int lastSyncedCursorBaseIndex: -1
    property bool rankingMode: false
    property var rankingBaseItem: null
    property string rankingStatsMd5: ""
    property var rankingClearCounts: ({})
    property int rankingPlayerRank: 0
    property int rankingPlayerCount: 0
    property int rankingTotalPlayCount: 0
    property int rankingStatsRevision: 0
    property var rankingSavedItems: []
    property var rankingSavedFolderContents: []
    property int rankingSavedRealItemCount: 0
    property int rankingSavedCurrentIndex: 0
    property real rankingSavedVisualIndex: 0
    property int rankingSavedSelectedOffset: 0
    readonly property var emptyScoreList: []
    readonly property var emptyScoreOptionIds: []
    readonly property var emptyDifficultyCharts: [null, null, null, null, null, null]
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
    readonly property var emptyFolderDistributionObject: ({
        lamps: [],
        ranks: []
    })
    readonly property var emptyDifficultyState: ({
        key: "empty",
        charts: emptyDifficultyCharts,
        counts: emptyDifficultyNumbers,
        levels: emptyDifficultyNumbers,
        lamps: emptyDifficultyNumbers
    })
    property alias navigationController: nativeNavigation
    property var visualChartWrapper: null
    property string visualChartContentRevision: ""
    property string visualStageFileSource: ""
    property string visualBackBmpSource: ""
    property string visualBannerSource: ""
    function selectedState() : var {
        return selectedDetailState;
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

    function applySelectedScoreState(state: var) : void {
        let nextState = state || ({});
        selectedDetailState.apply(nextState);
        let chartData = nextState.chartData;
        let nextChartWrapper = nextState.chartWrapper;
        if (visualChartWrapper !== nextChartWrapper) {
            visualChartWrapper = nextChartWrapper;
        }
        let nextStageFileSource = chartAssetUrl(chartData, chartData ? chartData.stageFile : "");
        if (visualStageFileSource !== nextStageFileSource) {
            visualStageFileSource = nextStageFileSource;
        }
        let nextBackBmpSource = chartAssetUrl(chartData, chartData ? chartData.backBmp : "");
        if (visualBackBmpSource !== nextBackBmpSource) {
            visualBackBmpSource = nextBackBmpSource;
        }
        let nextBannerSource = chartAssetUrl(chartData, chartData ? chartData.banner : "");
        if (visualBannerSource !== nextBannerSource) {
            visualBannerSource = nextBannerSource;
        }
        let nextChartRevision = nextState.contentRevision !== undefined
            ? String(nextState.contentRevision || "")
            : chartContentRevisionForState(nextState);
        if (visualChartContentRevision !== nextChartRevision) {
            visualChartContentRevision = nextChartRevision;
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
    // beatoraja select button art is all/5/7/10/14/9/24/24D. 9K and 24K are not supported here.
    readonly property var beatorajaKeyFilterOrder: [0, 1, 2, 3, 4]
    readonly property var clearTypePriorities: ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"]

    readonly property int count: items.length
    readonly property int logicalCount: realItemCount > 0 ? realItemCount : count
    readonly property var current: logicalCount > 0 ? items[currentIndex] : null
    readonly property int focusedIndex: logicalCount > 0 ? currentIndex : 0
    readonly property var focusedItem: logicalCount > 0 ? items[focusedIndex] : null
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
        context: root
        visualState: visualState
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
            publishBarState(true);
        }
    }

    signal openedFolder()
    signal entryChangeSoundsRequested(int count)
    Connections {
        target: root.updatesActive ? visualState : null
        function onCursorBaseIndexChanged() : void {
            root.publishCursorBaseIndex(false);
        }

        function onAnimationFinished() : void {
            if (root.updatesActive) {
                root.publishVisualIndex(true);
            }
        }
    }

    function touchList() : void {
        listRevision += 1;
        refreshVisibleBarEntries(true);
    }

    function touchSelection() : void {
        nativeNavigation.touchSelection();
    }

    function flushFocusedStateRefresh() : var {
        return nativeNavigation.refreshFocusedState();
    }

    function refreshFocusedState() : var {
        return nativeNavigation.refreshFocusedState();
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

    function publishVisualFrame(force: var) : var {
        return !!force && updatesActive && !suppressVisualIndexPublish;
    }

    function publishBarState(force: var) : var {
        if (!updatesActive || suppressVisualIndexPublish) {
            return false;
        }

        publishVisualFrame(force);
        publishBaseIndex(force);
        return publishCursorBaseIndex(force);
    }

    function publishBaseIndex(force: var) : var {
        if (!updatesActive || suppressVisualIndexPublish) {
            return false;
        }
        if (force) {
            refreshVisibleBarEntries(!!force);
            return true;
        }
        return false;
    }

    function publishCursorBaseIndex(force: var) : var {
        if (!updatesActive || suppressVisualIndexPublish) {
            return false;
        }
        let cursorBaseIndex = visualState.cursorBaseIndex;
        if (force || lastSyncedCursorBaseIndex !== cursorBaseIndex) {
            lastSyncedCursorBaseIndex = cursorBaseIndex;
            return syncCurrentToVisual(cursorBaseIndex);
        }
        return false;
    }

    function publishVisualIndex(force: var) : var {
        if (!updatesActive) {
            return false;
        }
        let changed = publishVisualFrame(force);
        if (force) {
            publishBarState(false);
        }
        return changed;
    }

    function setVisualIndexRaw(index: var) : void {
        suppressVisualIndexPublish = true;
        visualState.jumpTo(index);
        suppressVisualIndexPublish = false;
    }

    function currentNormalizedVisualIndex() : var {
        return normalizedVisualIndex;
    }

    function nativeBarGraphValue(type: var) : var {
        return barGraphValue(type);
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
        publishBarState();
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
        return publishBarState();
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

    function syncCurrentToVisual(cursorBaseIndex: var) : var {
        return nativeNavigation.syncCurrentToVisual(cursorBaseIndex === undefined ? -1 : cursorBaseIndex);
    }

    function commitLogicalSelection(index: var) : var {
        return nativeNavigation.commitLogicalSelection(index);
    }

    function updateVisualIndex(now: var) : var {
        if (!updatesActive) {
            return;
        }
        visualState.advanceAnimation(now);
        if (pendingWheelSteps !== 0) {
            let steps = pendingWheelSteps;
            pendingWheelSteps = 0;
            applyLr2ScrollDelta(-steps, lr2WheelDuration, now, animatedTopbarFixed(now));
            return;
        }
        publishVisualIndex();
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
            publishBarState(true);
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

    function emptyFolderDistribution() {
        return {
            lamps: emptyFolderGraphLamps,
            ranks: emptyFolderGraphRanks
        };
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

    function folderDistribution(item) {
        let key = folderLampKey(item);
        return key && folderDistributionByKey[key] !== undefined
            ? folderDistributionByKey[key]
            : emptyFolderDistribution();
    }

    function normalizedFolderDistribution(distribution: var) : var {
        let lamps = distribution
            ? (distribution.lamps !== undefined ? distribution.lamps : distribution["lamps"])
            : null;
        let ranks = distribution
            ? (distribution.ranks !== undefined ? distribution.ranks : distribution["ranks"])
            : null;
        return {
            lamps: normalizedNumberArray(lamps, emptyFolderGraphLamps, 11),
            ranks: normalizedNumberArray(ranks, emptyFolderGraphRanks, 28)
        };
    }

    function barGraphDistribution(item: var) : var {
        return isFolderLikeForLamp(item)
            ? normalizedFolderDistribution(folderDistribution(item))
            : emptyFolderDistributionObject;
    }

    function folderScoreCountsFromSummaryCounts(counts) {
        if (!counts) {
            return emptyFolderScoreCounts;
        }
        if (counts.NOPLAY === undefined
                && counts.FAILED === undefined
                && counts.AEASY === undefined) {
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

    function refreshFolderLamps() {
        let db = Rg.profileList?.mainProfile?.scoreDb;
        if (!db) {
            clearFolderLampState();
            return;
        }

        let request = ++folderLampRequestRevision;
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
                if (request !== folderLampRequestRevision) {
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
        folderDistributionByKey = ({});
        selectItemModel.clearFolderSummaries();
    }

    function queueFolderLampSummaryResult(key, result) {
        let summary = result || ({});
        let counts = summary.counts !== undefined ? summary.counts : summary["counts"];
        let distribution = summary.distribution !== undefined
            ? summary.distribution
            : summary["distribution"];
        let lamp = summary.lamp !== undefined ? summary.lamp : summary["lamp"];
        pendingFolderLampByKey[key] = lamp || 0;
        pendingFolderScoreCountsByKey[key] = folderScoreCountsFromSummaryCounts(counts);
        pendingFolderDistributionByKey[key] = distribution || emptyFolderDistribution();
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
        folderDistributionByKey = Object.assign({}, folderDistributionByKey, pendingFolderDistributionByKey);
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
        let filter = effectiveKeyFilter();
        if (!isChart(item) || filter === 0) {
            return true;
        }
        let keymode = item.keymode || 0;
        if (beatorajaKeyFilterOrderActive()) {
            switch (filter) {
            case 1:
                return keymode === 5;
            case 2:
                return keymode === 7;
            case 3:
                return keymode === 10;
            case 4:
                return keymode === 14;
            default:
                return true;
            }
        }
        switch (filter) {
        case 1:
            return keymode === 5 || keymode === 7;
        case 2:
            return keymode === 7;
        case 3:
            return keymode === 5;
        case 4:
            return keymode === 10 || keymode === 14;
        case 5:
            return keymode === 14;
        case 6:
            return keymode === 10;
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

            let groupKey = chartDifficultyGroupKey(item);
            if (!groups[groupKey]) {
                groups[groupKey] = [];
                groupOrder.push(groupKey);
            }
            groups[groupKey].push(item);
        }

        let result = passthrough.slice();
        for (let groupKey of groupOrder) {
            let exact = [];
            let lower = [];
            let higher = [];
            let unknown = [];
            let lowerDifficulty = 0;
            let higherDifficulty = 0;
            for (let chart of groups[groupKey]) {
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

    function isSongLikeForSort(item: var) : var {
        return isRankingEntry(item) || isChart(item) || isEntry(item);
    }

    function compareByNameOnly(a: var, b: var) : var {
        return entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
    }

    function activeSortMode() : var {
        return sortSourceFrameCount >= 8 && sortMode === 0 ? 2 : sortMode;
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
        scoreRevision += 1;
        refreshFocusedState();
        if (activeSortUsesScores()) {
            sortOrFilterChanged(scoreLoadPreferredItem());
        } else {
            updateSelectItemModelData();
        }
    }

    function sortOrderForSourceCount(sourceCount: var) : var {
        return sourceCount >= 8 ? beatorajaSortOrder : lr2SortOrder;
    }

    function beatorajaKeyFilterOrderActive() : var {
        return sortSourceFrameCount >= 8;
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

    function exactKeymodeForFilter(filter: var, beatorajaOrder: var) : var {
        if (beatorajaOrder) {
            switch (filter) {
            case 1: return 5;
            case 2: return 7;
            case 3: return 10;
            case 4: return 14;
            default: return 0;
            }
        }
        switch (filter) {
        case 2: return 7;
        case 3: return 5;
        case 5: return 14;
        case 6: return 10;
        default: return 0;
        }
    }

    function keyFilterForExactKeymode(keymode: var, beatorajaOrder: var) : var {
        if (beatorajaOrder) {
            switch (keymode) {
            case 5: return 1;
            case 7: return 2;
            case 10: return 3;
            case 14: return 4;
            default: return 0;
            }
        }
        switch (keymode) {
        case 7: return 2;
        case 5: return 3;
        case 14: return 5;
        case 10: return 6;
        default: return 0;
        }
    }

    function remapKeyFilterOrder(filter: var, wasBeatoraja: var, isBeatoraja: var) : var {
        if (wasBeatoraja === isBeatoraja) {
            return filter;
        }

        let exactKeymode = exactKeymodeForFilter(filter, wasBeatoraja);
        return exactKeymode > 0
            ? keyFilterForExactKeymode(exactKeymode, isBeatoraja)
            : 0;
    }

    function effectiveKeyFilter() : var {
        let order = keyFilterOrder();
        return order.indexOf(keyFilter) >= 0 ? keyFilter : 0;
    }

    function keyFilterFrameForSourceCount(sourceCount: var) : var {
        let order = keyFilterOrder(sourceCount);
        return order.indexOf(keyFilter) >= 0 ? keyFilter : order[0];
    }

    function adjustKeyFilter(delta: var, sourceCount: var) : void {
        let order = keyFilterOrder(sourceCount);
        let frame = order.indexOf(keyFilter);
        if (frame < 0) {
            frame = 0;
        }
        frame = ((frame + delta) % order.length + order.length) % order.length;
        keyFilter = order[frame];
    }

    function resetSortSourceFrameCount() : void {
        setSortSourceFrameCount(5);
    }

    function setSortSourceFrameCount(sourceCount: var) : var {
        let wasBeatoraja = beatorajaKeyFilterOrderActive();
        let normalized = sourceCount >= 8 ? 8 : 5;
        if (sortSourceFrameCount === normalized) {
            return;
        }
        sortSourceFrameCount = normalized;
        keyFilter = remapKeyFilterOrder(keyFilter, wasBeatoraja, beatorajaKeyFilterOrderActive());
        if (folderContents.length > 0) {
            sortOrFilterChanged();
        }
    }

    function observeSortSourceFrameCount(sourceCount: var) : void {
        if (sourceCount >= 8 && sortSourceFrameCount < 8) {
            setSortSourceFrameCount(sourceCount);
        }
    }

    function sortFrameForSourceCount(sourceCount: var) : var {
        let order = sortOrderForSourceCount(sourceCount);
        let frame = order.indexOf(sortMode);
        if (frame >= 0) {
            return frame;
        }
        return sourceCount >= 8 ? 0 : 0;
    }

    function adjustSortMode(delta: var, sourceCount: var) : void {
        setSortSourceFrameCount(sourceCount);
        let order = sortOrderForSourceCount(sourceCount);
        let frame = order.indexOf(sortMode);
        if (frame < 0) {
            frame = 0;
        }
        frame = ((frame + delta) % order.length + order.length) % order.length;
        sortMode = order[frame];
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
        touchList();
        touchSelection();
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
        Rg.profileList.mainProfile.scoreDb.cancelPending();
        let folder = historyStack.length > 0 ? historyStack[historyStack.length - 1] : "";
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
            Rg.profileList.mainProfile.scoreDb.getScoresForMd5(md5s).then((result) => {
                scores = result.scores;
                handleScoresLoaded();
            });
            refreshPreviewFiles();
            refreshFolderLamps();
            return;
        }
        Rg.profileList.mainProfile.scoreDb.getScores(folder).then((result) => {
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
            playerStatsRevision += 1;
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
        touchList();
        touchSelection();
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
        touchList();
        touchSelection();
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
            touchSelection();
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
        commitLogicalSelection(normalized);
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
        touchSelection();
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
        return publishBarState();
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
        let focusTouched = commitLogicalSelection(nextIndex);
        if (!selectionTouched && !focusTouched && oldOffset !== selectedOffset) {
            touchSelection();
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
        let focusTouched = commitLogicalSelection(nextIndex);
        if (!selectionTouched && !focusTouched && (wasDragging || oldOffset !== selectedOffset)) {
            touchSelection();
        }
    }

    function animatedTopbarFixed(now: var) : var {
        return visualState.rawFixed;
    }

    function applyLr2ScrollDelta(entries: var, durationMs: var, now: var, currentFixed: var) : void {
        nativeNavigation.applyLr2ScrollDelta(entries,
                                             durationMs,
                                             now === undefined ? -1 : now,
                                             currentFixed === undefined ? -2147483648 : currentFixed);
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
        touchSelection();
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

    function barEntry(row: var, barCenter: var) : var {
        if (logicalCount === 0) {
            return null;
        }
        return items[normalizeIndex(visualState.baseIndex + row - barCenter)];
    }

    function refreshSelectItemModel() : void {
        selectItemModel.items = normalizedSelectModelItems(root.items);
    }

    function updateSelectItemModelData() : void {
        selectItemModel.updateItems(normalizedSelectModelItems(root.items));
    }

    function refreshVisibleBarEntries(force: var) : void {
        if (force) {
            refreshSelectItemModel();
        }
    }

    function visibleBarSlotForRow(row: var) : var {
        return visibleBarModel.slotForRow(row);
    }

    function visibleBarEntry(row: var, fallbackBarCenter: var) : var {
        return visibleBarModel.entryAtRow(row);
    }

    function barCellCore(entry: var) : var {
        return normalizedSelectModelItem(entry, 0);
    }

    function barTextCellForRow(row: var) : var {
        let slot = visibleBarModel.slotForRow(row);
        return slot >= 0 ? visibleBarModel.cellAtSlot(slot) : null;
    }

    function updateBarTextCellForRow(cell: var, row: var) : void {
    }

    function updateBarTextCell(cell: var, row: var, entry: var) : void {
    }

    function visibleBarTextCell(slot: var) : var {
        return visibleBarModel.cellAtSlot(slot);
    }

    function visibleBarDisplayName(row: var, fallbackBarCenter: var) : var {
        let slot = visibleBarSlotForRow(row);
        let cell = slot >= 0 ? visibleBarTextCell(slot) : null;
        if (cell && cell.row === row) {
            return cell.text || "";
        }
        return entryDisplayName(visibleBarEntry(row, fallbackBarCenter), true);
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
            return includeSubtitle && subtitle ? title + " " + subtitle : title;
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
            return item.title || "";
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

    function currentTableItem() : var {
        for (let i = historyStack.length - 1; i >= 0; --i) {
            if (isTable(historyStack[i])) {
                return historyStack[i];
            }
        }
        let item = selectedItem();
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
        let selected = selectedItem();
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
        if (isChart(item) || isEntry(item)) {
            return 0;
        }
        if (isTable(item) || isLevel(item)) {
            return 2;
        }
        if (isCourse(item)) {
            return 8;
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

    function entryTitleType(item: var) : var {
        // LR2 uses #SRC_BAR_TITLE index 0 for normal titles and 1 for the
        // recently-added flash variant; entry categories are handled by bodyType.
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

    function entryModelType(item: var) : var {
        if (isRankingEntry(item)) {
            return "ranking";
        }
        if (isChart(item)) {
            return "chart";
        }
        if (isEntry(item)) {
            return "entry";
        }
        if (isCourse(item)) {
            return "course";
        }
        if (isTable(item)) {
            return "table";
        }
        if (isLevel(item)) {
            return "level";
        }
        if (typeof item === "string") {
            return "folder";
        }
        return "unknown";
    }

    function normalizedSelectModelItem(item: var, fallbackIndex: var) : var {
        let summary = scoreLampRankForItem(item);
        let distribution = barGraphDistribution(item);
        return {
            rawItem: item,
            key: entrySelectionKey(item, fallbackIndex),
            folderKey: isFolderLikeForLamp(item) ? folderLampKey(item) : "",
            displayText: entryDisplayName(item, true),
            titleType: entryTitleType(item),
            bodyType: entryBodyType(item),
            title: entryMainTitle(item),
            subtitle: entrySubtitle(item),
            artist: entryArtist(item),
            subartist: entrySubartist(item),
            genre: entryGenre(item),
            path: item && item.path ? item.path : "",
            md5: item && item.md5 ? item.md5 : "",
            sha256: item && item.sha256 ? item.sha256 : "",
            directory: item && item.chartDirectory ? item.chartDirectory : (item && item.directory ? item.directory : ""),
            playLevel: entryPlayLevel(item),
            difficulty: entryDifficulty(item),
            keymode: item && item.keymode ? item.keymode : 0,
            lamp: isRankingEntry(item) ? clearTypeLamp(item.bestClearType) : summary.lamp,
            scoreRank: isRankingEntry(item) ? rankingEntryRank(item) : summary.rank,
            labelMask: entryLabelMask(item),
            folderLamp: isFolderLikeForLamp(item) ? entryLamp(item) : 0,
            folderDistribution: distribution,
            folderGraphLamps: distribution.lamps,
            folderGraphRanks: distribution.ranks,
            type: entryModelType(item),
            __lr2RankingEntry: isRankingEntry(item)
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
        let filter = effectiveKeyFilter();
        if (beatorajaKeyFilterOrderActive()) {
            switch (filter) {
            case 1: return "5KEYS";
            case 2: return "7KEYS";
            case 3: return "10KEYS";
            case 4: return "14KEYS";
            default: return "ALL";
            }
        }
        switch (filter) {
        case 1: return "SINGLE";
        case 2: return "7KEYS";
        case 3: return "5KEYS";
        case 4: return "DOUBLE";
        case 5: return "14KEYS";
        case 6: return "10KEYS";
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
                || ((item.title || "") + "\n" + (item.subtitle || "") + "\n"
                    + (item.artist || "") + "\n" + (item.playLevel || 0) + "\n"
                    + (item.keymode || 0) + "\n" + (item.difficulty || 0))
                || ("index:" + fallbackIndex));
        }
        if (isEntry(item)) {
            return "entry:" + (item.md5
                || item.path
                || ((item.title || "") + "\n" + (item.subtitle || "") + "\n"
                    + (item.artist || "") + "\n" + (item.level || "") + "\n"
                    + (item.difficulty || 0))
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

    function entryScores(item: var) : var {
        let id = entryIdentifier(item);
        if (!id) {
            return emptyScoreList;
        }
        return scores[id]
            || scores[String(id).toUpperCase()]
            || scores[String(id).toLowerCase()]
            || emptyScoreList;
    }

    function clearTypePriority(clear: var) : var {
        switch (clear || "NOPLAY") {
        case "FAILED":
            return 1;
        case "AEASY":
            return 2;
        case "EASY":
            return 3;
        case "NORMAL":
            return 4;
        case "HARD":
            return 5;
        case "EXHARD":
            return 6;
        case "FC":
            return 7;
        case "PERFECT":
            return 8;
        case "MAX":
            return 9;
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
        let state = selectedState();
        if ((item === state.item || item === state.chartData)
                && state.key
                && state.scoreRevision === scoreRevision) {
            return state.summary || buildScoreSummary(state.scoreList || emptyScoreList);
        }
        return buildScoreSummary(entryScores(item));
    }

    function scoreLampRankForItem(item: var) : var {
        return scoreSummaryForItem(item);
    }

    function clearTypeOf(score: var) : var {
        return score?.result?.clearType || "NOPLAY";
    }

    function clearTypeLamp(clear: var) : var {
        switch (clear || "NOPLAY") {
        case "FAILED":
            return 1;
        case "AEASY":
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
        if (!usesExtendedBarLampVariants(variants)) {
            return clearTypeLamp(clear);
        }
        switch (clear || "NOPLAY") {
        case "FAILED":
            return 1;
        case "AEASY":
            return hasBarLampVariant(variants, 9) ? 9 : 2;
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

    function entryBarLamp(item: var, variants: var) : var {
        if (isFolderLikeForLamp(item)) {
            return entryLamp(item);
        }
        return clearTypeBarLamp(entryClearType(item), variants);
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
        let state = selectedState();
        let summary = state.summary || buildScoreSummary(state.scoreList || emptyScoreList);
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
        if (keymode === 9) {
            return Math.floor((100000 * stats.pg
                               + 70000 * stats.gr
                               + 40000 * stats.gd) / totalNotes);
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
        let key = folderLampKey(selectedState().item);
        return key && folderScoreCountsByKey[key] !== undefined
            ? folderScoreCountsByKey[key]
            : null;
    }

    function currentFolderScoreCounts() {
        return currentFolderScoreCountsOrNull() || emptyFolderScoreCounts;
    }

    function appendScoreClearOptionIds(clearType: var, ids: var) : void {
        switch (clearType || "NOPLAY") {
        case "AEASY":
        case "LIGHTASSIST":
        case "LIGHT_ASSIST":
            ids.push(124);
            ids.push(1100);
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
            ids.push(1102);
            break;
        case "FC":
            ids.push(105);
            break;
        case "PERFECT":
            ids.push(105);
            ids.push(1103);
            break;
        case "MAX":
            ids.push(105);
            ids.push(1104);
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
        let state = selectedState();
        if ((item === state.item || item === state.chartData)
                && state.scoreRevision === scoreRevision) {
            if (!state.scoreOptionIds) {
                state.scoreOptionIds = scoreOptionIdsFromSummary(
                    state.summary || buildScoreSummary(state.scoreList || emptyScoreList));
            }
            return state.scoreOptionIds;
        }
        return scoreOptionIdsFromSummary(scoreSummaryForItem(item));
    }

    function scoreOptionIdsFromList(scoreList: var) : var {
        return scoreOptionIdsFromSummary(buildScoreSummary(scoreList));
    }

    function scoreOptionIdsFromSummary(summary: var) : var {
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
        return scoreLampRankForItem(item).lamp;
    }

    function entryClearType(item: var) : var {
        if (isRankingEntry(item)) {
            return item.bestClearType || "NOPLAY";
        }
        if (isFolderLikeForLamp(item)) {
            return "NOPLAY";
        }
        return scoreLampRankForItem(item).clearType;
    }

    function beatorajaClearOptionForClearType(clearType: var) : var {
        switch (clearType || "NOPLAY") {
        case "FAILED":
            return 101;
        case "AEASY":
            return 1100;
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
        return scoreLampRankForItem(item).rank;
    }

    function entryScoreRate(item: var) : var {
        return scoreLampRankForItem(item).scoreRate;
    }

    function chartDataForItem(item: var) : var {
        if (rankingMode && rankingBaseItem) {
            return rankingBaseItem;
        }
        return (isChart(item) || isEntry(item)) ? item : null;
    }

    function selectedChartData() : var {
        return selectedState().chartData;
    }

    function selectedItem() : var {
        return selectedState().item;
    }

    function chartWrapperForData(chartData: var) : var {
        return chartData ? { chartData: chartData } : null;
    }

    function chartHistogramRevision(chartData: var) : var {
        let histogram = chartData && chartData.histogramData ? chartData.histogramData : null;
        if (!histogram) {
            return "";
        }
        let parts = [];
        for (let i = 0; i < 6; ++i) {
            let series = histogram[i] || [];
            parts.push(series.length || 0);
        }
        return parts.join(":");
    }

    function chartContentRevisionForData(stateKey: var, chartData: var) : var {
        if (!chartData) {
            return String(stateKey || "");
        }
        return String(stateKey || "")
            + ":" + String(chartData.md5 || "")
            + ":" + String(chartData.length || 0)
            + ":" + String(chartData.normalNoteCount || 0)
            + ":" + String(chartData.scratchCount || 0)
            + ":" + String(chartData.lnCount || 0)
            + ":" + String(chartData.bssCount || 0)
            + ":" + String(chartData.mineCount || 0)
            + ":" + chartHistogramRevision(chartData);
    }

    function chartContentRevisionForState(state: var) : var {
        if (!state) {
            return "";
        }
        let chartData = state.chartWrapper && state.chartWrapper.chartData
            ? state.chartWrapper.chartData
            : state.chartData;
        return chartContentRevisionForData(state.key, chartData);
    }

    function selectedChartContentRevision() : var {
        return visualChartContentRevision;
    }

    function refreshSelectedScoreState() : var {
        let item = focusedItem;
        let nextChartData = chartDataForItem(item);
        let targetItem = isRankingEntry(item) ? item : (nextChartData || item);
        let stateKey = entrySelectionKey(item, focusedIndex)
            + "\n" + entrySelectionKey(targetItem, focusedIndex)
            + "\n" + (rankingMode ? "ranking" : "select");
        let difficultyState = difficultyStateForChart(nextChartData);
        let changed = selectedDetailState.refresh(stateKey,
                                                  scoreRevision,
                                                  listRevision,
                                                  item,
                                                  nextChartData,
                                                  entryScores(targetItem),
                                                  difficultyState.charts || emptyDifficultyCharts,
                                                  difficultyState.counts || emptyDifficultyNumbers,
                                                  difficultyState.levels || emptyDifficultyNumbers,
                                                  difficultyState.lamps || emptyDifficultyNumbers);
        if (changed) {
            let chartData = selectedDetailState.chartData;
            let nextChartWrapper = selectedDetailState.chartWrapper;
            if (visualChartWrapper !== nextChartWrapper) {
                visualChartWrapper = nextChartWrapper;
            }
            let nextStageFileSource = chartAssetUrl(chartData, chartData ? chartData.stageFile : "");
            if (visualStageFileSource !== nextStageFileSource) {
                visualStageFileSource = nextStageFileSource;
            }
            let nextBackBmpSource = chartAssetUrl(chartData, chartData ? chartData.backBmp : "");
            if (visualBackBmpSource !== nextBackBmpSource) {
                visualBackBmpSource = nextBackBmpSource;
            }
            let nextBannerSource = chartAssetUrl(chartData, chartData ? chartData.banner : "");
            if (visualBannerSource !== nextBannerSource) {
                visualBannerSource = nextBannerSource;
            }
            let nextChartRevision = chartContentRevisionForState(selectedDetailState);
            if (visualChartContentRevision !== nextChartRevision) {
                visualChartContentRevision = nextChartRevision;
            }
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
        let chart = selectedChartData();
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

    function setRankingStats(md5: var, clearCounts: var, playerCount: var, totalPlayCount: var, playerRank: var) : var {
        let nextMd5 = normalizedMd5(md5);
        let nextClearCounts = clearCounts || {};
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
        rankingStatsRevision += 1;
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
        let chart = selectedChartData();
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
        rankingSavedVisualIndex = currentNormalizedVisualIndex();
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
        touchList();
        touchSelection();
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
        touchList();
        touchSelection();
        return true;
    }

    function chartGroupKey(chart: var) : var {
        if (!chart) {
            return "";
        }
        // LR2 groups difficulty buttons by the song folder, not by the
        // currently selected chart row.
        return chart.chartDirectory || chart.directory || "";
    }

    function chartDifficultyGroupKey(chart: var) : var {
        if (!chart) {
            return "";
        }
        // OpenLR2 computes the side difficulty bars per song folder and
        // keymode. Mixing 5K/7K variants here makes the bars drift out of sync.
        return chartGroupKey(chart) + "\n" + (chart.keymode || 0);
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

            let groupKey = chartDifficultyGroupKey(item);
            if (!groups[groupKey]) {
                groups[groupKey] = [];
            }
            groups[groupKey].push(item);
        }

        for (let group of Object.values(groups)) {
            Object.assign(difficulties, inferGroupDifficulties(group));
        }

        chartGroupsByKey = groups;
        chartDifficultyByPath = difficulties;
    }

    function chartsForCurrentSong() : var {
        return chartsForSong(selectedChartData());
    }

    function chartsForSong(chart: var) : var {
        if (!chart) {
            return [];
        }

        let groupKey = chartDifficultyGroupKey(chart);
        let result = chartGroupsByKey[groupKey] || [];
        if (result.length === 0) {
            result.push(chart);
        }
        return result;
    }

    function buildSelectedDifficultyState(chart: var) : var {
        let charts = chartsForSong(chart);
        let byDiff = [null, null, null, null, null, null];
        let counts = [0, 0, 0, 0, 0, 0];
        let levels = [0, 0, 0, 0, 0, 0];
        let lamps = [0, 0, 0, 0, 0, 0];
        for (let candidate of charts) {
            let diff = entryDifficulty(candidate);
            if (diff < 1 || diff > 5) {
                continue;
            }
            counts[diff] += 1;
            if (!byDiff[diff] || (chart && sameEntry(candidate, chart))) {
                byDiff[diff] = candidate;
            }
        }
        for (let diff = 1; diff <= 5; ++diff) {
            let candidate = byDiff[diff];
            if (!candidate) {
                continue;
            }
            levels[diff] = candidate.playLevel || 0;
            lamps[diff] = entryLamp(candidate);
        }
        return {
            charts: byDiff,
            counts: counts,
            levels: levels,
            lamps: lamps
        };
    }

    function difficultyStateForChart(chart: var) : var {
        let key = chart
            ? chartDifficultyGroupKey(chart) + "\n" + entrySelectionKey(chart, 0)
            : "empty";

        let state = buildSelectedDifficultyState(chart);
        state.key = key;
        return state;
    }

    function chartForDifficulty(diff: var) : var {
        let difficultyState = selectedState().difficultyState || emptyDifficultyState;
        let charts = difficultyState.charts || emptyDifficultyCharts;
        return diff >= 1 && diff <= 5 ? charts[diff] : null;
    }

    function nextChartForDifficulty(diff: var) : var {
        let currentChart = selectedChartData();
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

        let currentChart = selectedChartData();
        let target = currentChart && entryDifficulty(currentChart) === diff
            ? nextChartForDifficulty(diff)
            : chartForDifficulty(diff);
        difficultyFilter = diff;
        sortOrFilterChanged(target || currentChart);
    }

    function difficultyCount(diff: var) : var {
        let difficultyState = selectedState().difficultyState || emptyDifficultyState;
        let counts = difficultyState.counts || emptyDifficultyNumbers;
        return diff >= 1 && diff <= 5 ? counts[diff] || 0 : 0;
    }

    function difficultyPlayLevel(diff: var) : var {
        let difficultyState = selectedState().difficultyState || emptyDifficultyState;
        let levels = difficultyState.levels || emptyDifficultyNumbers;
        return diff >= 1 && diff <= 5 ? levels[diff] || 0 : 0;
    }

    function levelBarFlashThreshold() : var {
        let chart = selectedChartData();
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
        let difficultyState = selectedState().difficultyState || emptyDifficultyState;
        let lamps = difficultyState.lamps || emptyDifficultyNumbers;
        return diff >= 1 && diff <= 5 ? lamps[diff] || 0 : 0;
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

    function judgeOption(chart: var) : var {
        let rank = chart ? (chart.rank || 75) : 75;
        if (rank <= 25) return 180;
        if (rank <= 50) return 181;
        if (rank <= 75) return 182;
        return 183;
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

    function selectedChartWrapper() : var {
        return visualChartWrapper || selectedState().chartWrapper;
    }

    function selectedPreviewSource() : var {
        let chartData = selectedChartData();
        return chartData ? previewFiles[chartData.chartDirectory] : "";
    }

    function numberValue(num: var) : var {
        return computeNumberValue(num);
    }

    function computeNumberValue(num: var) : var {
        let state = selectedState();
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
            return chart() && (chart().maxBpm || chart().mainBpm)
                ? Math.round(chart().maxBpm || chart().mainBpm)
                : -1;
        case 91:
        case 291:
            return chart() && (chart().minBpm || chart().mainBpm)
                ? Math.round(chart().minBpm || chart().mainBpm)
                : -1;
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
            return chart() ? (chart().normalNoteCount || 0) : -1;
        case 351:
            return chart() ? (chart().lnCount || 0) : -1;
        case 352:
            return chart() ? (chart().scratchCount || 0) : -1;
        case 353:
            return chart() ? (chart().bssCount || 0) : -1;
        case 354:
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
        return !!chartForDifficulty(diff);
    }

    function barGraphValue(type: var) : var {
        if (type === 101) {
            return currentNormalizedVisualIndex();
        }
        return computeBarGraphValue(type);
    }

    function computeBarGraphValue(type: var) : var {
        let state = selectedState();
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
                chartValue = selectedChartData();
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
