import QtQuick

/*!
    \qmltype StandardSelectShortcuts
    \inqmlmodule RhythmGameQml
    \brief Provides selection-specific F-key shortcuts.

    A \l StandardSelectState provides the built-in F2 and F3 behavior. F11
    requests skin-owned Internet ranking, and F12 opens settings through the
    application content frame. Built-in actions can be replaced, and request
    signals allow custom handling when no standard implementation consumes an
    operation.
*/
Item {
    id: root

    /*! Optional replacement for the F2 reload action. */
    property var reloadAction: null
    /*! Optional replacement for the F3 folder-opening action. */
    property var openSelectedFolderAction: null
    /*! Optional replacement for the F12 settings action. */
    property var openSettingsAction: null
    /*! Standard selection state used by the built-in F2 and F3 implementations. */
    property StandardSelectState selectState: null
    /*! Whether the F2 shortcut is active. */
    property bool reloadEnabled: true
    /*! Whether the F3 shortcut is active. */
    property bool openSelectedFolderEnabled: true
    /*! Whether the F11 shortcut is active. */
    property bool openInternetRankingEnabled: true
    /*! Whether the F12 settings shortcut is active. */
    property bool openSettingsEnabled: true

    /*! Emitted when F2 has no action or state-backed implementation. */
    signal reloadRequested()
    /*! Emitted when F3 has no action or state-backed implementation. */
    signal openSelectedFolderRequested()
    /*! Emitted when F11 requests skin-owned Internet ranking. */
    signal openInternetRankingRequested()

    QtObject {
        id: shortcutActions

        function reload() {
            if (typeof root.reloadAction === "function") {
                root.reloadAction();
            } else if (!root.selectState
                       || !root.selectState.reloadCurrentFolderOrTable()) {
                root.reloadRequested();
            }
        }

        function openSelectedFolder() {
            if (typeof root.openSelectedFolderAction === "function") {
                root.openSelectedFolderAction();
            } else if (!root.selectState
                       || !root.selectState.openSelectedFolder()) {
                root.openSelectedFolderRequested();
            }
        }

        function openSettings() {
            if (typeof root.openSettingsAction === "function") {
                root.openSettingsAction();
            } else {
                globalRoot.openSettings();
            }
        }
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled && root.reloadEnabled
        sequence: "F2"
        onActivated: shortcutActions.reload()
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled && root.openSelectedFolderEnabled
        sequence: "F3"
        onActivated: shortcutActions.openSelectedFolder()
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled && root.openInternetRankingEnabled
        sequence: "F11"
        onActivated: root.openInternetRankingRequested()
    }

    Shortcut {
        autoRepeat: false
        enabled: root.enabled && root.openSettingsEnabled
        sequence: "F12"
        onActivated: shortcutActions.openSettings()
    }
}
