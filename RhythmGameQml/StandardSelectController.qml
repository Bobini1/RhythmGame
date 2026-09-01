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

    A finite selector needs the following wiring. A circular wheel uses
    \l presentationEntries instead of \l entries and maps its repeated visual
    rows back through \l setFocused in the same way.

    \qml
    import QtQuick
    import RhythmGameQml

    FocusScope {
        id: screen

        property var openRankingAction: null

        StandardSelectController {
            id: selection

            onFocusRequested: index => songList.currentIndex = index
            onMoveRequested: steps => {
                if (songList.count > 0) {
                    songList.currentIndex =
                        (songList.currentIndex + steps + songList.count)
                        % songList.count;
                }
            }
            onOpenInternetRankingRequested: {
                if (typeof screen.openRankingAction === "function") {
                    screen.openRankingAction(focusedItem);
                }
            }
        }

        ListView {
            id: songList

            anchors.fill: parent
            focus: true
            model: selection.entries
            currentIndex: selection.focusedIndex
            delegate: Text {
                required property var modelData
                text: String(modelData)
            }

            onCurrentIndexChanged: {
                if (currentIndex >= 0) {
                    selection.setFocused(model[currentIndex]);
                }
            }

            Keys.onUpPressed: event => selection.handleUpPressed(event)
            Keys.onDownPressed: event => selection.handleDownPressed(event)
            Keys.onReleased: event => selection.handleReleased(event)
        }
    }
    \endqml

    The controller initializes browsing on completion unless \l autoInitialize
    is false. F2, F3, and F12 have built-in behavior. F11 only emits
    \l openInternetRankingRequested because ranking presentation belongs to the
    skin. Setting \l enabled to false suppresses selection input, shortcuts,
    and feedback. \l inputEnabled, \l shortcutsEnabled, and
    \l feedbackEnabled can disable those parts independently.
*/
StandardSelectState {
    id: root

    /*! Minimum number of entries produced by \l presentationEntries. */
    property int minimumEntryCount: 0
    /*! Optional \c tryAutoplayAction() pre-handler. True consumes the input. */
    property var tryAutoplayAction: null
    /*! Optional \c tryReplayAction() pre-handler. True consumes the input. */
    property var tryReplayAction: null
    /*! Optional \c cycleReplayTypeAction() replacement. */
    property var cycleReplayTypeAction: null
    /*! Optional \c tryCycleSortModeAction(delta) pre-handler. True consumes. */
    property var tryCycleSortModeAction: null
    /*! Optional \c reloadAction() replacement for F2. */
    property var reloadAction: null
    /*! Optional \c openSelectedFolderAction() replacement for F3. */
    property var openSelectedFolderAction: null
    /*! Optional \c openSettingsAction() replacement for F12. */
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
    /*! Whether standard audio and replacement feedback actions are active. */
    property bool feedbackEnabled: enabled
    /*! Number of analog scratch ticks required for one logical step. */
    property alias analogTicksPerStep: input.analogTicksPerStep
    /*! Delay before classic-scratch repeat begins, in milliseconds. */
    property alias initialRepeatDelayMillis: input.initialRepeatDelayMillis
    /*! Delay between repeated classic-scratch steps, in milliseconds. */
    property alias repeatDelayMillis: input.repeatDelayMillis
    /*! Optional \c enterFeedbackAction() replacement for the entering sound. */
    property var enterFeedbackAction: null
    /*! Optional \c leaveFeedbackAction() replacement for the leaving sound. */
    property var leaveFeedbackAction: null
    /*! Default entering-folder sound source. */
    property url enterFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-open"
    /*! Default leaving-folder sound source. */
    property url leaveFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-close"

    /*! Defensive snapshot repeated when \l minimumEntryCount requires it. */
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
