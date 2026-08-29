import QtQuick

// Selection-specific F-key shortcuts. A select state provides the standard
// F2/F3 behavior; actions can replace it and signals cover custom handling.
Item {
    id: root

    property var reloadAction: null
    property var openSelectedFolderAction: null
    property var openInternetRankingAction: null
    property var selectState: null
    property bool reloadEnabled: true
    property bool openSelectedFolderEnabled: true
    property bool openInternetRankingEnabled: true

    signal reloadRequested()
    signal openSelectedFolderRequested()
    signal openInternetRankingRequested()

    function reload() {
        if (typeof reloadAction === "function") {
            return reloadAction();
        }
        if (selectState) {
            return selectState.reloadCurrentFolderOrTable();
        }
        reloadRequested();
        return true;
    }

    function openSelectedFolder() {
        if (typeof openSelectedFolderAction === "function") {
            return openSelectedFolderAction();
        }
        if (selectState) {
            return selectState.openSelectedFolder();
        }
        openSelectedFolderRequested();
        return true;
    }

    function openInternetRanking() {
        if (typeof openInternetRankingAction === "function") {
            return openInternetRankingAction();
        }
        openInternetRankingRequested();
        return true;
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled && root.reloadEnabled
        sequence: "F2"
        onActivated: root.reload()
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled && root.openSelectedFolderEnabled
        sequence: "F3"
        onActivated: root.openSelectedFolder()
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled && root.openInternetRankingEnabled
        sequence: "F11"
        onActivated: root.openInternetRanking()
    }
}
