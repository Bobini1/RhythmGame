pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectState
    \inqmlmodule RhythmGameQml
    \brief Owns reusable song and table browsing state.

    The component owns folders, history, optional metadata enrichment,
    filtering, sorting, and activation while leaving list presentation to the
    skin. Use the lower-level selection components when these policies do not
    fit a custom selector.

    Unless \l autoInitialize is false, construction immediately opens the root
    browser. The skin must handle \l focusRequested and call \l setFocused
    whenever its visual focus changes. \l goForward opens either a folder or a
    playable item; \l goBack performs the matching history transaction.

    \l entries, \l folderContents, \l historyStack, \l scores, and
    \l previewFiles are shallow snapshots. Reordering an array or assigning a
    map key does not modify the selection state, but contained chart and table
    objects are not cloned. Score, preview, and folder-stat enrichment is
    asynchronous; consumers must react to property changes rather than assuming
    all metadata is present when \l openedFolder is emitted.
*/
Item {
    id: root

    /*! Filtered and sorted logical entries before presentation adaptation. */
    readonly property var entries: selectionState.entries.slice()
    /*! Raw contents of the current folder, table, level, or search. */
    readonly property var folderContents: sessionImpl.folderContents.slice()
    /*! Navigation history for the current selection session. */
    readonly property var historyStack: sessionImpl.historyStack.slice()
    /*! Score data loaded for the current folder contents. */
    readonly property var scores: Object.assign({}, sessionImpl.scores)
    /*! Preview-file data loaded for the current folder contents. */
    readonly property var previewFiles:
        Object.assign({}, sessionImpl.previewFiles)
    /*!
        Optional \c tryOpenPlayableAction(item,autoplay,replay,replayScore)
        pre-handler. True consumes the operation.
    */
    property var tryOpenPlayableAction: null
    /*! Whether standard score loading is active. */
    property bool scoresEnabled: true
    /*! Whether standard preview-file discovery is active. */
    property bool previewFilesEnabled: true
    /*! Whether per-folder clear-statistic loading is active. */
    property bool folderClearStatsEnabled: true
    /*! Whether construction automatically opens the root selection folder. */
    property bool autoInitialize: true
    /*! Sort mode used to prepare logical entries. */
    property int sortMode: selectionState.generalVars.selectSortMode
    /*! Key-mode filter used to prepare logical entries. */
    property int keymodeFilter: selectionState.generalVars.selectKeymodeFilter
    /*! Difficulty filter used to prepare logical entries. */
    property int difficultyFilter: 0
    /*! Whether items without scores sort after scored items. */
    property bool unscoredItemsLast: true
    /*! Index of \l focusedItem in \l entries. */
    readonly property int focusedIndex: selectionState.focusedIndex
    /*! Logical item currently focused by the skin. */
    readonly property var focusedItem: selectionState.focusedItem
    QtObject {
        id: selectionState

        property var entries: []
        property int focusedIndex: 0
        property var focusedItem: null
        property var folderClearStatsByKey: ({})
        property int folderClearStatsRevision: 0
        readonly property var generalVars:
            Rg.profileList.mainProfile.vars.generalVars
        readonly property var searchHistoryEntry: ({ "kind": "search" })

        function requestFocus(index) {
            selectionState.focusedIndex = index;
            selectionState.focusedItem = index >= 0
                && index < root.entries.length ? root.entries[index] : null;
            root.focusRequested(index);
        }

        function refreshScores() {
            sessionImpl.cancelScoreDbReplies();
            if (!root.scoresEnabled) {
                sessionImpl.scores = ({});
                return;
            }
            if (root.historyStack[root.historyStack.length - 1]
                    === selectionState.searchHistoryEntry) {
                let md5s = [];
                for (let item of root.folderContents) {
                    if (typeof item === "object" && "md5" in item) {
                        md5s.push(item.md5);
                    }
                }
                sessionImpl.trackScoreDbReply(
                    Rg.profileList.mainProfile.scoreDb.getScoresForMd5(md5s)
                ).then(result => sessionImpl.scores = result.scores);
            } else {
                sessionImpl.trackScoreDbReply(
                    Rg.profileList.mainProfile.scoreDb.getScores(
                        sessionImpl.folderForHistoryItem(
                            root.historyStack[root.historyStack.length - 1]))
                ).then(result => {
                    if (result instanceof tableQueryResult) {
                        let newScores = result.scores.scores;
                        for (let [key, value] of Object.entries(
                                result.courseScores.scores)) {
                            newScores[key] = value;
                        }
                        sessionImpl.scores = newScores;
                    } else {
                        sessionImpl.scores = result.scores;
                    }
                });
            }
        }

        function refreshPreviewFiles() {
            if (!root.previewFilesEnabled) {
                sessionImpl.previewFiles = ({});
                return;
            }
            let dirs = [];
            for (let item of root.folderContents) {
                if (item instanceof ChartData) {
                    dirs.push(item.chartDirectory);
                }
            }
            sessionImpl.previewFiles =
                Rg.songDirectoryFilePathFetcher.getPreviewFilePaths(dirs);
        }

        function clearStatsFromScoreSummary(summary) {
            let counts = summary?.counts || {};
            return {
                "NOPLAY": counts.NOPLAY || 0,
                "FAILED": counts.FAILED || 0,
                "AEASY": counts.AEASY || 0,
                "LIGHTASSIST": counts.LIGHTASSIST
                    || counts.LIGHT_ASSIST || 0,
                "EASY": counts.EASY || 0,
                "NORMAL": counts.NORMAL || 0,
                "HARD": counts.HARD || 0,
                "EXHARD": counts.EXHARD || 0,
                "FC": counts.FC || 0,
                "PERFECT": counts.PERFECT || 0,
                "MAX": counts.MAX || 0
            };
        }

        function folderClearStatsKey(item) {
            if (typeof item === "string") {
                return "folder:" + item;
            }
            if (item instanceof level) {
                return "level:" + item.name;
            }
            if (item instanceof table) {
                return "table:" + String(item.url || "");
            }
            return "";
        }

        function refreshFolderClearStats() {
            folderClearStatsReplies.cancelAll();
            selectionState.folderClearStatsByKey = ({});
            ++selectionState.folderClearStatsRevision;
            if (!root.folderClearStatsEnabled) {
                return;
            }
            for (let folder of root.folderContents) {
                if (folder instanceof ChartData || folder instanceof entry
                        || folder instanceof course || folder === null) {
                    continue;
                }
                let key = selectionState.folderClearStatsKey(folder);
                if (!key) {
                    continue;
                }
                folderClearStatsReplies.track(
                    Rg.profileList.mainProfile.scoreDb.getScoreSummary(folder)
                ).then(result => {
                    selectionState.folderClearStatsByKey[key] =
                        selectionState.clearStatsFromScoreSummary(result);
                    ++selectionState.folderClearStatsRevision;
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
                if (selectionState.sameEntry(items[i], target)) {
                    return i;
                }
            }
            return -1;
        }

        function open(item) {
            let folder = sessionImpl.resolveFolderContents(item);
            if (folder === null) {
                return null;
            }
            sessionImpl.commitFolderContents(folder);
            selectionState.entries = selectionState.preparedEntries(folder);
            selectionState.publishFolderContents();
            return root.entries;
        }

        function publishFolderContents() {
            root.refresh();
            root.openedFolder();
        }

        function selectedSongFolderPath() {
            if (root.focusedItem instanceof ChartData
                    && root.focusedItem.chartDirectory) {
                return root.focusedItem.chartDirectory;
            }
            if (typeof root.focusedItem === "string") {
                return root.focusedItem;
            }
            for (let i = root.historyStack.length - 1; i >= 0; --i) {
                if (typeof root.historyStack[i] === "string") {
                    return root.historyStack[i];
                }
            }
            return "";
        }

        function sortOrFilterChanged() {
            if (!root.folderContents.length) {
                return;
            }
            let old = root.focusedItem;
            selectionState.entries =
                selectionState.preparedEntries(root.folderContents);
            let index = chartFolderModel.indexOfItem(root.entries, old);
            selectionState.requestFocus(index >= 0 ? index : 0);
        }
    }

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

        tryOpenPlayableAction: root.tryOpenPlayableAction
    }

    PendingReplyGroup {
        id: folderClearStatsReplies
    }

    ChartFolderModel {
        id: chartFolderModel

        difficultyFilter: root.difficultyFilter
        keymodeFilter: root.keymodeFilter
        sortMode: root.sortMode
        unscoredItemsLast: root.unscoredItemsLast
        scores: root.scores

        onSortModeChanged: Qt.callLater(selectionState.sortOrFilterChanged)
        onKeymodeFilterChanged:
            Qt.callLater(selectionState.sortOrFilterChanged)
        onDifficultyFilterChanged:
            Qt.callLater(selectionState.sortOrFilterChanged)
        onUnscoredItemsLastChanged:
            Qt.callLater(selectionState.sortOrFilterChanged)
        onScoresChanged: {
            if (chartFolderModel.sortModeUsesScores()) {
                Qt.callLater(selectionState.sortOrFilterChanged);
            }
        }
    }

    /*! Updates the logical focused \a item after the skin moves focus. */
    function setFocused(item) {
        let logicalIndex = selectionState.indexOfEntry(entries, item);
        selectionState.focusedIndex = logicalIndex;
        selectionState.focusedItem = logicalIndex >= 0
            ? entries[logicalIndex] : null;
    }

    /*! Rebuilds \l entries and their associated data. */
    function refresh() {
        selectionState.refreshScores();
        selectionState.refreshPreviewFiles();
        selectionState.refreshFolderClearStats();
    }

    /*! Returns loaded clear statistics for folder-like \a item, or null. */
    function folderClearStatsFor(item) {
        let revision = selectionState.folderClearStatsRevision;
        let key = selectionState.folderClearStatsKey(item);
        return revision >= 0 && key
            ? (selectionState.folderClearStatsByKey[key] || null) : null;
    }

    /*!
        Opens \a item using \a autoplay, \a replay, and \a replayScore to
        select the requested play mode.
    */
    function openPlayable(item, autoplay = false, replay = false,
                          replayScore = null) {
        return activationImpl.openPlayable(item, autoplay, replay, replayScore);
    }

    /*! Initializes history from the configured song folders and tables. */
    function initialize() {
        if (historyStack.length !== 0) {
            return false;
        }
        sessionImpl.historyStack = [""];
        if (selectionState.open("") === null) {
            sessionImpl.historyStack = [];
            return false;
        }
        selectionState.requestFocus(0);
        return true;
    }

    /*! Leaves the current history entry. Returns whether navigation occurred. */
    function goBack() {
        if (historyStack.length < 2) {
            return false;
        }
        let oldHistory = historyStack;
        let last = oldHistory[oldHistory.length - 1];
        sessionImpl.historyStack = oldHistory.slice(0, oldHistory.length - 1);
        let folder = selectionState.open(
            historyStack[historyStack.length - 1]);
        if (folder === null) {
            sessionImpl.historyStack = oldHistory;
            return false;
        }
        let index = selectionState.indexOfEntry(folder, last);
        selectionState.requestFocus(index >= 0 ? index : 0);
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
        sessionImpl.historyStack = historyStack.concat([item]);
        if (selectionState.open(item) === null) {
            sessionImpl.historyStack = oldHistory;
            return false;
        }
        selectionState.requestFocus(0);
        enteredFolder();
        return true;
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
        if (globalRoot.scanRootSongFolderForPath(
                selectionState.selectedSongFolderPath())) {
            return true;
        }
        let old = focusedItem;
        let folder = selectionState.open(
            historyStack[historyStack.length - 1]);
        if (folder === null) {
            return false;
        }
        let index = selectionState.indexOfEntry(folder, old);
        selectionState.requestFocus(index >= 0 ? index : 0);
        return true;
    }

    /*! Opens \a directory and initially focuses \a initialItem. */
    function openChartDirectory(directory, initialItem) {
        let folder = sessionImpl.resolveChartDirectory(directory);
        if (!folder || !folder.length) {
            return false;
        }
        if (historyStack.length === 0
                || sessionImpl.folderForHistoryItem(
                    historyStack[historyStack.length - 1]) !== directory) {
            sessionImpl.historyStack =
                historyStack.concat([initialItem || directory]);
        }
        sessionImpl.commitFolderContents(folder);
        selectionState.entries = selectionState.preparedEntries(folder);
        selectionState.publishFolderContents();
        let index = chartFolderModel.indexOfItem(entries, initialItem);
        selectionState.requestFocus(index >= 0 ? index : 0);
        return true;
    }

    /*! Replaces the current entries with results for \a query. */
    function search(query) {
        let results = sessionImpl.resolveSearchResults(query);
        let resultCount = results.length;
        if (!results.length) {
            console.info("Search returned no results");
            return resultCount;
        }
        sessionImpl.commitFolderContents(results);
        selectionState.entries = selectionState.preparedEntries(results);
        if (historyStack[historyStack.length - 1]
                !== selectionState.searchHistoryEntry) {
            sessionImpl.historyStack = historyStack.concat(
                [selectionState.searchHistoryEntry]);
        }
        selectionState.requestFocus(0);
        selectionState.publishFolderContents();
        return resultCount;
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

    Component.onCompleted: {
        if (autoInitialize) {
            initialize();
        }
    }

    onScoresEnabledChanged: {
        if (historyStack.length > 0) {
            selectionState.refreshScores();
        }
    }

    onPreviewFilesEnabledChanged: {
        if (historyStack.length > 0) {
            selectionState.refreshPreviewFiles();
        }
    }

    onFolderClearStatsEnabledChanged: {
        if (historyStack.length > 0) {
            selectionState.refreshFolderClearStats();
        }
    }
}
