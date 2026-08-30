pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

// Reusable song/table browsing state. It owns folders, history, scores,
// filtering and activation while leaving presentation to the skin.
Item {
    id: root

    // Filtered and sorted logical entries. Presentation adapters may repeat it.
    property var entries: []
    property alias folderContents: sessionImpl.folderContents
    property alias historyStack: sessionImpl.historyStack
    property alias pendingScoreDbReplies: sessionImpl.pendingScoreDbReplies
    property alias scores: sessionImpl.scores
    property alias previewFiles: sessionImpl.previewFiles
    property alias session: sessionImpl
    property alias activation: activationImpl
    property var tryOpenPlayableAction: null
    property var folderClearStats: []
    property int focusedIndex: 0
    property var focusedItem: null
    readonly property var searchHistoryEntry: ({ "kind": "search" })

    readonly property var generalVars: Rg.profileList.mainProfile.vars.generalVars
    signal focusRequested(int index)
    signal openedFolder()
    signal enteredFolder()
    signal leftFolder()

    StandardSelectSession {
        id: sessionImpl
    }

    StandardSelectActivation {
        id: activationImpl
    }

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

    function setFocused(item) {
        let logicalIndex = indexOfEntry(entries, item);
        focusedIndex = logicalIndex;
        focusedItem = logicalIndex >= 0 ? entries[logicalIndex] : null;
    }

    function requestFocus(index) {
        focusedIndex = index;
        focusedItem = index >= 0 && index < entries.length
            ? entries[index] : null;
        focusRequested(index);
    }

    function trackScoreDbReply(reply) {
        return session.trackScoreDbReply(reply);
    }

    function cancelScoreDbReplies() {
        session.cancelScoreDbReplies();
    }

    function refresh() {
        refreshScores();
        refreshFolderClearStats();
    }

    function folderForHistoryItem(item) {
        return session.folderForHistoryItem(item);
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

    function preparedEntries(input) {
        return chartFolderModel.filterAndSort(input);
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
        if (typeof tryOpenPlayableAction === "function"
                && tryOpenPlayableAction(
                    item, autoplay, replay, replayScore)) {
            return true;
        }
        return activation.openPlayable(item, autoplay, replay, replayScore);
    }

    function open(item) {
        let folder = session.resolveFolderContents(item);
        if (folder === null) {
            return null;
        }
        session.commitFolderContents(folder);
        entries = preparedEntries(folder);
        publishFolderContents();
        return entries;
    }

    function publishFolderContents() {
        refresh();
        openedFolder();
    }

    function initialize() {
        if (historyStack.length !== 0) {
            return false;
        }
        historyStack = [""];
        if (open("") === null) {
            historyStack = [];
            return false;
        }
        requestFocus(0);
        return true;
    }

    function goBack() {
        if (historyStack.length < 2) {
            return false;
        }
        let oldHistory = historyStack;
        let last = oldHistory[oldHistory.length - 1];
        historyStack = oldHistory.slice(0, oldHistory.length - 1);
        let folder = open(historyStack[historyStack.length - 1]);
        if (folder === null) {
            historyStack = oldHistory;
            return false;
        }
        let index = indexOfEntry(folder, last);
        requestFocus(index >= 0 ? index : 0);
        leftFolder();
        return true;
    }

    function goForward(item) {
        if (openPlayable(item, false, false, null)) {
            return true;
        }
        if (item instanceof entry || item === null) {
            return false;
        }
        let oldHistory = historyStack;
        historyStack = historyStack.concat([item]);
        if (open(item) === null) {
            historyStack = oldHistory;
            return false;
        }
        requestFocus(0);
        enteredFolder();
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
        if (folder === null) {
            return false;
        }
        let index = indexOfEntry(folder, old);
        requestFocus(index >= 0 ? index : 0);
        return true;
    }

    function openChartDirectory(directory, initialItem) {
        let folder = session.resolveChartDirectory(directory);
        if (!folder || !folder.length) {
            return false;
        }
        if (historyStack.length === 0
                || folderForHistoryItem(historyStack[historyStack.length - 1])
                   !== directory) {
            historyStack = historyStack.concat([initialItem || directory]);
        }
        session.commitFolderContents(folder);
        entries = preparedEntries(folder);
        publishFolderContents();
        let index = chartFolderModel.indexOfItem(entries, initialItem);
        requestFocus(index >= 0 ? index : 0);
        return true;
    }

    function search(query) {
        let results = session.resolveSearchResults(query);
        let resultCount = results.length;
        if (!results.length) {
            console.info("Search returned no results");
            return resultCount;
        }
        session.commitFolderContents(results);
        entries = preparedEntries(results);
        if (historyStack[historyStack.length - 1] !== searchHistoryEntry) {
            historyStack = historyStack.concat([searchHistoryEntry]);
        }
        requestFocus(0);
        publishFolderContents();
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

    Component.onCompleted: initialize()
}
