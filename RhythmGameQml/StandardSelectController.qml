import QtQuick

// Convenience composition for skins that want the standard select session,
// model, feedback, input and shortcuts together. Lower-level parts remain
// exposed and can also be instantiated independently.
Item {
    id: root

    property int minimumEntryCount: 0
    property var tryAutoplayAction: null
    property var tryReplayAction: null
    property var cycleReplayTypeAction: null
    property var tryCycleSortModeAction: null
    property var tryOpenPlayableAction: null
    property var reloadAction: null
    property var openSelectedFolderAction: null
    property var openInternetRankingAction: null
    property bool reloadShortcutEnabled: true
    property bool openSelectedFolderShortcutEnabled: true
    property bool openInternetRankingShortcutEnabled: true
    property bool inputEnabled: enabled
    property bool shortcutsEnabled: enabled
    property bool feedbackEnabled: enabled

    property alias entries: modelAdapter.entries
    property alias logicalEntries: state.entries
    property alias folderContents: state.folderContents
    property alias historyStack: state.historyStack
    property alias scores: state.scores
    property alias previewFiles: state.previewFiles
    property alias folderClearStats: state.folderClearStats
    property alias focusedItem: state.focusedItem
    property alias selectState: state
    property alias session: state.session
    property alias activation: state.activation
    property alias input: input
    property alias navigation: input.navigation
    property alias shortcuts: shortcuts
    property alias feedback: feedback

    signal openedFolder()
    signal openInternetRankingRequested()
    signal focusRequested(int index)
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
        reloadEnabled: root.reloadShortcutEnabled
        openSelectedFolderEnabled:
            root.openSelectedFolderShortcutEnabled
        openInternetRankingEnabled:
            root.openInternetRankingShortcutEnabled
        onOpenInternetRankingRequested:
            root.openInternetRankingRequested()
    }

    function setFocused(item) {
        state.setFocused(item);
    }

    function refresh() {
        state.refresh();
    }

    function goBack() {
        return state.goBack();
    }

    function goForward(item) {
        return state.goForward(item);
    }

    function openPlayable(item, autoplay = false, replay = false,
                          replayScore = null) {
        return state.openPlayable(item, autoplay, replay, replayScore);
    }

    function reloadCurrentFolderOrTable() {
        return state.reloadCurrentFolderOrTable();
    }

    function openChartDirectory(directory, initialItem) {
        return state.openChartDirectory(directory, initialItem);
    }

    function search(query) {
        return state.search(query);
    }

    function sortOrFilterChanged() {
        state.sortOrFilterChanged();
    }

    function isChartItem(item) {
        return state.isChartItem(item);
    }

    function showAllChartsForCurrentSong() {
        return state.showAllChartsForCurrentSong();
    }

    function openSelectedReadme() {
        return state.openSelectedReadme();
    }

    function openSelectedFolder() {
        return state.openSelectedFolder();
    }

    function handleUpPressed(event) {
        input.handleUpPressed(event);
    }

    function handleDownPressed(event) {
        input.handleDownPressed(event);
    }

    function handleReleased(event) {
        input.handleReleased(event);
    }

}
