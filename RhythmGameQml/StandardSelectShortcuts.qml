import QtQuick

/*!
    \qmltype StandardSelectShortcuts
    \inqmlmodule RhythmGameQml
    \brief Provides selection-specific F-key shortcuts.

    A \l StandardSelectState provides the built-in F2 and F3 behavior. Each
    action can be replaced, and request signals allow custom handling when the
    standard implementation does not consume the operation.
*/
Item {
    id: root

    /*! Optional replacement for the F2 reload action. */
    property var reloadAction: null
    /*! Optional replacement for the F3 folder-opening action. */
    property var openSelectedFolderAction: null
    /*! Optional replacement for the F11 Internet-ranking action. */
    property var openInternetRankingAction: null
    /*! Standard selection state used by the built-in F2 and F3 implementations. */
    property StandardSelectState selectState: null
    /*! Whether the F2 shortcut is active. */
    property bool reloadEnabled: true
    /*! Whether the F3 shortcut is active. */
    property bool openSelectedFolderEnabled: true
    /*! Whether the F11 shortcut is active. */
    property bool openInternetRankingEnabled: true

    /*! Emitted when F2 has no action or state-backed implementation. */
    signal reloadRequested()
    /*! Emitted when F3 has no action or state-backed implementation. */
    signal openSelectedFolderRequested()
    /*! Emitted when F11 has no replacement action. */
    signal openInternetRankingRequested()

    /*! Runs the replacement or built-in reload behavior. */
    function reload() {
        if (typeof reloadAction === "function") {
            reloadAction();
            return true;
        }
        if (selectState && selectState.reloadCurrentFolderOrTable()) {
            return true;
        }
        reloadRequested();
        return true;
    }

    /*! Runs the replacement or built-in folder-opening behavior. */
    function openSelectedFolder() {
        if (typeof openSelectedFolderAction === "function") {
            openSelectedFolderAction();
            return true;
        }
        if (selectState && selectState.openSelectedFolder()) {
            return true;
        }
        openSelectedFolderRequested();
        return true;
    }

    /*! Runs or requests the Internet-ranking behavior. */
    function openInternetRanking() {
        if (typeof openInternetRankingAction === "function") {
            openInternetRankingAction();
            return true;
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
