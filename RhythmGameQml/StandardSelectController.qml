import QtQuick

/*!
    \qmltype StandardSelectController
    \inqmlmodule RhythmGameQml
    \brief Composes the complete standard selection behavior.

    Use this component when a skin wants the standard selection session, state,
    model adaptation, feedback, input, navigation, and shortcuts together.
    Lower-level components can instead be instantiated independently when a
    skin needs a different composition.

    The controller does not render a song list. Connect \l entries to the
    skin's presentation, call \l setFocused when its focus changes, and handle
    \l moveRequested to move that focus.
*/
Item {
    id: root

    /*! Minimum number of presentation entries produced by \l entries. */
    property int minimumEntryCount: 0
    /*! Optional autoplay pre-handler. True consumes; false or undefined continues. */
    property var tryAutoplayAction: null
    /*! Optional replay pre-handler. True consumes; false or undefined continues. */
    property var tryReplayAction: null
    /*! Optional replacement for cycling the selected replay type. */
    property var cycleReplayTypeAction: null
    /*! Optional sort pre-handler. True consumes; false or undefined continues. */
    property var tryCycleSortModeAction: null
    /*! Optional opening pre-handler. True consumes; false or undefined continues. */
    property var tryOpenPlayableAction: null
    /*! Optional replacement for the F2 reload action. */
    property var reloadAction: null
    /*! Optional replacement for the F3 folder-opening action. */
    property var openSelectedFolderAction: null
    /*! Optional replacement for the F11 Internet-ranking action. */
    property var openInternetRankingAction: null
    /*! Optional replacement for the F12 settings action. */
    property var openSettingsAction: null
    /*! Whether the F2 reload shortcut is active. */
    property bool reloadShortcutEnabled: true
    /*! Whether the F3 folder shortcut is active. */
    property bool openSelectedFolderShortcutEnabled: true
    /*! Whether the F11 Internet-ranking shortcut is active. */
    property bool openInternetRankingShortcutEnabled: true
    /*! Whether the F12 settings shortcut is active. */
    property bool openSettingsShortcutEnabled: true
    /*! Whether standard selection input is active. */
    property bool inputEnabled: enabled
    /*! Whether selection-specific F-key shortcuts are active. */
    property bool shortcutsEnabled: enabled
    /*! Whether standard selection audio feedback is active. */
    property bool feedbackEnabled: enabled
    /*! Whether standard score loading is active. */
    property alias scoresEnabled: state.scoresEnabled
    /*! Whether standard preview-file discovery is active. */
    property alias previewFilesEnabled: state.previewFilesEnabled
    /*! Whether per-folder clear-statistic loading is active. */
    property alias folderClearStatsEnabled: state.folderClearStatsEnabled
    /*! Whether construction automatically opens the root selection folder. */
    property alias autoInitialize: state.autoInitialize
    /*! Sort mode used to prepare logical entries. */
    property alias sortMode: state.sortMode
    /*! Key-mode filter used to prepare logical entries. */
    property alias keymodeFilter: state.keymodeFilter
    /*! Difficulty filter used to prepare logical entries. */
    property alias difficultyFilter: state.difficultyFilter
    /*! Whether items without scores sort after scored items. */
    property alias unscoredItemsLast: state.unscoredItemsLast
    /*! Number of analog scratch ticks required for one logical step. */
    property alias analogTicksPerStep: input.analogTicksPerStep
    /*! Delay before classic-scratch repeat begins, in milliseconds. */
    property alias initialRepeatDelayMillis: input.initialRepeatDelayMillis
    /*! Delay between repeated classic-scratch steps, in milliseconds. */
    property alias repeatDelayMillis: input.repeatDelayMillis
    /*! Optional replacement for entering-folder feedback. */
    property alias enterFeedbackAction: feedback.enterAction
    /*! Optional replacement for leaving-folder feedback. */
    property alias leaveFeedbackAction: feedback.leaveAction
    /*! Default entering-folder sound source. */
    property alias enterFeedbackSource: feedback.enterSoundSource
    /*! Default leaving-folder sound source. */
    property alias leaveFeedbackSource: feedback.leaveSoundSource

    /*! Presentation entries, repeated when \l minimumEntryCount requires it. */
    readonly property var entries: modelAdapter.entries.slice()
    /*! The unique, filtered, and sorted entries before presentation adaptation. */
    readonly property var logicalEntries: state.entries.slice()
    /*! Raw contents of the current folder, table, level, or search. */
    readonly property var folderContents: state.folderContents.slice()
    /*! Navigation history for the current selection session. */
    readonly property var historyStack: state.historyStack.slice()
    /*! Score data loaded for the current folder contents. */
    readonly property var scores: Object.assign({}, state.scores)
    /*! Preview-file data loaded for the current folder contents. */
    readonly property var previewFiles: Object.assign({}, state.previewFiles)
    /*! The logical item currently focused by the skin. */
    readonly property var focusedItem: state.focusedItem

    /*! Emitted after a folder or table has been opened. */
    signal openedFolder()
    /*! Emitted when F2 was not handled by the standard reload behavior. */
    signal reloadRequested()
    /*! Emitted when F3 was not handled by the standard folder behavior. */
    signal openSelectedFolderRequested()
    /*! Emitted when F11 has no replacement action and needs skin handling. */
    signal openInternetRankingRequested()
    /*! Requests that the skin focus \a index in \l logicalEntries. */
    signal focusRequested(int index)
    /*!
        Requests relative focus movement by \a steps. \a repeated identifies
        held input and \a analog identifies analog-scratch input.
    */
    signal moveRequested(int steps, bool repeated, bool analog)

    StandardSelectState {
        id: state

        tryOpenPlayableAction: root.tryOpenPlayableAction
        onFocusRequested: index => root.focusRequested(index)
        onOpenedFolder: root.openedFolder()
        onEnteredFolder: feedback.enterFolder()
        onLeftFolder: feedback.leaveFolder()
    }

    StandardSelectModelAdapter {
        id: modelAdapter

        source: state.entries
        minimumCount: root.minimumEntryCount
    }

    StandardSelectFeedback {
        id: feedback

        enabled: root.feedbackEnabled
    }

    StandardSelectInput {
        id: input

        enabled: root.inputEnabled
        selectState: state
        tryAutoplayAction: root.tryAutoplayAction
        tryReplayAction: root.tryReplayAction
        cycleReplayTypeAction: root.cycleReplayTypeAction
        tryCycleSortModeAction: root.tryCycleSortModeAction
        onMoveRequested: (steps, repeated, analog) => {
            root.moveRequested(steps, repeated, analog);
        }
    }

    StandardSelectShortcuts {
        id: shortcuts

        enabled: root.shortcutsEnabled
        selectState: state
        reloadAction: root.reloadAction
        openSelectedFolderAction: root.openSelectedFolderAction
        openInternetRankingAction: root.openInternetRankingAction
        openSettingsAction: root.openSettingsAction
        reloadEnabled: root.reloadShortcutEnabled
        openSelectedFolderEnabled:
            root.openSelectedFolderShortcutEnabled
        openInternetRankingEnabled:
            root.openInternetRankingShortcutEnabled
        openSettingsEnabled: root.openSettingsShortcutEnabled
        onReloadRequested: root.reloadRequested()
        onOpenSelectedFolderRequested:
            root.openSelectedFolderRequested()
        onOpenInternetRankingRequested:
            root.openInternetRankingRequested()
    }

    /*! Updates the logical focused \a item after the skin moves focus. */
    function setFocused(item) {
        state.setFocused(item);
    }

    /*! Initializes the standard root selection session. */
    function initialize() {
        return state.initialize();
    }

    /*! Reloads metadata associated with the current folder contents. */
    function refresh() {
        state.refresh();
    }

    /*! Leaves the current history entry. Returns whether navigation occurred. */
    function goBack() {
        return state.goBack();
    }

    /*! Opens \a item as a folder or playable item. */
    function goForward(item) {
        return state.goForward(item);
    }

    /*!
        Opens \a item using \a autoplay, \a replay, and \a replayScore to
        select the requested play mode.
    */
    function openPlayable(item, autoplay = false, replay = false,
                          replayScore = null) {
        return state.openPlayable(item, autoplay, replay, replayScore);
    }

    /*! Reloads the current local folder or online table. */
    function reloadCurrentFolderOrTable() {
        return state.reloadCurrentFolderOrTable();
    }

    /*! Opens \a directory and initially focuses \a initialItem. */
    function openChartDirectory(directory, initialItem) {
        return state.openChartDirectory(directory, initialItem);
    }

    /*! Replaces the current entries with results for \a query. */
    function search(query) {
        return state.search(query);
    }

    /*! Returns whether \a item is a playable chart. */
    function isChartItem(item) {
        return state.isChartItem(item);
    }

    /*! Shows every chart that belongs to the currently focused song. */
    function showAllChartsForCurrentSong() {
        return state.showAllChartsForCurrentSong();
    }

    /*! Opens the README associated with the focused item, if one exists. */
    function openSelectedReadme() {
        return state.openSelectedReadme();
    }

    /*! Opens the focused chart's directory in the system file browser. */
    function openSelectedFolder() {
        return state.openSelectedFolder();
    }

    /*! Returns loaded clear statistics for folder-like \a item, or null. */
    function folderClearStatsFor(item) {
        return state.folderClearStatsFor(item);
    }

    /*! Handles an Up key \a event from the skin's visual focus item. */
    function handleUpPressed(event) {
        input.handleUpPressed(event);
    }

    /*! Handles a Down key \a event from the skin's visual focus item. */
    function handleDownPressed(event) {
        input.handleDownPressed(event);
    }

    /*! Handles a keyboard direction-release \a event from the skin. */
    function handleReleased(event) {
        input.handleReleased(event);
    }

    /*! Clears held directions, analog accumulation, and repeat timing. */
    function resetNavigation() {
        input.resetNavigation();
    }

}
