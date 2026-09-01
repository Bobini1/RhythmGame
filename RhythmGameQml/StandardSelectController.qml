import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectController
    \inqmlmodule RhythmGameQml
    \brief Composes the complete standard selection behavior.

    The controller extends \l StandardSelectState with presentation adaptation,
    feedback, input, navigation, and shortcuts. Lower-level components can
    instead be instantiated independently when a skin needs a different
    composition.

    The controller does not render a song list. Connect \l presentationEntries
    to a circular presentation or inherited \l entries to a finite one, call
    \l setFocused when its focus changes, and handle \l moveRequested to move
    that focus.
*/
StandardSelectState {
    id: root

    /*! Minimum number of entries produced by \l presentationEntries. */
    property int minimumEntryCount: 0
    /*! Optional autoplay pre-handler. True consumes; false or undefined continues. */
    property var tryAutoplayAction: null
    /*! Optional replay pre-handler. True consumes; false or undefined continues. */
    property var tryReplayAction: null
    /*! Optional replacement for cycling the selected replay type. */
    property var cycleReplayTypeAction: null
    /*! Optional sort pre-handler. True consumes; false or undefined continues. */
    property var tryCycleSortModeAction: null
    /*! Optional replacement for the F2 reload action. */
    property var reloadAction: null
    /*! Optional replacement for the F3 folder-opening action. */
    property var openSelectedFolderAction: null
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
    /*! Number of analog scratch ticks required for one logical step. */
    property alias analogTicksPerStep: input.analogTicksPerStep
    /*! Delay before classic-scratch repeat begins, in milliseconds. */
    property alias initialRepeatDelayMillis: input.initialRepeatDelayMillis
    /*! Delay between repeated classic-scratch steps, in milliseconds. */
    property alias repeatDelayMillis: input.repeatDelayMillis
    /*! Optional replacement for entering-folder feedback. */
    property var enterFeedbackAction: null
    /*! Optional replacement for leaving-folder feedback. */
    property var leaveFeedbackAction: null
    /*! Default entering-folder sound source. */
    property url enterFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-open"
    /*! Default leaving-folder sound source. */
    property url leaveFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-close"

    /*! Entries repeated when \l minimumEntryCount requires it. */
    readonly property var presentationEntries: modelAdapter.entries.slice()

    /*! Emitted when F2 was not handled by the standard reload behavior. */
    signal reloadRequested()
    /*! Emitted when F3 was not handled by the standard folder behavior. */
    signal openSelectedFolderRequested()
    /*! Emitted when F11 requests skin-owned Internet ranking. */
    signal openInternetRankingRequested()
    /*!
        Requests relative focus movement by \a steps. \a repeated identifies
        held input and \a analog identifies analog-scratch input.
    */
    signal moveRequested(int steps, bool repeated, bool analog)

    StandardSelectModelAdapter {
        id: modelAdapter

        source: root.entries
        minimumCount: root.minimumEntryCount
    }

    AudioPlayer {
        id: enterFeedback

        source: root.enterFeedbackSource

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

        source: root.leaveFeedbackSource

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
        input.reset();
    }

    onEnteredFolder: enterFeedback.trigger()
    onLeftFolder: leaveFeedback.trigger()
}
