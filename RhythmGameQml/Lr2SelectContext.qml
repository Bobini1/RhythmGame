pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

Item {
    id: root

    property var items: []
    property var folderContents: []
    property var historyStack: []
    property var scores: ({})
    property var folderLampByKey: ({})
    property var folderScoreCountsByKey: ({})
    property var chartGroupCache: ({})
    property var chartDifficultyCache: ({})
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
    property real visualIndex: 0
    property real targetVisualIndex: 0
    property int listTopbarFixed: 0
    property int listCalculatedBarFixed: 0
    property int oldBarFixed: 0
    property int nowBarFixed: 0
    property int pendingWheelSteps: 0
    property int selectedOffset: 0
    property bool deferVisualSelectionSync: false
    property real animationStartVisualIndex: 0
    property double barMoveStartMs: 0
    property double barMoveEndMs: 0
    property int scrollDirection: 0
    property int revision: 0
    property int listRevision: 0
    property int selectionRevision: 0
    property int scoreRevision: 0
    property int folderLampRevision: 0
    property int folderLampRequestRevision: 0
    property bool scrollFixedPointDragging: false
    property bool scrollingText: false
    property string searchText: ""
    property var attachedTextCache: ({})
    property int difficultyFilter: 0
    property int keyFilter: 0
    property int sortMode: 0
    property int sortSourceFrameCount: 5
    property int realItemCount: 0
    property int barRowCount: 0
    property int barCenter: 0
    property int barEntriesRevision: 0
    property int barTextCellsRevision: 0
    property var visibleBarEntries: []
    property var visibleBarTextCells: []
    property var selectedScoreStateCache: ({})
    property int selectedScoreStateCacheRevision: -1
    property int cachedVisibleBarBaseIndex: -1
    property int cachedVisibleBarListRevision: -1
    property int cachedVisibleBarRowCount: -1
    property int cachedVisibleBarCenter: -1
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
    property var cachedSelectedChartData: null
    readonly property var emptyScoreList: []
    readonly property var emptyScoreOptionIds: []
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
    property var selectedScoreList: []
    property var selectedBestStats: null
    property var selectedScoreCounts: ({
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
    property var selectedScoreOptionIds: []
    readonly property int lr2SpeedFirst: 300
    readonly property int lr2SpeedNext: 70
    readonly property int lr2WheelDuration: 200
    readonly property int lr2ClickDuration: 200
    readonly property int lr2ScrollUp: 1
    readonly property int lr2ScrollDown: 2
    readonly property var lr2SortOrder: [0, 1, 2, 3, 4]
    readonly property var beatorajaSortOrder: [2, 5, 6, 7, 1, 3, 4, 8]
    readonly property var clearTypePriorities: ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"]

    readonly property int count: items.length
    readonly property int logicalCount: realItemCount > 0 ? realItemCount : count
    readonly property var current: logicalCount > 0 ? items[currentIndex] : null
    readonly property int visualDirection: scrollDirection === lr2ScrollDown ? 1 : (scrollDirection === lr2ScrollUp ? -1 : 0)
    property bool visualMoveActive: false
    property real normalizedVisualIndex: 0
    property int visualBaseIndex: 0
    property real scrollOffset: 0

    signal openedFolder()
    signal barTextCellsInvalidated()
    onBarRowCountChanged: refreshVisibleBarEntries(true)
    onBarCenterChanged: refreshVisibleBarEntries(true)

    function touch() {
        revision += 1;
    }

    function touchList() {
        listRevision += 1;
        refreshVisibleBarEntries(true);
        touch();
    }

    function touchSelection() {
        refreshSelectedScoreState();
        selectionRevision += 1;
        touch();
    }

    function wrapBarFixed(value) {
        let span = logicalCount * 1000;
        if (span <= 0) {
            return 0;
        }
        return ((value % span) + span) % span;
    }

    function clampBarFixed(value) {
        let maxFixed = Math.max(0, logicalCount * 1000 - 1);
        return Math.max(0, Math.min(maxFixed, Math.round(value)));
    }

    function lr2ChangeValueByTime(from, to, start, end, now) {
        if (end <= start || now >= end) {
            return to;
        }
        if (now <= start) {
            return from;
        }
        return Math.trunc(from + (to - from) * ((now - start) / (end - start)));
    }

    function publishBarState() {
        let calculatedFixed = wrapBarFixed(listTopbarFixed);
        if (listCalculatedBarFixed !== calculatedFixed) {
            listCalculatedBarFixed = calculatedFixed;
        }

        let normalized = logicalCount > 0 ? calculatedFixed / 1000.0 : 0;
        let baseIndex = Math.floor(normalized);
        let offset = normalized - baseIndex;
        let baseChanged = visualBaseIndex !== baseIndex;

        deferVisualSelectionSync = true;
        if (normalizedVisualIndex !== normalized) {
            normalizedVisualIndex = normalized;
        }
        if (baseChanged) {
            visualBaseIndex = baseIndex;
        }
        if (scrollOffset !== offset) {
            scrollOffset = offset;
        }
        let moving = Math.abs(nowBarFixed - listTopbarFixed) > 0;
        if (visualMoveActive !== moving) {
            visualMoveActive = moving;
        }
        if (baseChanged
                || cachedVisibleBarBaseIndex < 0
                || cachedVisibleBarListRevision !== listRevision
                || cachedVisibleBarRowCount !== barRowCount
                || cachedVisibleBarCenter !== barCenter) {
            refreshVisibleBarEntries(false);
        }
        deferVisualSelectionSync = false;
        return syncCurrentToVisual();
    }

    function setVisualIndexImmediate(index) {
        let fixed = Math.round(index * 1000);
        listTopbarFixed = fixed;
        oldBarFixed = fixed;
        nowBarFixed = fixed;
        visualIndex = index;
        targetVisualIndex = index;
        animationStartVisualIndex = index;
        barMoveStartMs = 0;
        barMoveEndMs = 0;
        scrollDirection = 0;
        publishBarState();
    }

    function beginVisualMove(durationMs, now) {
        if (now === undefined) {
            now = Date.now();
        }
        barMoveStartMs = now;
        barMoveEndMs = now + Math.max(1, durationMs);
        visualIndex = listTopbarFixed / 1000.0;
        targetVisualIndex = nowBarFixed / 1000.0;
        animationStartVisualIndex = oldBarFixed / 1000.0;
        publishBarState();
    }

    function normalizedVisualIndexFor(index) {
        return normalizedVisualIndexForCount(index, logicalCount);
    }

    function normalizedVisualIndexForCount(index, itemCount) {
        if (itemCount <= 0) {
            return 0;
        }
        return ((index % itemCount) + itemCount) % itemCount;
    }

    function baseIndexForVisual(index) {
        return Math.floor(normalizedVisualIndexFor(index));
    }

    function visualRemainderFor(index) {
        let normalized = normalizedVisualIndexFor(index);
        return normalized - Math.floor(normalized);
    }

    function cursorBaseIndexForVisual(index) {
        return cursorBaseIndexForFixed(Math.floor(normalizedVisualIndexFor(index) * 1000));
    }

    function cursorBaseIndexForFixed(fixed) {
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

    function syncCurrentToVisual() {
        if (logicalCount === 0) {
            return false;
        }
        let nextIndex = normalizeIndex(cursorBaseIndexForFixed(listCalculatedBarFixed) + selectedOffset);
        if (currentIndex === nextIndex) {
            return false;
        }
        currentIndex = nextIndex;
        targetIndex = nextIndex;
        scrollingText = false;
        scrollingTextTimer.restart();
        touchSelection();
        return true;
    }

    function updateVisualIndex(now) {
        if (pendingWheelSteps !== 0) {
            let steps = pendingWheelSteps;
            pendingWheelSteps = 0;
            applyLr2ScrollDelta(-steps, lr2WheelDuration, now);
            return;
        }
        let nextTopbarFixed = animatedTopbarFixed(now);
        let moving = Math.abs(nowBarFixed - nextTopbarFixed) > 0;
        if (listTopbarFixed === nextTopbarFixed && visualMoveActive === moving) {
            return;
        }
        listTopbarFixed = nextTopbarFixed;
        publishBarState();
    }

    function queueWheelSteps(steps) {
        pendingWheelSteps += steps;
    }

    function nearestVisualIndex(index, anchor) {
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

    function isChart(item) {
        return item instanceof ChartData;
    }

    function isEntry(item) {
        return item instanceof entry;
    }

    function isCourse(item) {
        return item instanceof course;
    }

    function isLevel(item) {
        return item instanceof level;
    }

    function isTable(item) {
        return item instanceof table;
    }

    function isLoadedTable(item) {
        return isTable(item) && item.status === table.Loaded;
    }

    function isRankingEntry(item) {
        return !!item && item.__lr2RankingEntry === true;
    }

    function classCoursesForTable(tableItem) {
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

    function folderLampFromScores(result) {
        if (!result) {
            return 0;
        }
        if (result instanceof tableQueryResult) {
            result = result.scores;
        }

        let lamp = 5;
        let seen = false;
        if ((result.unplayed || 0) > 0) {
            return 0;
        }

        for (let scoreList of Object.values(result.scores || {})) {
            seen = true;
            lamp = Math.min(lamp, clearTypeLamp(getClearType(scoreList)));
        }
        return seen ? lamp : 0;
    }

    function folderScoreCountsFromScores(result) {
        if (!result) {
            return emptyFolderScoreCounts;
        }
        if (result instanceof tableQueryResult) {
            result = result.scores;
        }

        let counts = {
            total: Math.max(0, result.unplayed || 0),
            play: 0,
            clear: 0,
            fail: 0,
            noplay: Math.max(0, result.unplayed || 0),
            assist: 0,
            lightAssist: 0,
            easy: 0,
            normal: 0,
            hard: 0,
            exhard: 0,
            fc: 0,
            perfect: 0,
            max: 0
        };
        for (let scoreList of Object.values(result.scores || {})) {
            ++counts.total;
            if (!scoreList || scoreList.length === 0) {
                ++counts.noplay;
                continue;
            }
            ++counts.play;
            let clearType = getClearType(scoreList);
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
        }
        return counts;
    }

    function refreshFolderLamps() {
        let db = Rg.profileList?.mainProfile?.scoreDb;
        if (!db) {
            folderLampByKey = ({});
            folderScoreCountsByKey = ({});
            folderLampRevision += 1;
            return;
        }

        let request = ++folderLampRequestRevision;
        folderLampByKey = ({});
        folderScoreCountsByKey = ({});
        folderLampRevision += 1;

        for (let item of folderContents) {
            if (!isFolderLikeForLamp(item)) {
                continue;
            }

            let key = folderLampKey(item);
            if (!key) {
                continue;
            }

            db.getScores(item).then((result) => {
                if (request !== folderLampRequestRevision) {
                    return;
                }
                let nextLamps = Object.assign({}, folderLampByKey);
                nextLamps[key] = folderLampFromScores(result);
                folderLampByKey = nextLamps;
                let nextCounts = Object.assign({}, folderScoreCountsByKey);
                nextCounts[key] = folderScoreCountsFromScores(result);
                folderScoreCountsByKey = nextCounts;
                folderLampRevision += 1;
                touch();
            });
        }
    }

    Component.onDestruction: {
        Rg.profileList?.mainProfile?.scoreDb?.cancelPending();
    }

    function addToMinimumCount(input) {
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

    function keyFilterMatches(item) {
        if (!isChart(item) || keyFilter === 0) {
            return true;
        }
        let keymode = item.keymode || 0;
        switch (keyFilter) {
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

    function chartFilterMatches(item) {
        return keyFilterMatches(item);
    }

    function difficultyFilteredCharts(input) {
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

    function compareByTitle(a, b) {
        let titleDiff = compareByNameOnly(a, b);
        if (titleDiff !== 0) {
            return titleDiff;
        }
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return 0;
        }
        return compareByDifficulty(a, b);
    }

    function compareByArtist(a, b) {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let artistDiff = entryArtist(a).localeCompare(entryArtist(b));
        if (artistDiff !== 0) {
            return artistDiff;
        }
        return compareByTitle(a, b);
    }

    function compareNumberWithMissing(aValue, bValue) {
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

    function entryMaxBpmForSort(item) {
        if (!isChart(item) && !isEntry(item)) {
            return null;
        }
        let bpm = item.maxBpm || item.mainBpm || item.minBpm || 0;
        return bpm > 0 ? bpm : null;
    }

    function entryLengthForSort(item) {
        if (!isChart(item) && !isEntry(item)) {
            return null;
        }
        return (item.length || 0) > 0 ? item.length : null;
    }

    function entryScoreRateForSort(item) {
        let best = bestScoreByPoints(cachedEntryScores(item));
        if (!best || !best.result || best.result.maxPoints <= 0) {
            return null;
        }
        return best.result.points / best.result.maxPoints;
    }

    function entryMissCountForSort(item) {
        let scoreList = cachedEntryScores(item);
        if (!scoreList || scoreList.length === 0) {
            return null;
        }
        let counts = scoreCounts(scoreList);
        return counts.minBadPoor >= 0 ? counts.minBadPoor : null;
    }

    function compareByBpm(a, b) {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryMaxBpmForSort(a), entryMaxBpmForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByTitle(a, b);
    }

    function compareByLength(a, b) {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryLengthForSort(a), entryLengthForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByTitle(a, b);
    }

    function compareByDifficulty(a, b) {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let levelDiff = entryPlayLevel(a) - entryPlayLevel(b);
        if (levelDiff !== 0) {
            return levelDiff;
        }
        return compareByNameOnly(a, b);
    }

    function compareByClear(a, b) {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let aScores = cachedEntryScores(a);
        let bScores = cachedEntryScores(b);
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

    function compareByScore(a, b) {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryScoreRateForSort(a), entryScoreRateForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByDifficulty(a, b);
    }

    function compareByMissCount(a, b) {
        if (!isSongLikeForSort(a) || !isSongLikeForSort(b)) {
            return compareByNameOnly(a, b);
        }
        let diff = compareNumberWithMissing(entryMissCountForSort(a), entryMissCountForSort(b));
        if (diff !== 0) {
            return diff;
        }
        return compareByDifficulty(a, b);
    }

    function isSongLikeForSort(item) {
        return isRankingEntry(item) || isChart(item) || isEntry(item);
    }

    function compareByNameOnly(a, b) {
        return entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
    }

    function activeSortMode() {
        return sortSourceFrameCount >= 8 && sortMode === 0 ? 2 : sortMode;
    }

    function compareCharts(a, b) {
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

    function activeSortUsesScores() {
        switch (activeSortMode()) {
        case 3: // clear lamp
        case 4: // score rate
        case 8: // miss count
            return true;
        default:
            return false;
        }
    }

    function scoreLoadPreferredItem() {
        if (currentIndex !== 0
                || targetIndex !== 0
                || selectedOffset !== 0
                || pendingWheelSteps !== 0
                || visualMoveActive) {
            return current;
        }
        return null;
    }

    function handleScoresLoaded() {
        refreshSelectedScoreState();
        scoreRevision += 1;
        if (activeSortUsesScores()) {
            sortOrFilterChanged(scoreLoadPreferredItem());
        }
        touch();
    }

    function sortOrderForSourceCount(sourceCount) {
        return sourceCount >= 8 ? beatorajaSortOrder : lr2SortOrder;
    }

    function resetSortSourceFrameCount() {
        setSortSourceFrameCount(5);
    }

    function setSortSourceFrameCount(sourceCount) {
        let normalized = sourceCount >= 8 ? 8 : 5;
        if (sortSourceFrameCount === normalized) {
            return;
        }
        sortSourceFrameCount = normalized;
        if (folderContents.length > 0) {
            sortOrFilterChanged();
        }
    }

    function observeSortSourceFrameCount(sourceCount) {
        if (sourceCount >= 8 && sortSourceFrameCount < 8) {
            setSortSourceFrameCount(sourceCount);
        }
    }

    function sortFrameForSourceCount(sourceCount) {
        let order = sortOrderForSourceCount(sourceCount);
        let frame = order.indexOf(sortMode);
        if (frame >= 0) {
            return frame;
        }
        return sourceCount >= 8 ? 0 : 0;
    }

    function adjustSortMode(delta, sourceCount) {
        setSortSourceFrameCount(sourceCount);
        let order = sortOrderForSourceCount(sourceCount);
        let frame = order.indexOf(sortMode);
        if (frame < 0) {
            frame = 0;
        }
        frame = ((frame + delta) % order.length + order.length) % order.length;
        sortMode = order[frame];
    }

    function sortFilter(input) {
        let entries = [];
        for (let item of input) {
            if (isChart(item) || isEntry(item)) {
                if (!chartFilterMatches(item)) {
                    continue;
                }
            }
            entries.push(item);
        }
        entries = difficultyFilteredCharts(entries);
        if (activeSortMode() !== 0) {
            entries.sort(compareCharts);
        }
        let result = entries;
        if (result.length === 0) {
            result.push(null);
        }
        return result;
    }

    function sortOrFilterChanged(preferredItem) {
        if (rankingMode) {
            hideRanking();
        }
        if (!folderContents.length) {
            return;
        }
        let old = preferredItem === undefined ? current : preferredItem;
        let sortedFiltered = sortFilter(folderContents);
        realItemCount = sortedFiltered.length;
        addToMinimumCount(sortedFiltered);
        items = sortedFiltered;
        let currentIdx = sortedFiltered.findIndex((item) => sameEntry(item, old));
        currentIndex = currentIdx >= 0 ? currentIdx : 0;
        targetIndex = currentIndex;
        selectedOffset = 0;
        setVisualIndexImmediate(currentIndex);
        scrollingText = false;
        scrollingTextTimer.restart();
        touchList();
        touchSelection();
    }

    function refreshPreviewFiles() {
        let dirs = [];
        for (let item of folderContents) {
            if (isChart(item)) {
                dirs.push(item.chartDirectory);
            }
        }
        previewFiles = Rg.previewFilePathFetcher.getPreviewFilePaths(dirs);
    }

    function refreshScores() {
        Rg.profileList.mainProfile.scoreDb.cancelPending();
        let folder = historyStack.length > 0 ? historyStack[historyStack.length - 1] : "";
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

    function refreshPlayerStats() {
        let db = Rg.profileList?.mainProfile?.scoreDb;
        if (!db) {
            return;
        }
        db.getTotalStats().then((result) => {
            playerStats = result || playerStats;
            touch();
        });
    }

    function open(item, initialItem) {
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
        rebuildFolderCaches(folderContents);
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
        setVisualIndexImmediate(currentIndex);
        refreshScores();
        refreshPlayerStats();
        openedFolder();
        touchList();
        touchSelection();
        return folder;
    }

    function search(query) {
        let results = Rg.songFolderFactory.search(query || "");
        if (!results.length) {
            console.info("Search returned no results");
            return;
        }
        folderContents = [...results];
        rebuildFolderCaches(folderContents);
        results = sortFilter(results);
        realItemCount = results.length;
        addToMinimumCount(results);
        if (historyStack[historyStack.length - 1] !== "SEARCH") {
            historyStack.push("SEARCH");
        }
        items = results;
        currentIndex = 0;
        targetIndex = 0;
        selectedOffset = 0;
        setVisualIndexImmediate(0);
        refreshScores();
        refreshPlayerStats();
        openedFolder();
        touchList();
        touchSelection();
    }

    function openRoot() {
        hideRanking();
        historyStack = [""];
        open("");
    }

    function normalizeIndex(index) {
        if (logicalCount === 0) {
            return 0;
        }
        return ((index % logicalCount) + logicalCount) % logicalCount;
    }

    function setCurrentIndex(index) {
        if (logicalCount === 0) {
            currentIndex = 0;
            targetIndex = 0;
            selectedOffset = 0;
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
        let targetVisual = nearestVisualIndex(normalized, listTopbarFixed / 1000.0);
        oldBarFixed = listTopbarFixed;
        nowBarFixed = Math.round(targetVisual * 1000);
        scrollDirection = nowBarFixed < listTopbarFixed ? lr2ScrollUp : lr2ScrollDown;
        beginVisualMove(lr2ClickDuration, now);
        scrollingText = false;
        scrollingTextTimer.restart();
        touchSelection();
    }

    function setScrollIndexImmediate(index) {
        if (logicalCount === 0) {
            return;
        }
        let normalized = normalizeIndex(Math.round(index));
        selectedOffset = 0;
        currentIndex = normalized;
        targetIndex = normalized;
        setVisualIndexImmediate(normalized);
        scrollingText = false;
        scrollingTextTimer.restart();
        touchSelection();
    }

    function beginScrollFixedPointDrag() {
        scrollDirection = 0;
        scrollFixedPointDragging = true;
    }

    function setBarFixedImmediate(fixedValue) {
        let clamped = clampBarFixed(fixedValue);
        listTopbarFixed = clamped;
        oldBarFixed = clamped;
        nowBarFixed = clamped;
        barMoveStartMs = 0;
        barMoveEndMs = 0;
        return publishBarState();
    }

    function dragScrollFixedPoint(fixedValue) {
        if (logicalCount === 0) {
            return;
        }

        let clamped = clampBarFixed(fixedValue);
        scrollDirection = clamped < listCalculatedBarFixed ? lr2ScrollUp
            : (clamped > listCalculatedBarFixed ? lr2ScrollDown : scrollDirection);
        selectedOffset = 0;
        setBarFixedImmediate(clamped);
    }

    function setScrollFixedPoint(fixedValue, durationMs, snapToEntry) {
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
        if (!shouldSnap) {
            setBarFixedImmediate(clamped);
            return;
        }
        setBarFixedImmediate(clamped);
        oldBarFixed = listTopbarFixed;
        nowBarFixed = Math.round(nearestVisualIndex(rounded, clamped / 1000.0) * 1000);
        scrollDirection = nowBarFixed < listTopbarFixed ? lr2ScrollUp
            : (nowBarFixed > listTopbarFixed ? lr2ScrollDown : scrollDirection);
        barMoveStartMs = now;
        barMoveEndMs = now + Math.max(1, durationMs !== undefined ? durationMs : 100);
        targetIndex = normalizeIndex(rounded);
        let selectionTouched = publishBarState();
        if (!selectionTouched && oldOffset !== selectedOffset) {
            scrollingText = false;
            scrollingTextTimer.restart();
            touchSelection();
        }
    }

    function finishScrollFixedPoint(durationMs) {
        if (logicalCount === 0) {
            scrollFixedPointDragging = false;
            return;
        }

        let now = Date.now();
        let wasDeferred = deferVisualSelectionSync;
        let wasDragging = scrollFixedPointDragging;
        scrollFixedPointDragging = false;
        deferVisualSelectionSync = false;
        if (!wasDeferred) {
            updateVisualIndex(now);
        }

        let rounded = Math.floor(listCalculatedBarFixed / 1000);
        if (listCalculatedBarFixed % 1000 > 499) {
            rounded += 1;
        }
        let oldOffset = selectedOffset;
        selectedOffset = 0;
        oldBarFixed = listTopbarFixed;
        nowBarFixed = Math.round(nearestVisualIndex(rounded, listTopbarFixed / 1000.0) * 1000);
        scrollDirection = nowBarFixed < listTopbarFixed ? lr2ScrollUp
            : (nowBarFixed > listTopbarFixed ? lr2ScrollDown : scrollDirection);
        barMoveStartMs = now;
        barMoveEndMs = now + Math.max(1, durationMs !== undefined ? durationMs : 100);
        targetIndex = normalizeIndex(rounded);
        let selectionTouched = publishBarState();
        if (!selectionTouched && (wasDragging || oldOffset !== selectedOffset)) {
            scrollingText = false;
            scrollingTextTimer.restart();
            touchSelection();
        }
    }

    function animatedTopbarFixed(now) {
        return lr2ChangeValueByTime(oldBarFixed, nowBarFixed, barMoveStartMs, barMoveEndMs, now);
    }

    function applyLr2ScrollDelta(entries, durationMs, now, currentFixed) {
        if (logicalCount === 0 || entries === 0) {
            return;
        }
        listTopbarFixed = currentFixed !== undefined ? currentFixed : animatedTopbarFixed(now);
        oldBarFixed = listTopbarFixed;
        nowBarFixed += Math.round(entries * 1000);
        scrollDirection = entries < 0 ? lr2ScrollUp : lr2ScrollDown;
        targetIndex = normalizeIndex(Math.round(nowBarFixed / 1000) + selectedOffset);
        beginVisualMove(durationMs, now);
    }

    function scrollBy(entries, durationMs) {
        if (logicalCount === 0 || entries === 0) {
            return;
        }
        let now = Date.now();
        let duration = durationMs !== undefined ? durationMs : lr2SpeedFirst;
        applyLr2ScrollDelta(entries, duration, now, animatedTopbarFixed(now));
    }

    function scrollByKey(entries, repeated) {
        if (logicalCount === 0 || entries === 0) {
            return;
        }
        let now = Date.now();
        if (repeated) {
            if (now <= barMoveEndMs - 20) {
                return;
            }
            scrollBy(entries, lr2SpeedNext);
            return;
        }
        if (now <= barMoveEndMs) {
            return;
        }
        scrollBy(entries, lr2SpeedFirst);
    }

    function selectVisibleRow(row, barCenter) {
        if (logicalCount === 0) {
            return;
        }
        selectedOffset = row - barCenter;
        currentIndex = normalizeIndex(visualBaseIndex + selectedOffset);
        targetIndex = currentIndex;
        scrollingText = false;
        scrollingTextTimer.restart();
        touchSelection();
    }

    function activationItem() {
        if (logicalCount === 0) {
            return null;
        }
        return current;
    }

    function selectedRow(barCenter) {
        return barCenter + selectedOffset;
    }

    function visualSelectedRow(barCenter) {
        return selectedRow(barCenter);
    }

    function visualRowForIndex(index, barCenter) {
        if (logicalCount === 0) {
            return barCenter;
        }
        let offset = normalizeIndex(index) - normalizeIndex(visualBaseIndex);
        if (offset > logicalCount / 2) {
            offset -= logicalCount;
        }
        if (offset < -logicalCount / 2) {
            offset += logicalCount;
        }
        return barCenter + offset;
    }

    function currentVisualRow(barCenter) {
        return visualRowForIndex(currentIndex, barCenter);
    }

    function decrementViewIndex(repeated) {
        scrollByKey(-1, !!repeated);
    }

    function incrementViewIndex(repeated) {
        scrollByKey(1, !!repeated);
    }

    function goBack() {
        if (rankingMode) {
            hideRanking();
            return;
        }
        if (historyStack.length <= 1) {
            return;
        }
        let last = historyStack.pop();
        open(historyStack[historyStack.length - 1], last);
    }

    function goForward(item, autoplay, replay, replayScore) {
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
        historyStack.push(item);
        open(item);
    }

    function sameEntry(a, b) {
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

    function barEntry(row, barCenter) {
        if (logicalCount === 0) {
            return null;
        }
        return items[normalizeIndex(visualBaseIndex + row - barCenter)];
    }

    function refreshVisibleBarEntries(force) {
        let rowCount = Math.max(0, barRowCount || 0);
        if (logicalCount === 0 || rowCount === 0) {
            if (force || visibleBarEntries.length !== 0 || visibleBarTextCells.length !== 0) {
                visibleBarEntries = [];
                visibleBarTextCells = [];
                cachedVisibleBarBaseIndex = -1;
                cachedVisibleBarListRevision = listRevision;
                cachedVisibleBarRowCount = rowCount;
                cachedVisibleBarCenter = barCenter;
                barEntriesRevision += 1;
                barTextCellsRevision += 1;
                barTextCellsInvalidated();
            }
            return;
        }

        if (!force
                && cachedVisibleBarBaseIndex >= 0
                && cachedVisibleBarListRevision === listRevision
                && cachedVisibleBarRowCount === rowCount
                && cachedVisibleBarCenter === barCenter
                && visibleBarEntries.length === rowCount
                && visibleBarTextCells.length === rowCount) {
            let forward = (visualBaseIndex - cachedVisibleBarBaseIndex + logicalCount) % logicalCount;
            let backward = (cachedVisibleBarBaseIndex - visualBaseIndex + logicalCount) % logicalCount;
            let delta = forward <= backward ? forward : -backward;
            if (delta !== 0 && Math.abs(delta) <= Math.min(rowCount, 4)) {
                let entries = visibleBarEntries;
                let textCells = visibleBarTextCells;
                shiftBarTextCellRows(textCells, -delta);
                while (delta > 0) {
                    let entry = items[normalizeIndex(visualBaseIndex + rowCount - delta - barCenter)];
                    entries.shift();
                    entries.push(entry);
                    let row = rowCount - delta;
                    updateOutOfRangeBarTextCell(textCells, row, rowCount);
                    delta -= 1;
                }
                while (delta < 0) {
                    let entry = items[normalizeIndex(visualBaseIndex - delta - 1 - barCenter)];
                    entries.pop();
                    entries.unshift(entry);
                    let row = -delta - 1;
                    updateOutOfRangeBarTextCell(textCells, row, rowCount);
                    delta += 1;
                }
                visibleBarTextCells = textCells.slice();
                cachedVisibleBarBaseIndex = visualBaseIndex;
                barEntriesRevision += 1;
                barTextCellsRevision += 1;
                barTextCellsInvalidated();
                return;
            }
        }

        if (!force
                && cachedVisibleBarBaseIndex === visualBaseIndex
                && cachedVisibleBarListRevision === listRevision
                && cachedVisibleBarRowCount === rowCount
                && cachedVisibleBarCenter === barCenter) {
            return;
        }

        let entries = [];
        let textCells = [];
        for (let row = 0; row < rowCount; ++row) {
            let entry = items[normalizeIndex(visualBaseIndex + row - barCenter)];
            entries.push(entry);
            textCells.push(barTextCellForRow(row));
        }
        visibleBarEntries = entries;
        visibleBarTextCells = textCells;
        cachedVisibleBarBaseIndex = visualBaseIndex;
        cachedVisibleBarListRevision = listRevision;
        cachedVisibleBarRowCount = rowCount;
        cachedVisibleBarCenter = barCenter;
        barEntriesRevision += 1;
        barTextCellsRevision += 1;
        barTextCellsInvalidated();
    }

    function shiftBarTextCellRows(cells, delta) {
        for (let i = 0; i < cells.length; ++i) {
            let cell = cells[i];
            if (cell) {
                cell.row += delta;
            }
        }
    }

    function updateOutOfRangeBarTextCell(cells, row, rowCount) {
        for (let i = 0; i < cells.length; ++i) {
            let cell = cells[i];
            if (cell && (cell.row < 0 || cell.row >= rowCount)) {
                updateBarTextCellForRow(cell, row);
                return;
            }
        }
    }

    function visibleBarEntry(row, fallbackBarCenter) {
        if (row >= 0 && row < visibleBarEntries.length) {
            return visibleBarEntries[row];
        }
        return barEntry(row, fallbackBarCenter);
    }

    function barTextCellForRow(row) {
        let entry = items[normalizeIndex(visualBaseIndex + row - barCenter)];
        return {
            row: row,
            entry: entry,
            text: entryDisplayName(entry, true),
            titleType: entryTitleType(entry),
            bodyType: entryBodyType(entry),
            playLevel: entryPlayLevel(entry),
            difficulty: entryDifficulty(entry),
            keymode: entry ? (entry.keymode || 0) : 0,
            ranking: isRankingEntry(entry),
            chartLike: isChart(entry),
            entryLike: isEntry(entry),
            folderLike: isFolderLikeForLamp(entry),
            lamp: entryLamp(entry),
            clearType: entryClearType(entry),
            rank: entryRank(entry)
        };
    }

    function updateBarTextCellForRow(cell, row) {
        let entry = items[normalizeIndex(visualBaseIndex + row - barCenter)];
        cell.row = row;
        cell.entry = entry;
        cell.text = entryDisplayName(entry, true);
        cell.titleType = entryTitleType(entry);
        cell.bodyType = entryBodyType(entry);
        cell.playLevel = entryPlayLevel(entry);
        cell.difficulty = entryDifficulty(entry);
        cell.keymode = entry ? (entry.keymode || 0) : 0;
        cell.ranking = isRankingEntry(entry);
        cell.chartLike = isChart(entry);
        cell.entryLike = isEntry(entry);
        cell.folderLike = isFolderLikeForLamp(entry);
        cell.lamp = entryLamp(entry);
        cell.clearType = entryClearType(entry);
        cell.rank = entryRank(entry);
    }

    function visibleBarTextCell(slot) {
        if (slot >= 0 && slot < visibleBarTextCells.length) {
            return visibleBarTextCells[slot];
        }
        return barTextCellForRow(slot);
    }

    function visibleBarDisplayName(row, fallbackBarCenter) {
        let cell = visibleBarTextCell(row);
        if (cell && cell.row === row) {
            return cell.text || "";
        }
        return entryDisplayName(visibleBarEntry(row, fallbackBarCenter), true);
    }

    function entryDisplayName(item, includeSubtitle) {
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

    function entryMainTitle(item) {
        if (isRankingEntry(item)) {
            return item.title || "";
        }
        if (isChart(item) || isEntry(item)) {
            return item.title || "";
        }
        return entryDisplayName(item, false);
    }

    function currentFolderDisplayName() {
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

    function currentTableItem() {
        for (let i = historyStack.length - 1; i >= 0; --i) {
            if (isTable(historyStack[i])) {
                return historyStack[i];
            }
        }
        return isTable(current) ? current : null;
    }

    function currentTableName() {
        let tableItem = currentTableItem();
        return tableItem ? (tableItem.name || "") : "";
    }

    function currentTableLevelName() {
        let item = historyStack.length > 0 ? historyStack[historyStack.length - 1] : null;
        if (isLevel(item)) {
            return item.name || "";
        }
        return isLevel(current) ? (current.name || "") : "";
    }

    function currentTableFullName() {
        let tableName = currentTableName();
        let levelName = currentTableLevelName();
        if (tableName.length > 0 && levelName.length > 0) {
            return tableName + " " + levelName;
        }
        return tableName.length > 0 ? tableName : levelName;
    }

    function entrySubtitle(item) {
        return isChart(item) || isEntry(item) ? (item.subtitle || "") : "";
    }

    function entryGenre(item) {
        return isChart(item) ? (item.genre || "") : "";
    }

    function entryArtist(item) {
        return isChart(item) || isEntry(item) ? (item.artist || "") : "";
    }

    function entrySubartist(item) {
        return isChart(item) || isEntry(item) ? (item.subartist || "") : "";
    }

    function entryBodyType(item) {
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

    function entryTitleType(item) {
        // OpenLR2 uses title type 1 for the optional "new title" flash, not
        // for selection. We do not have LR2 add-date metadata here yet, so
        // default every row to the normal title source.
        return 0;
    }

    function keyFilterLabel() {
        switch (keyFilter) {
        case 1: return "SINGLE";
        case 2: return "7KEYS";
        case 3: return "5KEYS";
        case 4: return "DOUBLE";
        case 5: return "14KEYS";
        case 6: return "10KEYS";
        default: return "ALL";
        }
    }

    function sortLabel() {
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

    function difficultyFilterLabel() {
        switch (difficultyFilter) {
        case 1: return "EASY";
        case 2: return "STANDARD";
        case 3: return "HARD";
        case 4: return "EXPERT";
        case 5: return "ULTIMATE";
        default: return "ALL";
        }
    }

    function entryPlayLevel(item) {
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

    function rawChartDifficulty(item) {
        return isChart(item) ? (item.difficulty || 0) : 0;
    }

    function entryDifficulty(item) {
        if (!isChart(item)) {
            return 0;
        }
        let cached = chartDifficultyCache[item.path || ""];
        if (cached !== undefined) {
            return cached;
        }
        return Math.max(0, rawChartDifficulty(item));
    }

    function chartNoteCount(item) {
        if (!isChart(item)) {
            return 0;
        }
        return (item.normalNoteCount || 0)
            + (item.scratchCount || 0)
            + (item.lnCount || 0)
            + (item.bssCount || 0)
            + (item.mineCount || 0);
    }

    function chartPlayableNoteCount(item) {
        if (!isChart(item)) {
            return 0;
        }
        return (item.normalNoteCount || 0)
            + (item.scratchCount || 0)
            + (item.lnCount || 0)
            + (item.bssCount || 0);
    }

    function chartLengthSeconds(item) {
        return isChart(item) ? Math.floor(Math.max(0, item.length || 0) / 1000000000) : -1;
    }

    function chartDensityValue(item, propertyName, afterDot) {
        if (!isChart(item)) {
            return -1;
        }
        let density = Math.max(0, item[propertyName] || 0);
        return afterDot ? Math.floor(density * 100) % 100 : Math.floor(density);
    }

    function chartDensityWhole(item) {
        return chartDensityValue(item, "avgDensity", false);
    }

    function chartDensityAfterDot(item) {
        return chartDensityValue(item, "avgDensity", true);
    }

    function entryIdentifier(item) {
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

    function entryScores(item) {
        let revisionMarker = scoreRevision;
        let id = entryIdentifier(item);
        return id ? (scores[id] || emptyScoreList) : emptyScoreList;
    }

    function cachedEntryScores(item) {
        return item === current || item === cachedSelectedChartData
            ? selectedScoreList
            : entryScores(item);
    }

    function clearTypeOf(score) {
        return score?.result?.clearType || "NOPLAY";
    }

    function clearTypeLamp(clear) {
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

    function hasBarLampVariant(variants, variant) {
        return variants && variants.indexOf(variant) !== -1;
    }

    function usesExtendedBarLampVariants(variants) {
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

    function clearTypeBarLamp(clear, variants) {
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

    function entryBarLamp(item, variants) {
        if (isFolderLikeForLamp(item)) {
            return entryLamp(item);
        }
        return clearTypeBarLamp(entryClearType(item), variants);
    }

    function isClearedScore(score) {
        let clearType = clearTypeOf(score);
        return clearType !== "FAILED" && clearType !== "NOPLAY";
    }

    function scoreListByNewest(scoreList) {
        let result = [];
        for (let score of scoreList) {
            if (score && score.result) {
                result.push(score);
            }
        }
        result.sort((a, b) => (b.result.unixTimestamp || 0) - (a.result.unixTimestamp || 0));
        return result;
    }

    function getClearType(scoreList) {
        let clearType = "NOPLAY";
        for (let score of scoreList) {
            let next = clearTypeOf(score);
            if (clearTypePriorities.indexOf(next) > clearTypePriorities.indexOf(clearType)) {
                clearType = next;
            }
        }
        return clearType;
    }

    function bestScoreByPoints(scoreList) {
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

    function scoreWithBestClear(scoreList) {
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

    function scoreWithBestCombo(scoreList) {
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

    function replayScoreForType(item, replayType) {
        let scoreList = cachedEntryScores(item);
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

    function judgementCount(counts, judgement) {
        return judgement >= 0 && judgement < counts.length ? (counts[judgement] || 0) : 0;
    }

    function emptyJudgeTimingCounts() {
        return {
            early: [0, 0, 0, 0, 0, 0],
            late: [0, 0, 0, 0, 0, 0]
        };
    }

    function timingBucketForJudgement(judgement) {
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

    function judgeTimingCountsForScore(score) {
        let counts = emptyJudgeTimingCounts();
        let events = score && score.replayData && score.replayData.hitEvents
            ? score.replayData.hitEvents
            : [];
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

    function timingCount(counts, bucket, early) {
        if (bucket < 0 || !counts) {
            return 0;
        }
        let source = early ? counts.early : counts.late;
        return source && bucket < source.length ? (source[bucket] || 0) : 0;
    }

    function totalTimingCount(counts, early) {
        let total = 0;
        for (let bucket = 1; bucket <= 5; ++bucket) {
            total += timingCount(counts, bucket, early);
        }
        return total;
    }

    function statsForScore(score, includeTiming) {
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

    function bestStats(scoreList) {
        if (!scoreList || scoreList.length === 0) {
            return null;
        }
        return statsForScore(bestScoreByPoints(scoreList), false);
    }

    function ensureStatsTiming(stats, score) {
        if (!stats || stats.early || !score) {
            return stats;
        }
        let timingCounts = judgeTimingCountsForScore(score);
        stats.early = timingCounts.early;
        stats.late = timingCounts.late;
        stats.totalEarly = totalTimingCount(timingCounts, true);
        stats.totalLate = totalTimingCount(timingCounts, false);
        return stats;
    }

    function ensureSelectedBestStatsTiming() {
        return ensureStatsTiming(selectedBestStats, bestScoreByPoints(selectedScoreList));
    }

    function scoreCounts(scoreList) {
        if (!scoreList || scoreList.length === 0) {
            return emptyScoreCounts;
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
        let minBadPoor = -1;
        for (let score of scoreList) {
            if (!score || !score.result) {
                continue;
            }
            ++counts.play;
            if (isClearedScore(score)) {
                ++counts.clear;
            }
            let clearType = clearTypeOf(score);
            if (clearType === "FAILED") {
                ++counts.fail;
            } else if (clearType === "AEASY") {
                ++counts.assist;
            } else if (clearType === "EASY") {
                ++counts.easy;
            } else if (clearType === "NORMAL") {
                ++counts.normal;
            } else if (clearType === "HARD") {
                ++counts.hard;
            } else if (clearType === "EXHARD") {
                ++counts.exhard;
            } else if (clearType === "FC") {
                ++counts.fc;
            } else if (clearType === "PERFECT") {
                ++counts.perfect;
            } else if (clearType === "MAX") {
                ++counts.max;
            } else {
                ++counts.noplay;
            }
                let stats = statsForScore(score, false);
            if (stats) {
                minBadPoor = minBadPoor < 0 ? stats.badPoor : Math.min(minBadPoor, stats.badPoor);
            }
        }
        counts.minBadPoor = Math.max(0, minBadPoor);
        return counts;
    }

    function percentInteger(value, total) {
        return total > 0 ? Math.floor(Math.max(0, value || 0) * 100 / total) : 0;
    }

    function percentAfterDot(value, total) {
        return total > 0 ? Math.floor(Math.max(0, value || 0) * 10000 / total) % 100 : 0;
    }

    function scorePrintValue(stats, chart) {
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

    function currentFolderScoreCountsOrNull() {
        let revisionMarker = folderLampRevision;
        let key = folderLampKey(current);
        return key && folderScoreCountsByKey[key] !== undefined
            ? folderScoreCountsByKey[key]
            : null;
    }

    function currentFolderScoreCounts() {
        return currentFolderScoreCountsOrNull() || emptyFolderScoreCounts;
    }

    function appendScoreClearOptionIds(clearType, ids) {
        switch (clearType || "NOPLAY") {
        case "AEASY":
            ids.push(124);
            ids.push(1100);
            ids.push(121);
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
            ids.push(119);
            ids.push(125);
            ids.push(1102);
            break;
        case "FC":
            ids.push(105);
            break;
        case "PERFECT":
            ids.push(122);
            ids.push(1103);
            break;
        case "MAX":
            ids.push(122);
            ids.push(1104);
            break;
        }
    }

    function appendScoreOptionIds(score, ids) {
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

    function scoreOptionIds(item) {
        if (item === current || item === cachedSelectedChartData) {
            return selectedScoreOptionIds;
        }
        return scoreOptionIdsFromList(entryScores(item));
    }

    function scoreOptionIdsFromList(scoreList) {
        if (!scoreList || scoreList.length === 0) {
            return emptyScoreOptionIds;
        }
        let ids = [];
        appendScoreClearOptionIds(getClearType(scoreList), ids);
        for (let score of scoreList) {
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
            let revisionMarker = folderLampRevision;
            let key = folderLampKey(item);
            return key && folderLampByKey[key] !== undefined ? folderLampByKey[key] : 0;
        }
        let clear = getClearType(cachedEntryScores(item));
        return clearTypeLamp(clear);
    }

    function entryClearType(item) {
        if (isRankingEntry(item)) {
            return item.bestClearType || "NOPLAY";
        }
        if (isFolderLikeForLamp(item)) {
            return "NOPLAY";
        }
        return getClearType(cachedEntryScores(item));
    }

    function beatorajaClearOption(item) {
        switch (entryClearType(item)) {
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

    function entryRank(item) {
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
        let best = bestScoreByPoints(cachedEntryScores(item));
        if (!best || !best.result || best.result.maxPoints <= 0) {
            return 0;
        }
        let rate = 100.0 * best.result.points / best.result.maxPoints;
        if (rate >= 100.0) return 9;
        if (rate >= 100.0 * 8 / 9) return 8;
        if (rate >= 100.0 * 7 / 9) return 7;
        if (rate >= 100.0 * 6 / 9) return 6;
        if (rate >= 100.0 * 5 / 9) return 5;
        if (rate >= 100.0 * 4 / 9) return 4;
        if (rate >= 100.0 * 3 / 9) return 3;
        if (rate >= 100.0 * 2 / 9) return 2;
        return rate > 0 ? 1 : 0;
    }

    function entryScoreRate(item) {
        let best = bestScoreByPoints(cachedEntryScores(item));
        if (!best || !best.result || best.result.maxPoints <= 0) {
            return 0;
        }
        return best.result.points / best.result.maxPoints;
    }

    function selectedChartData() {
        return cachedSelectedChartData;
    }

    function refreshSelectedScoreState() {
        if (selectedScoreStateCacheRevision !== scoreRevision) {
            selectedScoreStateCache = {};
            selectedScoreStateCacheRevision = scoreRevision;
        }
        let item = current;
        let nextChartData = rankingMode && rankingBaseItem
            ? rankingBaseItem
            : ((isChart(item) || isEntry(item)) ? item : null);
        let stateKey = "";
        if (nextChartData) {
            stateKey = "chart:" + entryIdentifier(nextChartData);
        } else if (isRankingEntry(item)) {
            stateKey = "ranking:" + (item.sourceMd5 || "") + ":" + (item.rankingIndex || 0);
        } else {
            stateKey = "item:" + entryIdentifier(item);
        }
        let cached = stateKey ? selectedScoreStateCache[stateKey] : null;
        if (cached) {
            cachedSelectedChartData = cached.chartData;
            selectedScoreList = cached.scoreList;
            selectedBestStats = cached.bestStats;
            selectedScoreCounts = cached.scoreCounts;
            selectedScoreOptionIds = cached.scoreOptionIds;
            return;
        }
        let scoreList = entryScores(nextChartData || item);
        let nextBestStats = bestStats(scoreList);
        let nextScoreCounts = scoreCounts(scoreList);
        let nextScoreOptionIds = scoreOptionIdsFromList(scoreList);
        cachedSelectedChartData = nextChartData;
        selectedScoreList = scoreList;
        selectedBestStats = nextBestStats;
        selectedScoreCounts = nextScoreCounts;
        selectedScoreOptionIds = nextScoreOptionIds;
        if (stateKey) {
            selectedScoreStateCache[stateKey] = {
                chartData: nextChartData,
                scoreList: scoreList,
                bestStats: nextBestStats,
                scoreCounts: nextScoreCounts,
                scoreOptionIds: nextScoreOptionIds
            };
        }
    }

    function rankingClearCountValue() {
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

    function rankingClearPercent() {
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

    function rankingClearPercentAfterDot() {
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

    function normalizedMd5(value) {
        return value ? String(value).toLowerCase() : "";
    }

    function rankingStatsAvailable() {
        let chart = selectedChartData();
        let currentMd5 = chart ? normalizedMd5(chart.md5) : "";
        return currentMd5.length > 0
            && currentMd5 === normalizedMd5(rankingStatsMd5)
            && rankingPlayerCount > 0;
    }

    function setRankingStats(md5, clearCounts, playerCount, totalPlayCount, playerRank) {
        rankingStatsMd5 = normalizedMd5(md5);
        rankingClearCounts = clearCounts || {};
        rankingPlayerRank = Math.max(0, Number(playerRank || 0));
        rankingPlayerCount = Math.max(0, Number(playerCount || 0));
        rankingTotalPlayCount = Math.max(0, Number(totalPlayCount || rankingPlayerCount));
        touch();
    }

    function rankingEntryFrom(entry, index, chart) {
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

    function showRanking(entries, clearCounts, playerCount, totalPlayCount, playerRank) {
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
        scrollingText = false;
        scrollingTextTimer.restart();
        touchList();
        touchSelection();
        return true;
    }

    function hideRanking() {
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
        scrollingText = false;
        scrollingTextTimer.restart();
        touchList();
        touchSelection();
        return true;
    }

    function chartGroupKey(chart) {
        if (!chart) {
            return "";
        }
        // LR2 groups difficulty buttons by the song folder, not by the
        // currently selected chart row.
        return chart.chartDirectory || chart.directory || "";
    }

    function chartDifficultyGroupKey(chart) {
        if (!chart) {
            return "";
        }
        // OpenLR2 computes the side difficulty bars per song folder and
        // keymode. Mixing 5K/7K variants here makes the bars drift out of sync.
        return chartGroupKey(chart) + "\n" + (chart.keymode || 0);
    }

    function difficultyHint(chart) {
        let text = ((chart.title || "") + " " + (chart.subtitle || "")).toLowerCase();
        if (/\binsane\b/.test(text)) return 5;
        if (/\banother\b/.test(text)) return 4;
        if (/\bhyper\b/.test(text)) return 3;
        if (/\bnormal\b/.test(text)) return 2;
        if (/\bbeginner\b|\bbgn\b/.test(text)) return 1;
        if (entryPlayLevel(chart) <= 1) return 1;
        return 0;
    }

    function inferGroupDifficulties(group) {
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

    function rebuildFolderCaches(sourceItems) {
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

        chartGroupCache = groups;
        chartDifficultyCache = difficulties;
    }

    function chartsForCurrentSong() {
        let chart = selectedChartData();
        if (!chart) {
            return [];
        }

        let groupKey = chartDifficultyGroupKey(chart);
        let result = chartGroupCache[groupKey] || [];
        if (result.length === 0) {
            result.push(chart);
        }
        return result;
    }

    function chartForDifficulty(diff) {
        let currentChart = selectedChartData();
        let charts = chartsForCurrentSong();
        let fallback = null;
        for (let chart of charts) {
            if (entryDifficulty(chart) !== diff) {
                continue;
            }
            if (currentChart && sameEntry(chart, currentChart)) {
                return chart;
            }
            if (!fallback) {
                fallback = chart;
            }
        }
        return fallback;
    }

    function nextChartForDifficulty(diff) {
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

    function clickDifficulty(diff) {
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

    function difficultyCount(diff) {
        let count = 0;
        for (let chart of chartsForCurrentSong()) {
            if (entryDifficulty(chart) === diff) {
                ++count;
            }
        }
        return count;
    }

    function difficultyPlayLevel(diff) {
        let chart = chartForDifficulty(diff);
        return chart ? (chart.playLevel || 0) : 0;
    }

    function levelBarFlashThreshold() {
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

    function difficultyLevelBarOption(diff) {
        let level = difficultyPlayLevel(diff);
        return level > levelBarFlashThreshold() ? 74 + diff : 69 + diff;
    }

    function difficultyGraphValue(diff) {
        let level = difficultyPlayLevel(diff);
        if (level <= 0) {
            return 0;
        }
        return level / Math.max(1, levelBarFlashThreshold());
    }

    function difficultyLamp(diff) {
        let chart = chartForDifficulty(diff);
        return chart ? entryLamp(chart) : 0;
    }

    function attachedTextFile(chart) {
        if (!chart || !chart.chartDirectory) {
            return "";
        }

        let dir = chart.chartDirectory;
        let cached = attachedTextCache[dir];
        if (cached !== undefined) {
            return cached;
        }

        let found = "";
        let files = Rg.fileQuery.getSelectableFilesForDirectory(dir);
        for (let file of files) {
            if (String(file).toLowerCase().endsWith(".txt")) {
                let separator = dir.endsWith("/") || dir.endsWith("\\") ? "" : "/";
                found = dir + separator + file;
                break;
            }
        }
        attachedTextCache[dir] = found;
        return found;
    }

    function hasAttachedText(chart) {
        return attachedTextFile(chart).length > 0;
    }

    function hasReplay(chart) {
        return cachedEntryScores(chart).length > 0;
    }

    function hasBga(chart) {
        return !!chart && (!!chart.stageFile || !!chart.banner || !!chart.backBmp);
    }

    function hasLongNote(chart) {
        return !!chart && ((chart.lnCount || 0) + (chart.bssCount || 0)) > 0;
    }

    function judgeOption(chart) {
        let rank = chart ? (chart.rank || 75) : 75;
        if (rank <= 25) return 180;
        if (rank <= 50) return 181;
        if (rank <= 75) return 182;
        return 183;
    }

    function highLevelOption(chart) {
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

    function selectedChartWrapper() {
        let chartData = selectedChartData();
        return chartData ? { chartData: chartData } : null;
    }

    function selectedPreviewSource() {
        let chartData = selectedChartData();
        return chartData ? previewFiles[chartData.chartDirectory] : "";
    }

    function numberValue(num) {
        let chart = selectedChartData();
        let rankingEntry = isRankingEntry(current) ? current : null;
        let stats = selectedBestStats;
        let counts = selectedScoreCounts;
        let currentFolderCounts = currentFolderScoreCountsOrNull();
        let folderCounts = currentFolderCounts || emptyFolderScoreCounts;
        let hasRankingStats = rankingStatsAvailable();
        if ((num >= 410 && num <= 419) || (num >= 421 && num <= 424)) {
            stats = ensureSelectedBestStatsTiming();
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
            return chart ? (chart.playLevel || 0) : 0;
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
            return rankingEntry ? Math.round(rankingEntry.bestPoints) : (stats ? Math.round(stats.score) : 0);
        case 71:
            return rankingEntry ? Math.round(rankingEntry.bestPoints) : (stats ? Math.round(stats.exscore) : 0);
        case 72:
            return rankingEntry ? Math.round(rankingEntry.maxPoints) : (stats ? Math.round(stats.maxPoints) : 0);
        case 73:
            return rankingEntry && rankingEntry.maxPoints > 0
                ? Math.round(rankingEntry.bestPoints * 100 / rankingEntry.maxPoints)
                : (stats && stats.maxPoints > 0 ? Math.round(stats.score * 100 / stats.maxPoints) : 0);
        case 74:
            return rankingEntry ? Math.round(rankingEntry.maxPoints / 2) : chartPlayableNoteCount(chart);
        case 75:
            return rankingEntry ? rankingEntry.bestCombo : (stats ? stats.maxCombo : 0);
        case 76:
            return rankingEntry ? rankingEntry.bestComboBreaks : counts.minBadPoor;
        case 77:
            return rankingEntry ? rankingEntry.scoreCount : counts.play;
        case 78:
            return counts.clear;
        case 79:
            return counts.fail;
        case 100:
            return scorePrintValue(stats, chart);
        case 101:
            return stats ? Math.round(stats.exscore) : 0;
        case 102:
            return stats ? percentInteger(stats.exscore, stats.maxPoints) : 0;
        case 103:
            return stats ? percentAfterDot(stats.exscore, stats.maxPoints) : 0;
        case 104:
        case 105:
            return stats ? stats.maxCombo : 0;
        case 106:
            return chartPlayableNoteCount(chart);
        case 107:
            return 0;
        case 108:
        case 128:
            return stats ? Math.round(stats.exscore) : 0;
        case 109:
            return 0;
        case 80:
            return stats ? stats.pg : 0;
        case 81:
            return stats ? stats.gr : 0;
        case 82:
            return stats ? stats.gd : 0;
        case 83:
            return stats ? stats.bd : 0;
        case 84:
            return stats ? stats.poor : 0;
        case 85:
            return stats ? percentInteger(stats.pg, stats.totalJudgements) : 0;
        case 86:
            return stats ? percentInteger(stats.gr, stats.totalJudgements) : 0;
        case 87:
            return stats ? percentInteger(stats.gd, stats.totalJudgements) : 0;
        case 88:
            return stats ? percentInteger(stats.bd, stats.totalJudgements) : 0;
        case 89:
            return stats ? percentInteger(stats.poor, stats.totalJudgements) : 0;
        case 110:
            return stats ? stats.pg : 0;
        case 111:
            return stats ? stats.gr : 0;
        case 112:
            return stats ? stats.gd : 0;
        case 113:
            return stats ? stats.bd : 0;
        case 114:
            return stats ? stats.poor : 0;
        case 115:
            return stats ? percentInteger(stats.exscore, stats.maxPoints) : 0;
        case 116:
            return stats ? percentAfterDot(stats.exscore, stats.maxPoints) : 0;
        case 410:
            return stats ? timingCount(stats, 0, true) : 0;
        case 411:
            return stats ? timingCount(stats, 0, false) : 0;
        case 412:
            return stats ? timingCount(stats, 1, true) : 0;
        case 413:
            return stats ? timingCount(stats, 1, false) : 0;
        case 414:
            return stats ? timingCount(stats, 2, true) : 0;
        case 415:
            return stats ? timingCount(stats, 2, false) : 0;
        case 416:
            return stats ? timingCount(stats, 3, true) : 0;
        case 417:
            return stats ? timingCount(stats, 3, false) : 0;
        case 418:
            return stats ? timingCount(stats, 4, true) : 0;
        case 419:
            return stats ? timingCount(stats, 4, false) : 0;
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
            return stats ? Math.round(stats.exscore) : 0;
        case 152:
            return stats ? Math.round(stats.exscore) : 0;
        case 154:
            return 0;
        case 420:
            return stats ? stats.miss : 0;
        case 421:
            return stats ? timingCount(stats, 5, true) : 0;
        case 422:
            return stats ? timingCount(stats, 5, false) : 0;
        case 423:
            return stats ? stats.totalEarly : 0;
        case 424:
            return stats ? stats.totalLate : 0;
        case 425:
            return stats ? stats.comboBreak : 0;
        case 426:
            return stats ? stats.pr : 0;
        case 427:
            return stats ? stats.badPoor : 0;
        case 92:
            return hasRankingStats
                ? rankingPlayerRank
                : (chart && chart.mainBpm ? Math.round(chart.mainBpm) : -1);
        case 93:
            return hasRankingStats ? rankingPlayerCount : 0;
        case 94:
            return hasRankingStats ? rankingClearPercent("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : 0;
        case 179:
            return hasRankingStats ? rankingPlayerRank : 0;
        case 180:
            return hasRankingStats ? rankingPlayerCount : 0;
        case 181:
            return hasRankingStats ? rankingClearPercent("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : 0;
        case 182:
            return 0;
        case 200:
            return hasRankingStats ? rankingPlayerCount : counts.play;
        case 201:
            return hasRankingStats ? rankingTotalPlayCount : counts.play;
        case 202:
            return hasRankingStats ? rankingClearCountValue("NOPLAY") : counts.noplay;
        case 203:
            return hasRankingStats ? rankingClearPercent("NOPLAY") : percentInteger(counts.noplay, counts.play + counts.noplay);
        case 204:
            return hasRankingStats ? rankingClearCountValue("AEASY") : counts.assist;
        case 205:
            return hasRankingStats ? rankingClearPercent("AEASY") : percentInteger(counts.assist, counts.play);
        case 206:
            return hasRankingStats ? rankingClearCountValue("LIGHTASSIST") : counts.lightAssist;
        case 207:
            return hasRankingStats ? rankingClearPercent("LIGHTASSIST") : percentInteger(counts.lightAssist, counts.play);
        case 208:
            return hasRankingStats ? rankingClearCountValue("EXHARD") : counts.exhard;
        case 209:
            return hasRankingStats ? rankingClearPercent("EXHARD") : percentInteger(counts.exhard, counts.play);
        case 210:
            return hasRankingStats ? rankingClearCountValue("FAILED") : counts.fail;
        case 211:
            return hasRankingStats ? rankingClearPercent("FAILED") : percentInteger(counts.fail, counts.play);
        case 212:
            return hasRankingStats ? rankingClearCountValue("EASY") : counts.easy;
        case 213:
            return hasRankingStats ? rankingClearPercent("EASY") : percentInteger(counts.easy, counts.play);
        case 214:
            return hasRankingStats ? rankingClearCountValue("NORMAL") : counts.normal;
        case 215:
            return hasRankingStats ? rankingClearPercent("NORMAL") : percentInteger(counts.normal, counts.play);
        case 216:
            return hasRankingStats ? rankingClearCountValue("HARD") : counts.hard;
        case 217:
            return hasRankingStats ? rankingClearPercent("HARD") : percentInteger(counts.hard, counts.play);
        case 218:
            return hasRankingStats ? rankingClearCountValue("FC") : counts.fc;
        case 219:
            return hasRankingStats ? rankingClearPercent("FC") : percentInteger(counts.fc, counts.play);
        case 222:
            return hasRankingStats ? rankingClearCountValue("PERFECT") : counts.perfect;
        case 223:
            return hasRankingStats ? rankingClearPercent("PERFECT") : percentInteger(counts.perfect, counts.play);
        case 224:
            return hasRankingStats ? rankingClearCountValue("MAX") : counts.max;
        case 225:
            return hasRankingStats ? rankingClearPercent("MAX") : percentInteger(counts.max, counts.play);
        case 226:
            return hasRankingStats ? rankingClearCountValue("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : counts.clear;
        case 227:
            return hasRankingStats ? rankingClearPercent("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : percentInteger(counts.clear, counts.play);
        case 228:
            return hasRankingStats ? rankingClearCountValue("FC", "PERFECT", "MAX") : counts.fc + counts.perfect + counts.max;
        case 229:
            return hasRankingStats ? rankingClearPercent("FC", "PERFECT", "MAX") : percentInteger(counts.fc + counts.perfect + counts.max, counts.play);
        case 230:
            return hasRankingStats ? rankingClearPercentAfterDot("NOPLAY") : percentAfterDot(counts.noplay, counts.play + counts.noplay);
        case 231:
            return hasRankingStats ? rankingClearPercentAfterDot("AEASY") : percentAfterDot(counts.assist, counts.play);
        case 232:
            return hasRankingStats ? rankingClearPercentAfterDot("LIGHTASSIST") : percentAfterDot(counts.lightAssist, counts.play);
        case 233:
            return hasRankingStats ? rankingClearPercentAfterDot("EXHARD") : percentAfterDot(counts.exhard, counts.play);
        case 234:
            return hasRankingStats ? rankingClearPercentAfterDot("FAILED") : percentAfterDot(counts.fail, counts.play);
        case 235:
            return hasRankingStats ? rankingClearPercentAfterDot("EASY") : percentAfterDot(counts.easy, counts.play);
        case 236:
            return hasRankingStats ? rankingClearPercentAfterDot("NORMAL") : percentAfterDot(counts.normal, counts.play);
        case 237:
            return hasRankingStats ? rankingClearPercentAfterDot("HARD") : percentAfterDot(counts.hard, counts.play);
        case 238:
            return hasRankingStats ? rankingClearPercentAfterDot("FC") : percentAfterDot(counts.fc, counts.play);
        case 239:
            return hasRankingStats ? rankingClearPercentAfterDot("PERFECT") : percentAfterDot(counts.perfect, counts.play);
        case 240:
            return hasRankingStats ? rankingClearPercentAfterDot("MAX") : percentAfterDot(counts.max, counts.play);
        case 241:
            return hasRankingStats ? rankingClearPercentAfterDot("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : percentAfterDot(counts.clear, counts.play);
        case 242:
            return hasRankingStats ? rankingClearPercentAfterDot("FC", "PERFECT", "MAX") : percentAfterDot(counts.fc + counts.perfect + counts.max, counts.play);
        case 90:
        case 290:
            return chart && (chart.maxBpm || chart.mainBpm)
                ? Math.round(chart.maxBpm || chart.mainBpm)
                : -1;
        case 91:
        case 291:
            return chart && (chart.minBpm || chart.mainBpm)
                ? Math.round(chart.minBpm || chart.mainBpm)
                : -1;
        case 300:
            return currentFolderCounts ? folderCounts.total : -1;
        case 320:
            return currentFolderCounts ? folderCounts.noplay : -1;
        case 321:
            return currentFolderCounts ? folderCounts.fail : -1;
        case 322:
            return currentFolderCounts ? folderCounts.assist : -1;
        case 323:
            return currentFolderCounts ? folderCounts.lightAssist : -1;
        case 324:
            return currentFolderCounts ? folderCounts.easy : -1;
        case 325:
            return currentFolderCounts ? folderCounts.normal : -1;
        case 326:
            return currentFolderCounts ? folderCounts.hard : -1;
        case 327:
            return currentFolderCounts ? folderCounts.exhard : -1;
        case 328:
            return currentFolderCounts ? folderCounts.fc : -1;
        case 329:
            return currentFolderCounts ? folderCounts.perfect : -1;
        case 330:
            return currentFolderCounts ? folderCounts.max : -1;
        case 350:
            return chart ? (chart.normalNoteCount || 0) : -1;
        case 351:
            return chart ? (chart.lnCount || 0) : -1;
        case 352:
            return chart ? (chart.scratchCount || 0) : -1;
        case 353:
            return chart ? (chart.bssCount || 0) : -1;
        case 354:
            return chart ? (chart.mineCount || 0) : -1;
        case 360:
            return chartDensityValue(chart, "peakDensity", false);
        case 361:
            return chartDensityValue(chart, "peakDensity", true);
        case 362:
            return chartDensityValue(chart, "endDensity", false);
        case 363:
            return chartDensityValue(chart, "endDensity", true);
        case 364:
            return chartDensityWhole(chart);
        case 365:
            return chartDensityAfterDot(chart);
        case 368:
            return chart ? Math.floor(chart.total || 0) : -1;
        case 1163: {
            let seconds = chartLengthSeconds(chart);
            return seconds >= 0 ? Math.floor(seconds / 60) % 60 : -1;
        }
        case 1164: {
            let seconds = chartLengthSeconds(chart);
            return seconds >= 0 ? seconds % 60 : -1;
        }
        default:
            return 0;
        }
    }

    function hasDifficulty(diff) {
        return !!chartForDifficulty(diff);
    }

    function barGraphValue(type) {
        let stats = selectedBestStats;
        switch (type) {
        case 101:
            return logicalCount > 1 ? Math.max(0, Math.min(1, normalizedVisualIndex / Math.max(1, logicalCount - 1))) : 0;
        case 102:
            return 1;
        case 110:
        case 111:
            return stats && stats.maxPoints > 0 ? stats.exscore / stats.maxPoints : 0;
        case 112:
        case 113:
            return stats && stats.maxPoints > 0 ? stats.exscore / stats.maxPoints : 0;
        case 114:
        case 115:
            return 0;
        case 103: {
            let chart = selectedChartData();
            let level = chart ? (chart.playLevel || 0) : 0;
            return level > 0 ? level / Math.max(1, levelBarFlashThreshold()) : 0;
        }
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
            return difficultyGraphValue(type - 104);
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
            return stats ? stats.pg / stats.totalJudgements : 0;
        case 41:
            return stats ? stats.gr / stats.totalJudgements : 0;
        case 42:
            return stats ? stats.gd / stats.totalJudgements : 0;
        case 43:
            return stats ? stats.bd / stats.totalJudgements : 0;
        case 44:
            return stats ? stats.pr / stats.totalJudgements : 0;
        case 45:
            return stats && selectedChartData() ? stats.maxCombo / Math.max(1, selectedChartData().normalNoteCount + selectedChartData().scratchCount + selectedChartData().lnCount + selectedChartData().bssCount) : 0;
        case 46:
        case 47:
            return stats && stats.maxPoints > 0 ? stats.score / stats.maxPoints : 0;
        default:
            return 0;
        }
    }

    Timer {
        id: scrollingTextTimer
        interval: 500
        onTriggered: root.scrollingText = true
    }
}
