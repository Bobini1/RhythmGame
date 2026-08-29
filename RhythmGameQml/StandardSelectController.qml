import QtQuick

// Convenience composition for skins that want the standard state,
// navigation and shortcuts together. Each lower-level object remains exposed.
Item {
    id: root

    property var navigationTarget: parent
    property int minimumEntryCount: 0
    property var autoplayAction: null
    property var replayAction: null
    property var cycleReplayTypeAction: null
    property var cycleSortModeAction: null
    property var reloadAction: null
    property var openSelectedFolderAction: null
    property var openInternetRankingAction: null
    property bool reloadShortcutEnabled: true
    property bool openSelectedFolderShortcutEnabled: true
    property bool openInternetRankingShortcutEnabled: true
    property bool navigationEnabled: enabled
    property bool shortcutsEnabled: enabled

    property alias entries: state.entries
    property alias folderContents: state.folderContents
    property alias historyStack: state.historyStack
    property alias scores: state.scores
    property alias previewFiles: state.previewFiles
    property alias folderClearStats: state.folderClearStats
    property alias focusedItem: state.focusedItem
    property alias selectState: state
    property alias navigation: navigation
    property alias shortcuts: shortcuts

    signal openedFolder()
    signal openInternetRankingRequested()

    StandardSelectState {
        id: state

        minimumEntryCount: root.minimumEntryCount

        onFocusRequested: index => {
            if (!root.navigationTarget) {
                return;
            }
            root.navigationTarget.positionViewAtIndex(index, PathView.Center);
            root.navigationTarget.resetNavigation();
        }
        onOpenedFolder: root.openedFolder()
    }

    StandardSelectNavigation {
        id: navigation

        enabled: root.navigationEnabled
        target: root.navigationTarget
        selectState: state
        autoplayAction: root.autoplayAction
        replayAction: root.replayAction
        cycleReplayTypeAction: root.cycleReplayTypeAction
        cycleSortModeAction: root.cycleSortModeAction
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

    function setFocused(index, item) {
        state.setFocused(index, item);
    }

    function refresh() {
        state.refresh();
    }

    function goBack() {
        return state.goBack();
    }

    function goForward(item, skipSound = false) {
        return state.goForward(item, skipSound);
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
        navigation.handleUpPressed(event);
    }

    function handleDownPressed(event) {
        navigation.handleDownPressed(event);
    }

    function handleReleased(event) {
        navigation.handleReleased(event);
    }

    Component.onCompleted: state.goForward("", true)
}
