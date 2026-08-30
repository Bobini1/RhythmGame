pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectState
    \inqmlmodule RhythmGameQml
    \brief Owns reusable song and table browsing state.

    The component owns folders, history, scores, filtering, sorting, and
    activation while leaving the list and focus presentation to the skin.
*/
Item {
    id: root

    /*! Filtered and sorted logical entries before presentation adaptation. */
    property var entries: []
    /*! Raw contents of the current folder, table, level, or search. */
    property alias folderContents: sessionImpl.folderContents
    /*! Navigation history for the current selection session. */
    property alias historyStack: sessionImpl.historyStack
    /*! Score-database replies owned by this state. */
    property alias pendingScoreDbReplies: sessionImpl.pendingScoreDbReplies
    /*! Score data loaded for the current folder contents. */
    property alias scores: sessionImpl.scores
    /*! Preview-file data loaded for the current folder contents. */
    property alias previewFiles: sessionImpl.previewFiles
    /*! Lower-level selection-session component. */
    property alias session: sessionImpl
    /*! Lower-level chart and course activation component. */
    property alias activation: activationImpl
    /*! Optional opening pre-handler. True consumes; false or undefined continues. */
    property var tryOpenPlayableAction: null
    /*! Clear statistics for folders in \l entries. */
    property var folderClearStats: []
    /*! Index of \l focusedItem in \l entries. */
    property int focusedIndex: 0
    /*! Logical item currently focused by the skin. */
    property var focusedItem: null
    readonly property var searchHistoryEntry: ({ "kind": "search" })

    readonly property var generalVars: Rg.profileList.mainProfile.vars.generalVars
    /*! Requests that the skin focus \a index in \l entries. */
    signal focusRequested(int index)
    /*! Emitted after a folder or table has been opened. */
    signal openedFolder()
    /*! Emitted after entering a selection-history item. */
    signal enteredFolder()
    /*! Emitted after leaving a selection-history item. */
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

    /*! Updates the logical focused \a item after the skin moves focus. */
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

    /*! Rebuilds \l entries and their associated data. */
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

    /*!
        Opens \a item using \a autoplay, \a replay, and \a replayScore to
        select the requested play mode.
    */
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

    /*! Initializes history from the configured song folders and tables. */
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

    /*! Leaves the current history entry. Returns whether navigation occurred. */
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

    /*! Opens \a item as a folder or playable item. */
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

    /*! Reloads the current local folder or online table. */
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

    /*! Opens \a directory and initially focuses \a initialItem. */
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

    /*! Replaces the current entries with results for \a query. */
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

    /*! Rebuilds entries after a sort or filter setting changes. */
    function sortOrFilterChanged() {
        if (!folderContents.length) {
            return;
        }
        let old = focusedItem;
        entries = preparedEntries(folderContents);
        let index = chartFolderModel.indexOfItem(entries, old);
        requestFocus(index >= 0 ? index : 0);
    }

    /*! Returns whether \a item is a playable chart. */
    function isChartItem(item) {
        return item instanceof ChartData || item instanceof entry;
    }

    /*! Opens the focused chart's directory in the system file browser. */
    function openSelectedFolder() {
        if (focusedItem instanceof ChartData && focusedItem.chartDirectory) {
            return globalRoot.openLocalFolder(focusedItem.chartDirectory);
        }
        if (typeof focusedItem === "string") {
            return globalRoot.openLocalFolder(focusedItem);
        }
        return false;
    }

    /*! Opens the README associated with the focused item, if one exists. */
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

    /*! Shows every chart that belongs to the currently focused song. */
    function showAllChartsForCurrentSong() {
        return focusedItem instanceof ChartData
            && !!focusedItem.chartDirectory
            && openChartDirectory(focusedItem.chartDirectory, focusedItem);
    }

    Component.onCompleted: initialize()
}
