import QtQuick

/*!
    \qmltype StandardSelectShortcuts
    \inqmlmodule RhythmGameQml
    \brief Adds F2, F3, F11 and F12 actions to a selector.

    Assign \l selectState for the standard reload and open-folder operations.
    StandardSelectController already includes the shortcuts, so use this component
    separately only when composing your own selector.

    \table
        \header \li Key \li Action
        \row \li F2
             \li Use \l reloadAction if set. Otherwise ask the state to reload,
                 then emit \l reloadRequested if it cannot handle the request.
        \row \li F3
             \li Use \l openSelectedFolderAction if set. Otherwise ask the state
                 to open the folder, then emit \l openSelectedFolderRequested if unhandled.
        \row \li F11 \li Emit \l openInternetRankingRequested for the skin's ranking view.
        \row \li F12 \li Use \l openSettingsAction if set, otherwise open Settings.
    \endtable

    Without \l selectState, F2 and F3 emit their request signals unless a
    replacement action is supplied. A replacement handles the whole operation,
    and its return value is ignored. Each shortcut can be disabled separately.
    Disabling the component disables all four.
*/
Item {
    id: root

    /*! Calls \c reloadAction() in place of F2. */
    property var reloadAction: null
    /*! Calls \c openSelectedFolderAction() in place of F3. */
    property var openSelectedFolderAction: null
    /*! Calls \c openSettingsAction() in place of F12. */
    property var openSettingsAction: null
    /*! Standard selection state used by the built-in F2 and F3 implementations. */
    property StandardSelectState selectState: null
    /*! Controls whether the F2 shortcut is active. */
    property bool reloadEnabled: true
    /*! Controls whether the F3 shortcut is active. */
    property bool openSelectedFolderEnabled: true
    /*! Controls whether the F11 shortcut is active. */
    property bool openInternetRankingEnabled: true
    /*! Controls whether the F12 settings shortcut is active. */
    property bool openSettingsEnabled: true

    /*! Emitted when no replacement handles F2 and the state cannot reload. */
    signal reloadRequested()
    /*! Emitted when no replacement handles F3 and the state cannot open the folder. */
    signal openSelectedFolderRequested()
    /*! Emitted when F11 requests the skin's ranking view. */
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
