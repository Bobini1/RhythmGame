pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectState
    \inqmlmodule RhythmGameQml
    \brief Loads and tracks the entries shown by a song selector.

    The state keeps the current folder, history and focused entry. It also handles
    sorting, filtering, activation, saved results and preview loading. The theme
    draws the list. Use StandardSelectController to include standard input and
    shortcuts, or combine this state with your own controls.

    The root folder opens when the component is completed unless \l autoInitialize
    is false. Handle \l focusRequested to make the requested entry current in
    your view. Call \l setFocused when the theme changes the current entry.
    \l goForward opens a folder or playable item, while
    \l goBack restores the preceding history entry. Call \l exit to leave the
    selector, including its Arena room. \l openResult chooses the correct result
    role for a saved chart or course score.

    The \l sortMode, \l keymodeFilter and \l replayType properties follow the
    active profile and are read-only. Use \l setSortMode, \l setKeymodeFilter
    and \l setReplayType to save a player choice. The \l cycleSortMode and
    \l cycleReplayType actions choose the next option and save it in the same way.
    Search history keeps each query, so \l goBack can restore a search after
    \l showAllChartsForCurrentSong opens a chart directory.

    The arrays and maps in \l entries, \l folderContents, \l historyStack,
    \l results and \l previewFiles are shallow copies. Changing an array or map
    doesn't update the state. The chart and table objects inside them are shared.

    Saved results come from the profile's memory cache before the folder is
    sorted and opened. Previews and folder clear statistics load asynchronously.
    Bind to their properties so the display updates when they arrive. The data
    refreshes when the state is enabled again or the main profile changes.

    \l openReplay chooses a recording for the focused chart or course using
    the profile's \l replayType. Bind a replay button to \l replayAvailable.
    StandardSelectController also supplies the bound replay and cycling keys.
*/
Item {
    id: root

    StandardSelectReload { id: folderReload }

    Connections {
        target: Rg.songFolderFactory
        function onContentsChanged() {
            selectionState.libraryRefreshPending = true;
            Qt.callLater(selectionState.refreshLibraryContents);
        }
    }

    Connections {
        target: Rg.tables
        function onContentsChanged() {
            selectionState.libraryRefreshPending = true;
            Qt.callLater(selectionState.refreshLibraryContents);
        }
    }

    Connections {
        target: Rg.profileList
        function onMainProfileChanged() {
            if (root.historyStack.length > 0) root.refresh();
        }
    }

    Connections {
        target: Rg.profileList.mainProfile.scoreDb
        function onResultsChanged() {
            if (root.enabled) {
                selectionState.refreshFolderClearStats();
                if (chartFolderModel.sortModeUsesScores())
                    Qt.callLater(selectionState.sortOrFilterChanged);
            }
        }
    }

    onEnabledChanged: {
        if (enabled) {
            if (selectionState.libraryRefreshPending)
                Qt.callLater(selectionState.refreshLibraryContents);
            else if (root.historyStack.length > 0) root.refresh();
            if (chartFolderModel.sortModeUsesScores())
                Qt.callLater(selectionState.sortOrFilterChanged);
        }
    }

    /*!
        Filtered and sorted entries before presentation adaptation. Entries can be
        folder-path strings, ChartData objects, or entry, table, level and course
        values. An entry value describes a table chart missing from the song database.
        The list is empty before initialization and when browsing or filtering
        produces no entries. It never adds presentation placeholders. See the
        \l {Song selection entry types}{entry type table in lesson 5}.
    */
    readonly property var entries: selectionState.entries.slice()
    /*! Raw contents of the current folder, table, level, or search. */
    readonly property var folderContents: sessionImpl.folderContents.slice()
    onFolderContentsChanged: activationImpl.cancelScoreLoading()
    /*! Navigation history for the current selection session. */
    readonly property var historyStack: sessionImpl.historyStack.slice()
    /*!
        Map of chart MD5s and course identifiers to BmsResult or BmsResultCourse
        lists, newest first. Results contain no recordings or gauge histories.
        Use ScoreDetails to load one full score for statistics or graphs.
        Child folders and table levels use separate clear-statistic requests.
    */
    readonly property var results: {
        const scoreDb = Rg.profileList.mainProfile.scoreDb;
        const charts = scoreDb.chartResults;
        const courses = scoreDb.courseResults;
        const lists = {};
        for (const item of root.folderContents) {
            if ((item instanceof ChartData || item instanceof entry) && item.md5)
                lists[item.md5] = charts[item.md5.toUpperCase()] || [];
            else if (item instanceof course)
                lists[item.identifier] = courses[item.identifier] || [];
        }
        return lists;
    }
    /*! Preview-file data loaded for the current folder contents. */
    readonly property var previewFiles:
        Object.assign({}, sessionImpl.previewFiles)
    /*!
        Called as \c tryOpenPlayableAction(item,autoplay,replay,replayScore). Return true to
        handle the request, or false to continue with the standard action.
    */
    property var tryOpenPlayableAction: null
    /*! Called before autoplay. Return true when handled, or false for standard playback. */
    property var tryAutoplayAction: null
    /*! Called with the sort direction before cycling. Return true when handled. */
    property var tryCycleSortModeAction: null
    /*! Calls reloadAction() instead of reloading the current folder or table. */
    property var reloadAction: null
    /*! Calls openSelectedFolderAction() instead of opening the system file browser. */
    property var openSelectedFolderAction: null
    /*! Calls openSettingsAction() instead of opening Settings. */
    property var openSettingsAction: null
    /*! Controls whether standard preview-file discovery is active. */
    property bool previewFilesEnabled: true
    /*! Controls whether per-folder clear-statistic loading is active. */
    property bool folderClearStatsEnabled: true
    /*! Controls whether construction automatically opens the root selection folder. */
    property bool autoInitialize: true
    /*! Active profile's sort choice. Change it with setSortMode() or cycleSortMode(). */
    readonly property int sortMode: selectionState.generalVars.selectSortMode
    /*! Modes listed in sortOptions and visited by cycleSortMode(). Does not restrict setSortMode(). */
    property list<int> sortModes: StandardSelectOptions.sortOptions.map(option => option.value)
    /*! Sort choices in \l sortModes order, with stable \c value and translated \c text roles. */
    readonly property var sortOptions: root.sortModes.map(value => ({
        value: value, text: StandardSelectOptions.sortText(value)
    }))
    /*! Optional translated label for the current sort mode. */
    readonly property string sortModeText: StandardSelectOptions.sortText(root.sortMode)
    /*! Active profile's keymode choice. Change it with setKeymodeFilter(). */
    readonly property int keymodeFilter: selectionState.generalVars.selectKeymodeFilter
    /*! Additional difficulty filter for this view. Does not change profile settings. */
    property int difficultyFilter: 0
    /*! Controls whether items without scores sort after scored items. */
    property bool unscoredItemsLast: true
    /*! Index of \l focusedItem in \l entries, or -1 when no entry is selected. */
    readonly property int focusedIndex: selectionState.focusedIndex
    /*!
        Current entry used by selection actions. Updated by browsing or \l setFocused.
        Null when no entry is selected, including when the view shows a
        presentation placeholder.
    */
    readonly property var focusedItem: selectionState.focusedItem
    onFocusedItemChanged: activationImpl.cancelScoreLoading()
    /*! Whether the focused chart or complete course supports standard autoplay. */
    readonly property bool autoplayAvailable: enabled && !actions.arenaSeated
        && !actions.loadingScore && selectionState.playableChartsAvailable

    /*! Selects the newest run, best score rate, best clear, or best combo. */
    enum ReplayType { Newest, BestScore, BestClear, BestCombo }
    /*!
        The active profile's replay choice: 0 for Newest, 1 for BestScore,
        2 for BestClear and 3 for BestCombo. Change it with \l setReplayType
        or \l cycleReplayType. Named values such as
        \c StandardSelectState.BestScore can be used instead of numbers.
    */
    readonly property int replayType: selectionState.generalVars.replayType
    /*! Replay choices with stable \c value and translated \c text roles. */
    readonly property var replayOptions: StandardSelectOptions.replayOptions
    /*! Optional translated label for the current replay choice. */
    readonly property string replayTypeText: StandardSelectOptions.replayText(root.replayType)
    /*! The saved run chosen for playback, or null if no usable recording is available. */
    readonly property var replayResult: resultForReplayType(replayType)
    /*! Whether the standard replay action is available for the focused entry. */
    readonly property bool replayAvailable: enabled && !actions.arenaSeated
        && !activationImpl.loadingScore && !!replayResult && selectionState.playableChartsAvailable
    /*!
        Called as \c tryReplayAction(type) before standard playback. Return true
        when handled, or false to use the standard action. Both buttons calling
        \l openReplay and bound replay input use this hook.
    */
    property var tryReplayAction: null
    /*! Calls \c cycleReplayTypeAction() instead of cycling the profile's replay choice. */
    property var cycleReplayTypeAction: null

    QtObject {
        id: selectionState

        property bool libraryRefreshPending: false
        property var entries: []
        property int focusedIndex: -1
        property var focusedItem: null
        property var folderClearStatsByKey: ({})
        property int folderClearStatsRevision: 0
        readonly property var generalVars:
            Rg.profileList.mainProfile.vars.generalVars
        readonly property bool playableChartsAvailable: {
            if (root.focusedItem instanceof ChartData) return true;
            if (!(root.focusedItem instanceof course)
                    || root.focusedItem.unavailableReason) return false;
            const charts = root.focusedItem.loadCharts();
            return charts.length > 0
                && charts.every(chart => chart instanceof ChartData);
        }

        function refreshLibraryContents() {
            if (!root.enabled || !selectionState.libraryRefreshPending || !root.historyStack.length) {
                return;
            }
            selectionState.libraryRefreshPending = false;
            const old = root.focusedItem;
            let folder = selectionState.open(root.historyStack[root.historyStack.length - 1]);
            while (folder === null && root.historyStack.length > 1) {
                sessionImpl.historyStack = root.historyStack.slice(0, -1);
                folder = selectionState.open(root.historyStack[root.historyStack.length - 1]);
            }
            if (folder !== null) {
                const index = selectionState.indexOfEntry(folder, old);
                selectionState.requestFocus(index >= 0 ? index : 0);
            }
        }

        function clearPriority(clear) {
            const priorities = {
                NOPLAY: 0, FAILED: 1, AEASY: 2,
                EASY: 4, NORMAL: 5, DAN: 5, HARD: 6,
                EXDAN: 6, EXHARD: 7, EXHARDDAN: 7, FC: 8, PERFECT: 9, MAX: 10
            };
            return priorities[clear] || 0;
        }

        function requestFocus(index) {
            index = index >= 0 && index < root.entries.length ? index : -1;
            selectionState.focusedIndex = index;
            selectionState.focusedItem = index >= 0 ? root.entries[index] : null;
            root.focusRequested(index);
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
                return "table:" + item.identifier;
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
            if (a instanceof entry && b instanceof entry) {
                return a.md5 && b.md5 ? a.md5 === b.md5
                    : !!a.sha256 && a.sha256 === b.sha256;
            }
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
                return a.identifier === b.identifier;
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

        function isSearch(item) {
            return item !== null && typeof item === "object" && "searchQuery" in item;
        }

        function open(item) {
            const folder = isSearch(item)
                ? sessionImpl.resolveSearchResults(item.searchQuery)
                : sessionImpl.resolveFolderContents(item);
            if (folder === null) {
                return null;
            }
            sessionImpl.commitFolderContents(folder);
            root.refresh();
            selectionState.entries = selectionState.preparedEntries(folder);
            root.openedFolder();
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

    /*!
        Requests that the theme make \a index in \l entries current in its view.
        The index is -1 when no entry is selected.
        \l focusedItem is already updated when this signal is emitted.
        The theme chooses how to position its list or wheel.
    */
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

    /*! Shared play, saved-result and exit actions. */
    readonly property alias actions: activationImpl
    /*! Calls \c exitAction() in place of leaving selection. */
    property alias exitAction: activationImpl.exitAction

    /*! Leaves selection or its Arena room. Use goBack() for folder history. */
    function exit() { return activationImpl.exit(); }

    /*! Opens the saved \a score for the chart or course \a item. */
    function openResult(item, score) { return activationImpl.openResult(item, score); }

    StandardSelectActions {
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
        scores: root.results

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

    /*!
        Records \a item from \l entries as \l focusedItem after the theme changes
        its current entry. Pass the row's data, not its QML delegate.
        Repeated rows in StandardSelectController::presentationEntries refer to
        the same entries and can be selected normally.
        Passing null or an item outside \l entries clears the selection.
        \l focusedItem becomes null and \l focusedIndex becomes -1.
    */
    function setFocused(item) {
        let logicalIndex = selectionState.indexOfEntry(entries, item);
        selectionState.focusedIndex = logicalIndex;
        selectionState.focusedItem = logicalIndex >= 0
            ? entries[logicalIndex] : null;
    }

    /*! Refreshes preview-file and folder clear-statistic metadata. */
    function refresh() {
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
        Opens \a item using \a autoplay, \a replay, and \a replayScore to select the requested
        play mode.
    */
    function openPlayable(item, autoplay = false, replay = false,
                          replayScore = null) {
        return activationImpl.openPlayable(item, autoplay, replay, replayScore);
    }

    /*! Starts autoplay for the focused chart or course, or calls the custom action. */
    function openAutoplay() {
        if (!root.enabled || root.actions.arenaSeated) return false;
        if (typeof root.tryAutoplayAction === "function" && root.tryAutoplayAction()) {
            return true;
        }
        return root.autoplayAvailable && root.openPlayable(root.focusedItem, true);
    }

    /*!
        Saves \a mode in the active profile and updates \l sortMode.
        Accepts any supported SelectSortMode value, regardless of \l sortModes.
        Returns false for an unknown value or inactive screen, without changing the choice.
    */
    function setSortMode(mode) {
        if (!root.enabled || !StandardSelectOptions.sortOptions.some(option => option.value === mode))
            return false;
        selectionState.generalVars.selectSortMode = mode;
        return true;
    }

    /*!
        Saves \a filter in the active profile and updates \l keymodeFilter.
        Returns false for an unknown value or inactive screen, without changing the choice.
    */
    function setKeymodeFilter(filter) {
        if (!root.enabled || ![SelectKeymodeFilter.All, SelectKeymodeFilter.Single,
                SelectKeymodeFilter.Double, SelectKeymodeFilter.K5, SelectKeymodeFilter.K7,
                SelectKeymodeFilter.K10, SelectKeymodeFilter.K14].includes(filter))
            return false;
        selectionState.generalVars.selectKeymodeFilter = filter;
        return true;
    }

    /*!
        Moves through \l sortModes by the integer \a delta and saves the choice,
        or calls the custom action. Wraps at either end. Returns false when
        inactive or delta is not an integer. The default action also returns
        false if the list is empty or the next value is unsupported.
    */
    function cycleSortMode(delta = 1) {
        if (!root.enabled || !Number.isInteger(delta)) return false;
        if (typeof root.tryCycleSortModeAction === "function"
                && root.tryCycleSortModeAction(delta)) return true;
        const count = root.sortModes.length;
        if (count === 0) return false;
        const current = root.sortModes.indexOf(root.sortMode);
        const next = current < 0 ? 0 : ((current + delta) % count + count) % count;
        return root.setSortMode(root.sortModes[next]);
    }

    /*! Opens Settings, or calls the replacement action. */
    function openSettings() {
        if (!root.enabled) return false;
        if (typeof root.openSettingsAction === "function") root.openSettingsAction();
        else globalRoot.openSettings();
        return true;
    }

    /*!
        Returns a saved run for the focused chart or course using \a type.
        Defaults to \l replayType. When \a requireReplayData is true, ignores
        runs without recordings and courses without a recording for every stage.
        Pass false when choosing a saved result to display. Best score compares
        points divided by maximum points. Ties use the newer run.
    */
    function resultForReplayType(type = replayType, requireReplayData = true) {
        const item = root.focusedItem;
        const isCourse = item instanceof course;
        if (!isCourse && !(item instanceof ChartData)) {
            return null;
        }
        const candidates = root.results[isCourse ? item.identifier : item.md5] || [];
        let best = null;
        let bestValue = -Infinity;
        for (const score of candidates) {
            if (!score) continue;
            if (requireReplayData) {
                if (!score.replayAvailable) continue;
                if (isCourse && (!score.results || !item.chartIds.length
                        || score.results.length !== item.chartIds.length
                        || !score.results.every(stage => stage && stage.replayAvailable))) {
                    continue;
                }
            }
            const result = score;
            let value;
            switch (type) {
            case StandardSelectState.Newest: value = result.unixTimestamp; break;
            case StandardSelectState.BestScore:
                if (result.maxPoints <= 0) continue;
                value = result.points / result.maxPoints;
                break;
            case StandardSelectState.BestClear:
                value = selectionState.clearPriority(result.clearType);
                break;
            case StandardSelectState.BestCombo: value = result.maxCombo; break;
            default: return null;
            }
            if (value > bestValue || (value === bestValue
                    && (!best || result.unixTimestamp > best.unixTimestamp))) {
                best = score;
                bestValue = value;
            }
        }
        return best;
    }

    /*!
        Saves \a type in the active profile and updates \l replayType.
        Returns false for an unknown value or inactive screen, without changing the choice.
        Use \l cycleReplayType to advance to the next option and wrap at the end.
    */
    function setReplayType(type) {
        if (!root.enabled || !root.replayOptions.some(option => option.value === type))
            return false;
        selectionState.generalVars.replayType = type;
        return true;
    }

    /*! Cycles newest, best score, best clear and best combo, or calls the replacement action. */
    function cycleReplayType() {
        if (!root.enabled) return false;
        if (typeof root.cycleReplayTypeAction === "function") {
            root.cycleReplayTypeAction();
            return true;
        }
        const options = root.replayOptions;
        const index = options.findIndex(option => option.value === root.replayType);
        return options.length > 0 && root.setReplayType(options[(index + 1) % options.length].value);
    }

    /*!
        Opens the focused entry's replay using \a type, which defaults to
        \l replayType. Returns false when no usable recording is available.
        Replay playback uses the main profile alone, even in battle mode.
        Arena and inactive screens cannot start replay playback.
    */
    function openReplay(type = replayType) {
        if (!root.enabled || root.actions.arenaSeated) return false;
        if (typeof root.tryReplayAction === "function" && root.tryReplayAction(type)) {
            return true;
        }
        const score = root.resultForReplayType(type);
        if (!score || !selectionState.playableChartsAvailable) return false;
        return root.openPlayable(root.focusedItem, false, true, score);
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
        if (!root.enabled) return false;
        if (typeof root.reloadAction === "function") {
            root.reloadAction();
            return true;
        }
        if (folderReload.reload(root.focusedItem, root.historyStack,
                                selectionState.selectedSongFolderPath())) {
            return true;
        }
        if (root.historyStack.length === 0) {
            return false;
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
        const history = selectionState.isSearch(historyStack[historyStack.length - 1])
            ? historyStack.slice(0, -1) : historyStack;
        sessionImpl.historyStack = history.concat([{searchQuery: query || ""}]);
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
        if (!root.enabled) return false;
        if (typeof root.openSelectedFolderAction === "function") {
            root.openSelectedFolderAction();
            return true;
        }
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
