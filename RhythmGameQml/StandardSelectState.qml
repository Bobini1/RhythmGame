pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

// Reusable song/table browsing state. It owns folders, history, scores,
// filtering and activation while leaving presentation to the skin.
Item {
    id: root

    property int minimumEntryCount: 0
    // Filtered, sorted and optionally repeated for the presentation model.
    property var entries: []
    // The raw contents used to recompute entries and folder summaries.
    property var folderContents: []
    property var historyStack: []
    property var pendingScoreDbReplies: []
    property var scores: ({})
    property var previewFiles: ({})
    property var folderClearStats: []
    property int focusedIndex: 0
    property var focusedItem: null
    readonly property var searchHistoryEntry: ({ "kind": "search" })

    readonly property var generalVars: Rg.profileList.mainProfile.vars.generalVars
    readonly property bool arenaSeated:
        Rg.arenaSession.state === ArenaSession.InRoom
        || Rg.arenaSession.state === ArenaSession.Reconnecting

    signal focusRequested(int index)
    signal openedFolder()

    onOpenedFolder: refresh()

    ChartFolderModel {
        id: chartFolderModel

        sortMode: root.generalVars.selectSortMode
        keymodeFilter: root.generalVars.selectKeymodeFilter
        unscoredItemsLast: true
        scores: root.scores

        onSortModeChanged: Qt.callLater(root.sortOrFilterChanged)
        onKeymodeFilterChanged: Qt.callLater(root.sortOrFilterChanged)
        onDifficultyFilterChanged: Qt.callLater(root.sortOrFilterChanged)
        onUnscoredItemsLastChanged: Qt.callLater(root.sortOrFilterChanged)
        onScoresChanged: {
            if (chartFolderModel.sortModeUsesScores()) {
                Qt.callLater(root.sortOrFilterChanged);
            }
        }
    }

    AudioPlayer {
        id: closeFolderSound
        source: root.generalVars.soundsetPath + "f-close"
    }

    AudioPlayer {
        id: openFolderSound
        source: root.generalVars.soundsetPath + "f-open"
    }

    function setFocused(index, item) {
        focusedIndex = index;
        focusedItem = item;
    }

    function requestFocus(index) {
        focusedIndex = index;
        focusRequested(index);
    }

    function trackScoreDbReply(reply) {
        if (!reply || reply.resultAvailable) {
            return reply;
        }
        pendingScoreDbReplies.push(reply);
        let forget = function() {
            reply.finished.disconnect(forget);
            let index = pendingScoreDbReplies.indexOf(reply);
            if (index >= 0) {
                pendingScoreDbReplies.splice(index, 1);
                pendingScoreDbReplies = pendingScoreDbReplies.slice();
            }
        };
        reply.finished.connect(forget);
        return reply;
    }

    function cancelScoreDbReplies() {
        let replies = pendingScoreDbReplies;
        pendingScoreDbReplies = [];
        for (let reply of replies) {
            if (reply && !reply.resultAvailable) {
                reply.cancel();
            }
        }
    }

    function refresh() {
        refreshScores();
        refreshFolderClearStats();
    }

    function folderForHistoryItem(item) {
        return item instanceof ChartData ? item.chartDirectory : item;
    }

    function refreshScores() {
        cancelScoreDbReplies();
        if (historyStack[historyStack.length - 1] === searchHistoryEntry) {
            let md5s = [];
            for (let item of folderContents) {
                if (typeof item === "object" && "md5" in item) {
                    md5s.push(item.md5);
                }
            }
            trackScoreDbReply(
                Rg.profileList.mainProfile.scoreDb.getScoresForMd5(md5s)
            ).then(result => scores = result.scores);
        } else {
            trackScoreDbReply(
                Rg.profileList.mainProfile.scoreDb.getScores(
                    folderForHistoryItem(historyStack[historyStack.length - 1]))
            ).then(result => {
                if (result instanceof tableQueryResult) {
                    let newScores = result.scores.scores;
                    for (let [key, value] of Object.entries(result.courseScores.scores)) {
                        newScores[key] = value;
                    }
                    scores = newScores;
                } else {
                    scores = result.scores;
                }
            });
        }
        let dirs = [];
        for (let item of folderContents) {
            if (item instanceof ChartData) {
                dirs.push(item.chartDirectory);
            }
        }
        previewFiles = Rg.songDirectoryFilePathFetcher.getPreviewFilePaths(dirs);
    }

    function clearStatsFromScoreSummary(summary) {
        let counts = summary?.counts || {};
        return {
            "NOPLAY": counts.NOPLAY || 0,
            "FAILED": counts.FAILED || 0,
            "AEASY": counts.AEASY || 0,
            "LIGHTASSIST": counts.LIGHTASSIST || counts.LIGHT_ASSIST || 0,
            "EASY": counts.EASY || 0,
            "NORMAL": counts.NORMAL || 0,
            "HARD": counts.HARD || 0,
            "EXHARD": counts.EXHARD || 0,
            "FC": counts.FC || 0,
            "PERFECT": counts.PERFECT || 0,
            "MAX": counts.MAX || 0
        };
    }

    function refreshFolderClearStats() {
        folderClearStats = [];
        for (let folder of folderContents) {
            if (folder instanceof ChartData || folder instanceof entry
                    || folder instanceof course || folder === null) {
                continue;
            }
            trackScoreDbReply(
                Rg.profileList.mainProfile.scoreDb.getScoreSummary(folder)
            ).then(result => {
                folderClearStats.push([folder, clearStatsFromScoreSummary(result)]);
                folderClearStats = folderClearStats.slice();
            });
        }
    }

    function addToMinimumCount(input) {
        let length = input.length;
        if (length === 0 || length >= minimumEntryCount) {
            return input;
        }
        let limit = Math.ceil(minimumEntryCount / length) * length;
        for (let i = length; i < limit; ++i) {
            input.push(input[i % length] || null);
        }
        return input;
    }

    function preparedEntries(input) {
        return addToMinimumCount(chartFolderModel.filterAndSort(input));
    }

    function sameEntry(a, b) {
        if (a instanceof ChartData && b instanceof ChartData) {
            return a.path === b.path;
        }
        if (typeof a === "string" && typeof b === "string") {
            return a === b;
        }
        if (a instanceof level && b instanceof level) {
            return a.name === b.name;
        }
        if (a instanceof table && b instanceof table) {
            return String(a.url || "") === String(b.url || "");
        }
        if (a instanceof course && b instanceof course) {
            return a.identifier === b.identifier;
        }
        return a === b;
    }

    function indexOfEntry(items, target) {
        for (let i = 0; i < items.length; ++i) {
            if (sameEntry(items[i], target)) {
                return i;
            }
        }
        return -1;
    }

    function openPlayable(item, autoplay = false, replay = false,
                          replayScore = null) {
        if (item instanceof ChartData) {
            if (arenaSeated) {
                if (!autoplay && !replay && !replayScore) {
                    Rg.arenaSession.selectChart(item);
                }
                return true;
            }
            console.info("Opening chart " + item.path);
            let useReplay = !!replay && !!replayScore;
            if (Rg.profileList.battleActive) {
                globalRoot.openChart(item.path,
                    Rg.profileList.battleProfiles.player1Profile,
                    !!autoplay, useReplay, replayScore || null,
                    Rg.profileList.battleProfiles.player2Profile,
                    !!autoplay, false, null);
            } else {
                globalRoot.openChart(item.path, Rg.profileList.mainProfile,
                    !!autoplay, useReplay, replayScore || null,
                    null, false, false, null);
            }
            return true;
        }
        if (item instanceof course) {
            if (arenaSeated) {
                return true;
            }
            let useReplay = !!replay && !!replayScore;
            if (Rg.profileList.battleActive) {
                globalRoot.openCourse(item,
                    Rg.profileList.battleProfiles.player1Profile,
                    !!autoplay, useReplay, replayScore || null,
                    Rg.profileList.battleProfiles.player2Profile,
                    !!autoplay, false, null);
            } else {
                globalRoot.openCourse(item, Rg.profileList.mainProfile,
                    !!autoplay, useReplay, replayScore || null,
                    null, false, false, null);
            }
            return true;
        }
        return false;
    }

    function open(item) {
        item = folderForHistoryItem(item);
        let folder;
        if (item instanceof table) {
            folder = [...item.levels, ...item.courses];
        } else if (item instanceof level) {
            folder = item.loadCharts();
        } else if (typeof item === "string") {
            folder = [];
            if (item === "") {
                for (let tableItem of Rg.tables.getList()) {
                    if (tableItem.status === table.Loaded) {
                        folder.push(tableItem);
                    }
                }
            }
            folder.push(...Rg.songFolderFactory.open(item));
            if (item !== "" && folder.length === 0) {
                folder.push(...Rg.songFolderFactory.openChartDirectory(item));
            }
        } else {
            return [];
        }
        folderContents = [...folder];
        entries = preparedEntries(folder);
        openedFolder();
        return entries;
    }

    function goBack() {
        if (historyStack.length === 1) {
            return false;
        }
        let last = historyStack.pop();
        let folder = open(historyStack[historyStack.length - 1]);
        let index = indexOfEntry(folder, last);
        requestFocus(index >= 0 ? index : 0);
        closeFolderSound.stop();
        closeFolderSound.play();
        return true;
    }

    function goForward(item, skipSound = false) {
        if (openPlayable(item, false, false, null)) {
            return true;
        }
        if (item instanceof entry || item === null) {
            return false;
        }
        historyStack.push(item);
        open(item);
        if (!skipSound) {
            openFolderSound.stop();
            openFolderSound.play();
        }
        requestFocus(0);
        return true;
    }

    function selectedSongFolderPath() {
        if (focusedItem instanceof ChartData && focusedItem.chartDirectory) {
            return focusedItem.chartDirectory;
        }
        if (typeof focusedItem === "string") {
            return focusedItem;
        }
        for (let i = historyStack.length - 1; i >= 0; --i) {
            if (typeof historyStack[i] === "string") {
                return historyStack[i];
            }
        }
        return "";
    }

    function reloadCurrentFolderOrTable() {
        if (globalRoot.reloadTableForItem(focusedItem)) {
            return true;
        }
        if (historyStack.length === 0) {
            return false;
        }
        for (let i = historyStack.length - 1; i >= 0; --i) {
            if (globalRoot.reloadTableForItem(historyStack[i])) {
                return true;
            }
        }
        if (globalRoot.scanRootSongFolderForPath(selectedSongFolderPath())) {
            return true;
        }
        let old = focusedItem;
        let folder = open(historyStack[historyStack.length - 1]);
        let index = indexOfEntry(folder, old);
        requestFocus(index >= 0 ? index : 0);
        return true;
    }

    function openChartDirectory(directory, initialItem) {
        if (!directory) {
            return false;
        }
        let folder = Rg.songFolderFactory.openChartDirectory(directory);
        if (!folder.length) {
            return false;
        }
        if (historyStack.length === 0
                || folderForHistoryItem(historyStack[historyStack.length - 1])
                   !== directory) {
            historyStack.push(initialItem || directory);
        }
        folderContents = [...folder];
        entries = preparedEntries(folder);
        openedFolder();
        let index = chartFolderModel.indexOfItem(entries, initialItem);
        requestFocus(index >= 0 ? index : 0);
        return true;
    }

    function search(query) {
        let results = Rg.songFolderFactory.search(query);
        let resultCount = results.length;
        if (!results.length) {
            console.info("Search returned no results");
            return resultCount;
        }
        folderContents = [...results];
        entries = preparedEntries(results);
        if (historyStack[historyStack.length - 1] !== searchHistoryEntry) {
            historyStack.push(searchHistoryEntry);
        }
        requestFocus(0);
        openedFolder();
        return resultCount;
    }

    function sortOrFilterChanged() {
        if (!folderContents.length) {
            return;
        }
        let old = focusedItem;
        entries = preparedEntries(folderContents);
        let index = chartFolderModel.indexOfItem(entries, old);
        requestFocus(index >= 0 ? index : 0);
    }

    function isChartItem(item) {
        return item instanceof ChartData || item instanceof entry;
    }

    function openSelectedFolder() {
        if (focusedItem instanceof ChartData && focusedItem.chartDirectory) {
            return globalRoot.openLocalFolder(focusedItem.chartDirectory);
        }
        if (typeof focusedItem === "string") {
            return globalRoot.openLocalFolder(focusedItem);
        }
        return false;
    }

    function openSelectedReadme() {
        if (!(focusedItem instanceof ChartData) || !focusedItem.chartDirectory) {
            return false;
        }
        let paths = Rg.songDirectoryFilePathFetcher.getReadmeFilePaths(
            [focusedItem.chartDirectory]);
        let path = paths[focusedItem.chartDirectory] || "";
        let localPath = path.length > 0 ? Rg.songAssets.localFile(path) : "";
        return localPath.length > 0
            && Qt.openUrlExternally(globalRoot.localFileUrl(localPath));
    }

    function showAllChartsForCurrentSong() {
        return focusedItem instanceof ChartData
            && !!focusedItem.chartDirectory
            && openChartDirectory(focusedItem.chartDirectory, focusedItem);
    }

    Component.onDestruction: cancelScoreDbReplies()
}
