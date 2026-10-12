import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectController
    \inqmlmodule RhythmGameQml
    \brief Supplies browsing, input, shortcuts and music for a song selector.

    The controller includes StandardSelectState plus navigation, shortcuts and
    folder feedback, background music and chart previews. Your theme owns the
    list or wheel, its current row and its highlight. Don't add the included
    components separately to the same screen.

    \l StandardSelectListView connects a finite list to this controller. Supply its
    delegate and call its activate(index) method from row clicks. It handles
    current-row changes, arrows, mouse wheel input and preview movement.

    A custom list or circular wheel can use the controller directly. Use
    entries for one row per item, or presentationEntries with minimumEntryCount
    for repeated rows. Handle moveRequested and focusRequested in your view,
    and call setFocused when its current entry changes. Forward Up, Down and
    release key events to handleUpPressed, handleDownPressed and handleReleased.
    Forward wheel events from the view to handleWheel.
    The theme manages keyboard focus in either approach.

    Browsing initializes when the component is completed. To choose the timing,
    set \l autoInitialize to false and call \l initialize after the view is ready.

    F2 reloads the current folder or table, F3 opens the selected folder, and F12
    opens Settings. F11 calls \l openInternetRanking to open the selected chart's
    ranking page. The included \l ranking also supplies data for a ranking display.
    Buttons can call \l reloadCurrentFolderOrTable, \l openSelectedFolder and
    \l openSettings to use the same actions and overrides as those shortcuts.
    At the root, keys 2 and 4 call \l cycleSortMode. The profile keeps the choice.
    Autoplay and replay have standard actions. Key 7 calls the inherited
    \l openReplay; key 6 calls \l cycleReplayType. Bind replay buttons to
    \l replayAvailable and call \l openReplay from their click handlers.
    Autoplay buttons use \l autoplayAvailable and \l openAutoplay.
    Override \l tryReplayAction to provide a custom replay picker. Escape leaves selection or
    its Arena room. Disable Escape with \l exitEnabled.
    StandardArenaSelectOverlay supplies the Arena ready controls and shortcut.

    The included \l preview follows the focused chart. StandardSelectListView
    binds preview.moving automatically. For another view, bind it to the view's
    movement state to wait until scrolling stops. Background music
    stops while a preview is playing and returns after \l backgroundMusicDelayMillis.
    Customize the \l preview and \l backgroundMusic properties, or set either to
    null to omit it. Don't add a second StandardSelectPreview alongside the controller.

    Disabling the controller stops its input, shortcuts, feedback and music. Use
    \l inputEnabled, \l shortcutsEnabled or \l feedbackEnabled to disable just
    one part. Disabled keyboard handlers leave events unaccepted for another handler.
    Keep the controller outside any visual container that you disable for a
    popup if music should continue while that popup is open.

    Set \l enterFeedbackSource and \l leaveFeedbackSource to choose folder sounds.
    For custom feedback, set \l feedbackEnabled to false and handle
    \l enteredFolder and \l leftFolder yourself.
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
