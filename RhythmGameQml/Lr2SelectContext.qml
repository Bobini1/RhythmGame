pragma ValueTypeBehavior: Addressable

import QtQuick
import RhythmGameQml 1.0

Item {
    id: root

    property var items: []
    property var folderContents: []
    property var historyStack: []
    property var scores: ({})
    property var previewFiles: ({})
    property int currentIndex: 0
    property int targetIndex: currentIndex
    property real visualIndex: 0
    property real targetVisualIndex: 0
    property int selectedOffset: 0
    property real animationStartVisualIndex: 0
    property double barMoveStartMs: 0
    property double barMoveEndMs: 0
    property int revision: 0
    property int scoreRevision: 0
    property bool scrollingText: false
    property int difficultyFilter: 0
    property int keyFilter: 0
    property int sortMode: 0
    readonly property int lr2SpeedFirst: 300
    readonly property int lr2SpeedNext: 70
    readonly property int lr2WheelDuration: 200
    readonly property int lr2ClickDuration: 200
    readonly property var clearTypePriorities: ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"]

    readonly property int count: items.length
    readonly property var current: count > 0 ? items[currentIndex] : null
    readonly property int visualDirection: targetVisualIndex > visualIndex + 0.001 ? 1 : (targetVisualIndex < visualIndex - 0.001 ? -1 : 0)
    readonly property bool visualMoveActive: Math.abs(targetVisualIndex - visualIndex) > 0.001
    readonly property real normalizedVisualIndex: normalizedVisualIndexFor(visualIndex)
    readonly property int visualBaseIndex: Math.floor(normalizedVisualIndex)
    readonly property real scrollOffset: normalizedVisualIndex - visualBaseIndex

    signal openedFolder()

    function touch() {
        revision += 1;
    }

    function setVisualIndexImmediate(index) {
        visualIndex = index;
        targetVisualIndex = index;
        animationStartVisualIndex = index;
        barMoveStartMs = 0;
        barMoveEndMs = 0;
    }

    function beginVisualMove(durationMs, now) {
        if (now === undefined) {
            now = Date.now();
            updateVisualIndex(now);
        }
        animationStartVisualIndex = visualIndex;
        barMoveStartMs = now;
        barMoveEndMs = now + Math.max(1, durationMs);
    }

    Timer {
        id: visualScrollTimer
        interval: 1
        repeat: true
        running: root.visualMoveActive

        onTriggered: root.advanceVisualIndex()
    }

    function advanceVisualIndex() {
        updateVisualIndex(Date.now());
    }

    function normalizedVisualIndexFor(index) {
        if (count === 0) {
            return 0;
        }
        return ((index % count) + count) % count;
    }

    function baseIndexForVisual(index) {
        return Math.floor(normalizedVisualIndexFor(index));
    }

    function visualRemainderFor(index) {
        let normalized = normalizedVisualIndexFor(index);
        return normalized - Math.floor(normalized);
    }

    function cursorBaseIndexForVisual(index) {
        let base = baseIndexForVisual(index);
        // OpenLR2 renders with floor(listCalculatedBar), but GetSongCursor()
        // advances by one while scrolling down and the fixed-point remainder is
        // non-zero. This keeps the selected item identity in lockstep with LR2
        // without changing the stationary glow row.
        if (visualRemainderFor(index) > 0.001 && targetVisualIndex > index + 0.001) {
            base += 1;
        }
        return base;
    }

    function syncCurrentToVisual() {
        if (count === 0) {
            return;
        }
        let nextIndex = normalizeIndex(cursorBaseIndexForVisual(visualIndex) + selectedOffset);
        if (currentIndex === nextIndex) {
            return;
        }
        currentIndex = nextIndex;
        targetIndex = nextIndex;
        scrollingText = false;
        scrollingTextTimer.restart();
        touch();
    }

    function updateVisualIndex(now) {
        let distance = targetVisualIndex - visualIndex;
        if (Math.abs(distance) <= 0.001) {
            visualIndex = targetVisualIndex;
            syncCurrentToVisual();
            return;
        }

        if (barMoveEndMs <= barMoveStartMs || now >= barMoveEndMs) {
            visualIndex = targetVisualIndex;
            syncCurrentToVisual();
            return;
        }

        let progress = Math.max(0, Math.min(1, (now - barMoveStartMs) / (barMoveEndMs - barMoveStartMs)));
        visualIndex = animationStartVisualIndex + (targetVisualIndex - animationStartVisualIndex) * progress;
        syncCurrentToVisual();
    }

    function nearestVisualIndex(index, anchor) {
        if (count === 0) {
            return 0;
        }
        let target = index;
        while (target - anchor > count / 2) {
            target -= count;
        }
        while (target - anchor < -count / 2) {
            target += count;
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
            return keymode === 5 || keymode === 7 || keymode === 9;
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
        case 7:
            return keymode === 9;
        default:
            return true;
        }
    }

    function difficultyFilterMatches(item) {
        return !isChart(item) || difficultyFilter === 0 || item.difficulty === difficultyFilter;
    }

    function chartFilterMatches(item) {
        return keyFilterMatches(item) && difficultyFilterMatches(item);
    }

    function compareCharts(a, b) {
        if (sortMode === 1) {
            return entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
        }
        if (sortMode === 2) {
            let levelDiff = entryPlayLevel(a) - entryPlayLevel(b);
            if (levelDiff !== 0) {
                return levelDiff;
            }
            return entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
        }
        if (sortMode === 3) {
            let lampDiff = entryLamp(a) - entryLamp(b);
            if (lampDiff !== 0) {
                return lampDiff;
            }
            return entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
        }
        if (sortMode === 4) {
            let rankDiff = entryRank(b) - entryRank(a);
            if (rankDiff !== 0) {
                return rankDiff;
            }
            return entryDisplayName(a, true).localeCompare(entryDisplayName(b, true));
        }
        return 0;
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
        if (sortMode !== 0) {
            charts.sort(compareCharts);
        }
        let result = folders.concat(charts);
        if (result.length === 0) {
            result.push(null);
        }
        return result;
    }

    function sortOrFilterChanged() {
        if (!folderContents.length) {
            return;
        }
        let old = current;
        let sortedFiltered = sortFilter(folderContents);
        addToMinimumCount(sortedFiltered);
        items = sortedFiltered;
        let currentIdx = sortedFiltered.findIndex((item) => sameEntry(item, old));
        currentIndex = currentIdx >= 0 ? currentIdx : 0;
        targetIndex = currentIndex;
        selectedOffset = 0;
        setVisualIndexImmediate(currentIndex);
        scrollingText = false;
        scrollingTextTimer.restart();
        touch();
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
            scoreRevision += 1;
            touch();
        });
        refreshPreviewFiles();
    }

    function open(item) {
        let folder;
        if (isTable(item)) {
            let courses = item.courses;
            folder = [...item.levels, ...courses];
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
        folder = sortFilter(folder);
        addToMinimumCount(folder);
        items = folder;
        currentIndex = 0;
        targetIndex = 0;
        selectedOffset = 0;
        setVisualIndexImmediate(0);
        refreshScores();
        openedFolder();
        touch();
        return folder;
    }

    function search(query) {
        let results = Rg.songFolderFactory.search(query || "");
        if (!results.length) {
            console.info("Search returned no results");
            return;
        }
        folderContents = [...results];
        results = sortFilter(results);
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
        openedFolder();
        touch();
    }

    function openRoot() {
        historyStack = [];
        goForward("", true);
    }

    function normalizeIndex(index) {
        if (count === 0) {
            return 0;
        }
        return ((index % count) + count) % count;
    }

    function setCurrentIndex(index) {
        if (count === 0) {
            currentIndex = 0;
            targetIndex = 0;
            selectedOffset = 0;
            setVisualIndexImmediate(0);
            touch();
            return;
        }
        let normalized = normalizeIndex(index);
        if (currentIndex === normalized && selectedOffset === 0) {
            return;
        }
        let now = Date.now();
        updateVisualIndex(now);
        selectedOffset = 0;
        currentIndex = normalized;
        targetIndex = normalized;
        targetVisualIndex = nearestVisualIndex(normalized, targetVisualIndex);
        beginVisualMove(lr2ClickDuration, now);
        scrollingText = false;
        scrollingTextTimer.restart();
        touch();
    }

    function scrollBy(entries, durationMs) {
        if (count === 0 || entries === 0) {
            return;
        }
        let now = Date.now();
        let duration = durationMs !== undefined ? durationMs : lr2SpeedFirst;
        updateVisualIndex(now);
        targetVisualIndex += entries;
        let centerTarget = Math.round(targetVisualIndex);
        targetIndex = normalizeIndex(centerTarget + selectedOffset);
        beginVisualMove(duration, now);
    }

    function scrollByKey(entries, repeated) {
        if (count === 0 || entries === 0) {
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
        if (count === 0) {
            return;
        }
        selectedOffset = row - barCenter;
        currentIndex = normalizeIndex(visualBaseIndex + selectedOffset);
        targetIndex = currentIndex;
        scrollingText = false;
        scrollingTextTimer.restart();
        touch();
    }

    function selectedRow(barCenter) {
        return barCenter + selectedOffset;
    }

    function visualSelectedRow(barCenter) {
        return selectedRow(barCenter);
    }

    function visualRowForIndex(index, barCenter) {
        if (count === 0) {
            return barCenter;
        }
        let offset = normalizeIndex(index) - normalizeIndex(visualBaseIndex);
        if (offset > count / 2) {
            offset -= count;
        }
        if (offset < -count / 2) {
            offset += count;
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
        if (historyStack.length <= 1) {
            return;
        }
        let last = historyStack.pop();
        let folder = open(historyStack[historyStack.length - 1]);
        let idx = folder.findIndex((folderItem) => sameEntry(folderItem, last));
        if (idx >= 0) {
            setCurrentIndex(idx);
        }
    }

    function goForward(item, autoplay) {
        if (isChart(item)) {
            if (Rg.profileList.battleActive) {
                globalRoot.openChart(item.path, Rg.profileList.battleProfiles.player1Profile, !!autoplay, false, null, Rg.profileList.battleProfiles.player2Profile, !!autoplay, false, null);
            } else {
                globalRoot.openChart(item.path, Rg.profileList.mainProfile, !!autoplay, false, null, null, false, false, null);
            }
            return;
        }
        if (isCourse(item)) {
            if (Rg.profileList.battleActive) {
                globalRoot.openCourse(item, Rg.profileList.battleProfiles.player1Profile, !!autoplay, false, null, Rg.profileList.battleProfiles.player2Profile, !!autoplay, false, null);
            } else {
                globalRoot.openCourse(item, Rg.profileList.mainProfile, !!autoplay, false, null, null, false, false, null);
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
        if (count === 0) {
            return null;
        }
        return items[normalizeIndex(visualBaseIndex + row - barCenter)];
    }

    function entryDisplayName(item, includeSubtitle) {
        if (!item) {
            return "";
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
        if (isChart(item) || isEntry(item)) {
            return item.title || "";
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
        case 7: return "9KEYS";
        default: return "ALL";
        }
    }

    function sortLabel() {
        switch (sortMode) {
        case 1: return "TITLE";
        case 2: return "LEVEL";
        case 3: return "CLEAR";
        case 4: return "RANK";
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

    function entryDifficulty(item) {
        return isChart(item) ? Math.max(0, item.difficulty || 0) : 0;
    }

    function entryPlayLevel(item) {
        if (isChart(item)) {
            return item.playLevel || 0;
        }
        if (isEntry(item)) {
            let parsed = parseInt(item.level);
            return isNaN(parsed) ? 0 : parsed;
        }
        return 0;
    }

    function entryIdentifier(item) {
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
        return id ? (scores[id] || []) : [];
    }

    function getClearType(scoreList) {
        let clearType = "NOPLAY";
        for (let score of scoreList) {
            let next = score?.result?.clearType || "NOPLAY";
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

    function bestStats(scoreList) {
        let score = bestScoreByPoints(scoreList);
        if (!score || !score.result) {
            return null;
        }
        let counts = score.result.judgementCounts || [];
        return {
            pg: counts[Judgement.Perfect] || 0,
            gr: counts[Judgement.Great] || 0,
            gd: counts[Judgement.Good] || 0,
            bd: counts[Judgement.Bad] || 0,
            pr: (counts[Judgement.Poor] || 0) + (counts[Judgement.EmptyPoor] || 0),
            maxCombo: score.result.maxCombo || 0,
            score: score.result.points || 0,
            exscore: score.result.points || 0,
            maxPoints: score.result.maxPoints || 0
        };
    }

    function entryLamp(item) {
        let clear = getClearType(entryScores(item));
        switch (clear) {
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

    function entryRank(item) {
        let best = bestScoreByPoints(entryScores(item));
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

    function selectedChartData() {
        return isChart(current) ? current : null;
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
        let stats = bestStats(entryScores(current));
        switch (num) {
        case 10:
            return 100;
        case 11:
            return 100;
        case 12:
            return Rg.profileList.mainProfile.vars.generalVars.offset || 0;
        case 13:
            return 0;
        case 45:
        case 46:
        case 47:
        case 48:
        case 49:
            return chart && chart.difficulty === num - 44 ? chart.playLevel : 0;
        case 57:
            return 100;
        case 58:
            return 100;
        case 59:
            return 100;
        case 70:
            return stats ? Math.round(stats.score) : 0;
        case 71:
            return stats ? Math.round(stats.exscore) : 0;
        case 72:
            return stats ? Math.round(stats.maxPoints) : 0;
        case 73:
            return stats && stats.maxPoints > 0 ? Math.round(stats.score * 100 / stats.maxPoints) : 0;
        case 74:
            return chart ? chart.normalNoteCount + chart.scratchCount + chart.lnCount + chart.bssCount : 0;
        case 75:
            return stats ? stats.pg : 0;
        case 76:
            return stats ? stats.gr : 0;
        case 77:
            return stats ? stats.gd : 0;
        case 78:
            return stats ? stats.bd : 0;
        case 79:
            return stats ? stats.pr : 0;
        case 90:
            return chart ? Math.round(chart.maxBpm || chart.mainBpm || 0) : 0;
        case 91:
            return chart ? Math.round(chart.minBpm || chart.mainBpm || 0) : 0;
        default:
            return 0;
        }
    }

    function levelBarValue(diff) {
        let counts = [0, 0, 0, 0, 0, 0];
        for (let item of folderContents) {
            if (isChart(item)) {
                let difficulty = Math.max(0, Math.min(5, item.difficulty || 0));
                counts[difficulty] += 1;
            }
        }
        let maxCount = Math.max(1, counts[1], counts[2], counts[3], counts[4], counts[5]);
        return counts[diff] / maxCount;
    }

    function hasDifficulty(diff) {
        for (let item of folderContents) {
            if (isChart(item) && item.difficulty === diff) {
                return true;
            }
        }
        return selectedChartData() ? selectedChartData().difficulty === diff : false;
    }

    function barGraphValue(type) {
        let stats = bestStats(entryScores(current));
        switch (type) {
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            return levelBarValue(type - 4);
        case 40:
            return stats && stats.maxPoints > 0 ? stats.pg / stats.maxPoints : 0;
        case 41:
            return stats && stats.maxPoints > 0 ? stats.gr / stats.maxPoints : 0;
        case 42:
            return stats && stats.maxPoints > 0 ? stats.gd / stats.maxPoints : 0;
        case 43:
            return stats && stats.maxPoints > 0 ? stats.bd / stats.maxPoints : 0;
        case 44:
            return stats && stats.maxPoints > 0 ? stats.pr / stats.maxPoints : 0;
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
