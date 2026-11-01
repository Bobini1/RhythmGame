import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectController
    \inqmlmodule RhythmGameQml
    \brief Adds browsing, input, previews and music to a song selector.

    Use this component as the starting point for a selection screen. It loads
    folders and tables, opens charts and courses, and handles standard input.
    Your theme draws the list or wheel. See the
    \l {../theme_tutorial_select.html}{selection lesson} for a complete example.

    \section1 Connect your view

    For a normal list, use StandardSelectListView. Set its \c controller to
    this component, supply a delegate, and call its \c activate(index) method
    when a row is clicked. Set keyboard focus on the list and its enclosing
    focus scopes.

    For a custom wheel, use \l entries for one row per item or
    \l presentationEntries for repeated rows. Handle \l moveRequested and
    \l focusRequested, then call \l setFocused with the selected row's data.
    Forward Up, Down, release and wheel events to \l handleUpPressed,
    \l handleDownPressed, \l handleReleased and \l handleWheel.

    The root folder opens automatically. Set \l autoInitialize to false and
    call \l initialize when your view is ready if you need different timing.

    \section1 Use the built-in actions

    Buttons can call the same actions as keys: \l openAutoplay, \l openReplay,
    \l cycleSortMode, \l openSettings and \l exit. Bind replay and autoplay
    buttons to \l replayAvailable and \l autoplayAvailable.
    StandardSelectInput lists the controller key mappings;
    StandardSelectShortcuts lists F2, F3, F11 and F12.

    The controller already includes StandardSelectState, StandardSelectInput
    and StandardSelectShortcuts. Do not add a second copy beside it.
    Add StandardArenaSelectOverlay for Arena ready controls and chat.

    \section1 Customize audio and rankings

    Change \l preview, \l backgroundMusic or \l ranking to customize these
    features; set one to null to omit it. The preview follows the focused chart,
    and music stops while it plays. StandardSelectListView delays previews
    during movement; a custom wheel must bind \c preview.moving itself.

    Set \l enterFeedbackSource and \l leaveFeedbackSource for folder sounds.
    To play your own feedback, set \l feedbackEnabled to false and handle
    \l enteredFolder and \l leftFolder.

    \section1 Pause input for a popup

    Disabling the controller stops its input, shortcuts, audio and ranking
    work. To keep music playing while a popup is open, leave the controller
    enabled and set \l inputEnabled and \l shortcutsEnabled to false.
    Keep it outside any container you disable for that popup.
*/
StandardSelectState {
    id: root

    /*!
        Preview for the focused chart. Plays a discovered preview file or falls
        back to chart audio without BGA. Customize its delayMillis, moving,
        volume or chartAudioEnabled properties. Set it to null to omit previews,
        or supply another StandardSelectPreview with its own selection bindings.
    */
    property StandardSelectPreview preview: StandardSelectPreview {
        id: defaultPreview

        enabled: root.enabled && root.preview === defaultPreview
        selectState: root
    }
    /*!
        Looping selection music. Its source defaults to the active profile's
        bgmPath followed by "select". The player accepts a local file path.
        Playback stops while a preview plays or the controller is disabled.

        Customize the player's source, volume or fadeInMillis. Set it to null
        to omit background music, or supply another AudioPlayer. A replacement
        owns its playback binding and must stop when the controller is disabled.
    */
    property AudioPlayer backgroundMusic: AudioPlayer {
        id: defaultBackgroundMusic

        source: {
            const path = Rg.profileList.mainProfile.vars.generalVars.bgmPath;
            return root.backgroundMusic === defaultBackgroundMusic && path
                ? path + "select" : "";
        }
        playing: backgroundMusicDelay.canPlay && backgroundMusicDelay.elapsed
            && defaultBackgroundMusic.loaded
        looping: true
        fadeInMillis: 1000
    }
    /*! Delay before background music starts or returns after a preview, in milliseconds. */
    property int backgroundMusicDelayMillis: 500

    /*!
        Ranking for the focused chart. Uses the active profile and its chosen
        provider. Bind a display to its entries, position and loading properties.
        Customize its properties, or replace it with another StandardRanking.
        Set it to null to omit ranking support.
    */
    property StandardRanking ranking: StandardRanking {
        id: defaultRanking
        enabled: root.enabled && root.ranking === defaultRanking
        chartData: root.focusedItem
    }
    /*! Calls openInternetRankingAction() instead of opening the ranking website. */
    property var openInternetRankingAction: null
    /*! Minimum number of entries produced by \l presentationEntries. */
    property int minimumEntryCount: 0
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
    /*! Controls whether selection-specific F-key shortcuts are active. */
    property bool shortcutsEnabled: enabled
    /*! Controls folder sounds, independently of music. */
    property bool feedbackEnabled: enabled
    /*! Number of analog scratch ticks required for one logical step. */
    property alias analogTicksPerStep: input.analogTicksPerStep
    /*! Delay before classic-scratch repeat begins, in milliseconds. */
    property alias initialRepeatDelayMillis: input.initialRepeatDelayMillis
    /*! Delay between repeated classic-scratch steps, in milliseconds. */
    property alias repeatDelayMillis: input.repeatDelayMillis
    /*! Entering-folder sound source. Use an empty URL to omit it. */
    property url enterFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-open"
    /*! Leaving-folder sound source. Use an empty URL to omit it. */
    property url leaveFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-close"

    /*!
        Contains a copy of the entries, repeated to fill \l minimumEntryCount when
        needed. When \l entries is empty, contains null placeholders instead,
        with at least one row. The logical \l entries list stays empty.
    */
    readonly property var presentationEntries: modelAdapter.entries.slice()

    /*! Emitted when F2 was not handled by the standard reload behavior. */
    signal reloadRequested()
    /*! Emitted when F3 was not handled by the standard folder behavior. */
    signal openSelectedFolderRequested()
    /*!
        Requests that the theme move its current row by \a steps. The theme
        chooses whether movement wraps or stops at the end of the list.
        \a repeated identifies held input and \a analog identifies analog scratch or wheel input.
    */
    signal moveRequested(int steps, bool repeated, bool analog)

    StandardSelectModelAdapter {
        id: modelAdapter

        source: root.entries
        minimumCount: root.minimumEntryCount
    }

    Timer {
        id: backgroundMusicDelay

        property bool elapsed: false
        readonly property bool canPlay: root.enabled
            && root.backgroundMusic === defaultBackgroundMusic
            && !root.preview?.playing
        interval: Math.max(1, root.backgroundMusicDelayMillis)
        running: canPlay && !elapsed
        onCanPlayChanged: elapsed = false
        onTriggered: elapsed = true
    }

    AudioPlayer {
        id: enterFeedback

        source: root.feedbackEnabled ? root.enterFeedbackSource : ""

        function trigger() {
            if (!root.enabled || !root.feedbackEnabled) {
                return;
            }
            stop();
            play();
        }
    }

    AudioPlayer {
        id: leaveFeedback

        source: root.feedbackEnabled ? root.leaveFeedbackSource : ""

        function trigger() {
            if (!root.enabled || !root.feedbackEnabled) {
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
        onMoveRequested: (steps, repeated, analog) => {
            root.moveRequested(steps, repeated, analog);
        }
    }

    StandardSelectShortcuts {
        id: shortcuts

        enabled: root.shortcutsEnabled
        selectState: root
        openInternetRankingAction: root.openInternetRanking
        reloadEnabled: root.reloadShortcutEnabled
        openSelectedFolderEnabled:
            root.openSelectedFolderShortcutEnabled
        openInternetRankingEnabled:
            root.openInternetRankingShortcutEnabled
        openSettingsEnabled: root.openSettingsShortcutEnabled
        onReloadRequested: root.reloadRequested()
        onOpenSelectedFolderRequested:
            root.openSelectedFolderRequested()
    }

    /*! Opens the selected chart's ranking page, or calls the replacement action. */
    function openInternetRanking() {
        if (!enabled) return false;
        if (typeof openInternetRankingAction === "function") {
            return openInternetRankingAction() !== false;
        }
        return ranking ? ranking.open() : false;
    }

    /*! Handles an Up key \a event from the theme's visual focus item. */
    function handleUpPressed(event) {
        return input.handleUpPressed(event);
    }

    /*! Handles a Down key \a event from the theme's visual focus item. */
    function handleDownPressed(event) {
        return input.handleDownPressed(event);
    }

    /*! Handles a keyboard direction-release \a event from the theme. */
    function handleReleased(event) {
        return input.handleReleased(event);
    }

    /*! Handles a wheel \a event from the theme's view. Returns true when accepted. */
    function handleWheel(event) {
        return input.handleWheel(event);
    }

    /*! Clears held directions, accumulated scratch and wheel movement, and repeat timing. */
    function resetNavigation() {
        input.reset();
    }

    onEnteredFolder: enterFeedback.trigger()
    onLeftFolder: leaveFeedback.trigger()
}
