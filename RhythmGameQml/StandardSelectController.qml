import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectController
    \inqmlmodule RhythmGameQml
    \brief Supplies browsing, input and shortcuts for a song selector.

    The controller includes StandardSelectState plus navigation, shortcuts and
    folder feedback. Your skin draws the list or wheel and moves its focus.
    Don't add the included components separately to the same screen.

    Use inherited \l entries for a list with one row per item. For a circular
    wheel that needs repeated rows, use \l presentationEntries and set
    \l minimumEntryCount. Call \l setFocused when the focused item changes.
    Handle \l focusRequested to restore focus and \l moveRequested to move it.

    The focused visual item must forward its keyboard events to \l handleUpPressed,
    \l handleDownPressed and \l handleReleased. The
    \l {../skin_tutorial_select.html}{selection lesson} contains an installable
    list with the focus and keyboard handling in place.

    Browsing initializes when the component is completed. To choose the timing,
    set \l autoInitialize to false and call \l initialize after the view is ready.

    F2 reloads the current folder or table, F3 opens the selected folder, and F12
    opens Settings. F11 emits \l openInternetRankingRequested so the skin can show
    its own ranking view. Autoplay has a standard action. Replay requires
    \l tryReplayAction to choose a saved replay. Escape leaves selection or
    its Arena room. In Arena, pressing Start twice on the same player side
    toggles ready. Disable either gesture with \l exitEnabled or \l readyEnabled.

    Disabling the controller stops its input, shortcuts and feedback. Use
    \l inputEnabled, \l shortcutsEnabled or \l feedbackEnabled to disable just
    one part. Disabled keyboard handlers leave events unaccepted for another handler.
*/
StandardSelectState {
    id: root

    /*! Minimum number of entries produced by \l presentationEntries. */
    property int minimumEntryCount: 0
    /*!
        Called as \c tryAutoplayAction(). Return true to handle the request, or false to
        continue with the standard action.
    */
    property var tryAutoplayAction: null
    /*!
        Called as \c tryReplayAction() to choose and open a replay. Return true when handled.
        Without a successful handler, replay input does nothing.
    */
    property var tryReplayAction: null
    /*! Calls \c cycleReplayTypeAction() when key 6 is pressed. Without a callback, the key does nothing. */
    property var cycleReplayTypeAction: null
    /*!
        Called as \c tryCycleSortModeAction(delta). Return true to handle the request, or
        false to continue with the standard action.
    */
    property var tryCycleSortModeAction: null
    /*! Calls \c reloadAction() in place of F2. */
    property var reloadAction: null
    /*! Calls \c openSelectedFolderAction() in place of F3. */
    property var openSelectedFolderAction: null
    /*! Calls \c openSettingsAction() in place of F12. */
    property var openSettingsAction: null
    /*! Controls whether the F2 reload shortcut is active. */
    property bool reloadShortcutEnabled: true
    /*! Controls whether the F3 folder shortcut is active. */
    property bool openSelectedFolderShortcutEnabled: true
    /*! Controls whether the F11 Internet-ranking shortcut is active. */
    property bool openInternetRankingShortcutEnabled: true
    /*! Controls whether the F12 settings shortcut is active. */
    property bool openSettingsShortcutEnabled: true
    /*! Controls whether standard selection input is active. */
    property bool inputEnabled: enabled
    /*! Controls the Escape shortcut for leaving selection. */
    property alias exitEnabled: input.exitEnabled
    /*! Controls the double-Start Arena ready gesture. */
    property alias readyEnabled: input.readyEnabled
    /*! Controls whether selection-specific F-key shortcuts are active. */
    property bool shortcutsEnabled: enabled
    /*! Controls whether standard audio and replacement feedback actions are active. */
    property bool feedbackEnabled: enabled
    /*! Number of analog scratch ticks required for one logical step. */
    property alias analogTicksPerStep: input.analogTicksPerStep
    /*! Delay before classic-scratch repeat begins, in milliseconds. */
    property alias initialRepeatDelayMillis: input.initialRepeatDelayMillis
    /*! Delay between repeated classic-scratch steps, in milliseconds. */
    property alias repeatDelayMillis: input.repeatDelayMillis
    /*! Calls \c enterFeedbackAction() in place of the entering sound. */
    property var enterFeedbackAction: null
    /*! Calls \c leaveFeedbackAction() in place of the leaving sound. */
    property var leaveFeedbackAction: null
    /*! Default entering-folder sound source, loaded only for built-in feedback. */
    property url enterFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-open"
    /*! Default leaving-folder sound source, loaded only for built-in feedback. */
    property url leaveFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-close"

    /*! Contains a copy of the entries, repeated to fill \l minimumEntryCount when needed. */
    readonly property var presentationEntries: modelAdapter.entries.slice()

    /*! Emitted when F2 was not handled by the standard reload behavior. */
    signal reloadRequested()
    /*! Emitted when F3 was not handled by the standard folder behavior. */
    signal openSelectedFolderRequested()
    /*! Emitted when F11 requests the skin's ranking view. */
    signal openInternetRankingRequested()
    /*!
        Requests relative focus movement by \a steps. \a repeated identifies held input and \a
        analog identifies analog-scratch input.
    */
    signal moveRequested(int steps, bool repeated, bool analog)

    StandardSelectModelAdapter {
        id: modelAdapter

        source: root.entries
        minimumCount: root.minimumEntryCount
    }

    AudioPlayer {
        id: enterFeedback

        source: root.feedbackEnabled
                && typeof root.enterFeedbackAction !== "function"
            ? root.enterFeedbackSource : ""

        function trigger() {
            if (!root.enabled || !root.feedbackEnabled) {
                return;
            }
            if (typeof root.enterFeedbackAction === "function") {
                root.enterFeedbackAction();
                return;
            }
            stop();
            play();
        }
    }

    AudioPlayer {
        id: leaveFeedback

        source: root.feedbackEnabled
                && typeof root.leaveFeedbackAction !== "function"
            ? root.leaveFeedbackSource : ""

        function trigger() {
            if (!root.enabled || !root.feedbackEnabled) {
                return;
            }
            if (typeof root.leaveFeedbackAction === "function") {
                root.leaveFeedbackAction();
                return;
            }
            stop();
            play();
        }
    }

    StandardSelectInput {
        id: input

        enabled: root.inputEnabled
        selectState: root
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
        selectState: root
        reloadAction: root.reloadAction
        openSelectedFolderAction: root.openSelectedFolderAction
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

    /*! Handles an Up key \a event from the skin's visual focus item. */
    function handleUpPressed(event) {
        return input.handleUpPressed(event);
    }

    /*! Handles a Down key \a event from the skin's visual focus item. */
    function handleDownPressed(event) {
        return input.handleDownPressed(event);
    }

    /*! Handles a keyboard direction-release \a event from the skin. */
    function handleReleased(event) {
        return input.handleReleased(event);
    }

    /*! Clears held directions, analog accumulation, and repeat timing. */
    function resetNavigation() {
        input.reset();
    }

    onEnteredFolder: enterFeedback.trigger()
    onLeftFolder: leaveFeedback.trigger()
}
