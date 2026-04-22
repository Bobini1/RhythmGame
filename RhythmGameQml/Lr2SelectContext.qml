pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

Item {
    id: root

    property var items: []
    property var folderContents: []
    property var historyStack: []
    property var scores: ({})
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
    property bool scrollFixedPointDragging: false
    property bool scrollingText: false
    property string searchText: ""
    property var attachedTextCache: ({})
    property int difficultyFilter: 0
    property int keyFilter: 0
    property int sortMode: 0
    property int realItemCount: 0
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
        easy: 0,
        normal: 0,
        hard: 0,
        fc: 0,
        minBadPoor: 0
    })
    property var selectedScoreList: []
    property var selectedBestStats: null
    property var selectedScoreCounts: ({
        play: 0,
        clear: 0,
        fail: 0,
        easy: 0,
        normal: 0,
        hard: 0,
        fc: 0,
        minBadPoor: 0
    })
    property var selectedScoreOptionIds: []
    readonly property int lr2SpeedFirst: 300
    readonly property int lr2SpeedNext: 70
    readonly property int lr2WheelDuration: 200
    readonly property int lr2ClickDuration: 200
    readonly property int lr2ScrollUp: 1
    readonly property int lr2ScrollDown: 2
    readonly property var clearTypePriorities: ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"]

    readonly property int count: items.length
    readonly property int logicalCount: realItemCount > 0 ? realItemCount : count
    readonly property var current: logicalCount > 0 ? items[currentIndex] : null
    readonly property int visualDirection: scrollDirection === lr2ScrollDown ? 1 : (scrollDirection === lr2ScrollUp ? -1 : 0)
    readonly property bool visualMoveActive: Math.abs(nowBarFixed - listTopbarFixed) > 0
    readonly property real normalizedVisualIndex: normalizedVisualIndexFor(visualIndex)
    readonly property int visualBaseIndex: Math.floor(normalizedVisualIndex)
    readonly property real scrollOffset: normalizedVisualIndex - visualBaseIndex

    signal openedFolder()
    function touch() {
        revision += 1;
    }

    function touchList() {
        listRevision += 1;
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
        listCalculatedBarFixed = wrapBarFixed(listTopbarFixed);
        deferVisualSelectionSync = true;
        visualIndex = listCalculatedBarFixed / 1000.0;
        targetVisualIndex = nowBarFixed / 1000.0;
        animationStartVisualIndex = oldBarFixed / 1000.0;
        deferVisualSelectionSync = false;
        return syncCurrentToVisual();
    }

    function setVisualIndexImmediate(index) {
        let fixed = Math.round(index * 1000);
        listTopbarFixed = fixed;
        oldBarFixed = fixed;
        nowBarFixed = fixed;
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
        }
        listTopbarFixed = lr2ChangeValueByTime(oldBarFixed, nowBarFixed, barMoveStartMs, barMoveEndMs, now);
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
        let titleDiff = entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
        if (titleDiff !== 0) {
            return titleDiff;
        }
        return compareByDifficulty(a, b);
    }

    function compareByDifficulty(a, b) {
        let levelDiff = entryPlayLevel(a) - entryPlayLevel(b);
        if (levelDiff !== 0) {
            return levelDiff;
        }
        return entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
    }

    function compareByClear(a, b) {
        let lampDiff = entryLamp(b) - entryLamp(a);
        if (lampDiff !== 0) {
            return lampDiff;
        }
        return compareByDifficulty(a, b);
    }

    function compareByScore(a, b) {
        return entryScoreRate(a) - entryScoreRate(b);
    }

    function compareCharts(a, b) {
        switch (sortMode) {
        case 1:
            return compareByDifficulty(a, b);
        case 2:
            return compareByTitle(a, b);
        case 3:
            return compareByClear(a, b);
        case 4:
            return compareByScore(a, b);
        default:
            return 0;
        }
    }

    function sortFilter(input) {
        let folders = [];
        let charts = [];
        for (let item of input) {
            if (isChart(item) || isEntry(item)) {
                if (!chartFilterMatches(item)) {
                    continue;
                }
                charts.push(item);
            } else {
                folders.push(item);
            }
        }
        charts = difficultyFilteredCharts(charts);
        if (sortMode !== 0) {
            charts.sort(compareCharts);
        }
        let result = folders.concat(charts);
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
                refreshSelectedScoreState();
                scoreRevision += 1;
                touch();
            });
            refreshPreviewFiles();
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
            refreshSelectedScoreState();
            scoreRevision += 1;
            touch();
        });
        refreshPreviewFiles();
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

    function applyLr2ScrollDelta(entries, durationMs, now) {
        if (logicalCount === 0 || entries === 0) {
            return;
        }
        listTopbarFixed = lr2ChangeValueByTime(oldBarFixed, nowBarFixed, barMoveStartMs, barMoveEndMs, now);
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
        updateVisualIndex(now);
        applyLr2ScrollDelta(entries, duration, now);
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
        switch (sortMode) {
        case 1: return "LEVEL";
        case 2: return "TITLE";
        case 3: return "CLEAR";
        case 4: return "SCORE";
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

    function statsForScore(score) {
        if (!score || !score.result) {
            return null;
        }
        let counts = score.result.judgementCounts || [];
        let pg = judgementCount(counts, Judgement.Perfect);
        let gr = judgementCount(counts, Judgement.Great);
        let gd = judgementCount(counts, Judgement.Good);
        let bd = judgementCount(counts, Judgement.Bad);
        let pr = judgementCount(counts, Judgement.Poor) + judgementCount(counts, Judgement.EmptyPoor);
        return {
            pg: pg,
            gr: gr,
            gd: gd,
            bd: bd,
            pr: pr,
            totalJudgements: Math.max(1, pg + gr + gd + bd + pr),
            badPoor: bd + pr,
            maxCombo: score.result.maxCombo || 0,
            score: score.result.points || 0,
            exscore: score.result.points || 0,
            maxPoints: score.result.maxPoints || 0
        };
    }

    function bestStats(scoreList) {
        if (!scoreList || scoreList.length === 0) {
            return null;
        }
        return statsForScore(bestScoreByPoints(scoreList));
    }

    function scoreCounts(scoreList) {
        if (!scoreList || scoreList.length === 0) {
            return emptyScoreCounts;
        }
        let counts = {
            play: 0,
            clear: 0,
            fail: 0,
            easy: 0,
            normal: 0,
            hard: 0,
            fc: 0,
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
            } else if (clearType === "AEASY" || clearType === "EASY") {
                ++counts.easy;
            } else if (clearType === "NORMAL") {
                ++counts.normal;
            } else if (clearType === "HARD" || clearType === "EXHARD") {
                ++counts.hard;
            } else if (clearType === "FC" || clearType === "PERFECT" || clearType === "MAX") {
                ++counts.fc;
            }
            let stats = statsForScore(score);
            if (stats) {
                minBadPoor = minBadPoor < 0 ? stats.badPoor : Math.min(minBadPoor, stats.badPoor);
            }
        }
        counts.minBadPoor = Math.max(0, minBadPoor);
        return counts;
    }

    function appendScoreOptionIds(score, ids) {
        if (!score || !score.result) {
            return;
        }
        switch (clearTypeOf(score)) {
        case "AEASY":
        case "EASY":
            ids.push(121);
            break;
        case "NORMAL":
            ids.push(118);
            break;
        case "HARD":
        case "EXHARD":
            ids.push(119);
            break;
        case "FC":
            ids.push(120);
            break;
        case "PERFECT":
        case "MAX":
            ids.push(122);
            break;
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
        let clear = getClearType(cachedEntryScores(item));
        return clearTypeLamp(clear);
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
        let item = current;
        let nextChartData = rankingMode && rankingBaseItem
            ? rankingBaseItem
            : ((isChart(item) || isEntry(item)) ? item : null);
        let scoreList = entryScores(nextChartData || item);
        let nextBestStats = bestStats(scoreList);
        let nextScoreCounts = scoreCounts(scoreList);
        let nextScoreOptionIds = scoreOptionIdsFromList(scoreList);
        cachedSelectedChartData = nextChartData;
        selectedScoreList = scoreList;
        selectedBestStats = nextBestStats;
        selectedScoreCounts = nextScoreCounts;
        selectedScoreOptionIds = nextScoreOptionIds;
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
        return Math.round(total * 100 / rankingPlayerCount);
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
        rankingSavedVisualIndex = visualIndex;
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
        let hasRankingStats = rankingStatsAvailable();
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
            return difficultyPlayLevel(num - 44);
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
            return rankingEntry ? Math.round(rankingEntry.maxPoints / 2) : (chart ? chart.normalNoteCount + chart.scratchCount + chart.lnCount + chart.bssCount : 0);
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
        case 80:
            return stats ? stats.pg : 0;
        case 81:
            return stats ? stats.gr : 0;
        case 82:
            return stats ? stats.gd : 0;
        case 83:
            return stats ? stats.bd : 0;
        case 84:
            return stats ? stats.pr : 0;
        case 85:
            return stats ? Math.round(stats.pg * 100 / stats.totalJudgements) : 0;
        case 86:
            return stats ? Math.round(stats.gr * 100 / stats.totalJudgements) : 0;
        case 87:
            return stats ? Math.round(stats.gd * 100 / stats.totalJudgements) : 0;
        case 88:
            return stats ? Math.round(stats.bd * 100 / stats.totalJudgements) : 0;
        case 89:
            return stats ? Math.round(stats.pr * 100 / stats.totalJudgements) : 0;
        case 92:
            return hasRankingStats ? rankingPlayerRank : 0;
        case 93:
            return hasRankingStats ? rankingPlayerCount : 0;
        case 94:
            return hasRankingStats ? rankingClearPercent("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX") : 0;
        case 200:
            return hasRankingStats ? rankingPlayerCount : counts.play;
        case 201:
            return hasRankingStats ? rankingTotalPlayCount : counts.play;
        case 210:
            return hasRankingStats ? rankingClearCountValue("FAILED") : counts.fail;
        case 211:
            return hasRankingStats ? rankingClearPercent("FAILED") : (counts.play > 0 ? Math.round(counts.fail * 100 / counts.play) : 0);
        case 212:
            return hasRankingStats ? rankingClearCountValue("AEASY", "EASY") : counts.easy;
        case 213:
            return hasRankingStats ? rankingClearPercent("AEASY", "EASY") : (counts.play > 0 ? Math.round(counts.easy * 100 / counts.play) : 0);
        case 214:
            return hasRankingStats ? rankingClearCountValue("NORMAL") : counts.normal;
        case 215:
            return hasRankingStats ? rankingClearPercent("NORMAL") : (counts.play > 0 ? Math.round(counts.normal * 100 / counts.play) : 0);
        case 216:
            return hasRankingStats ? rankingClearCountValue("HARD", "EXHARD") : counts.hard;
        case 217:
            return hasRankingStats ? rankingClearPercent("HARD", "EXHARD") : (counts.play > 0 ? Math.round(counts.hard * 100 / counts.play) : 0);
        case 218:
            return hasRankingStats ? rankingClearCountValue("FC", "PERFECT", "MAX") : counts.fc;
        case 219:
            return hasRankingStats ? rankingClearPercent("FC", "PERFECT", "MAX") : (counts.play > 0 ? Math.round(counts.fc * 100 / counts.play) : 0);
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
