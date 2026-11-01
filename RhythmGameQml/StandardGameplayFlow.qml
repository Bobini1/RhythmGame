import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayFlow
    \inqmlmodule RhythmGameQml
    \brief Starts play, handles exit and retry, saves scores and opens results.

    Place one instance inside the gameplay screen and bind \l gameplay to the
    supplied GameplayContext. The flow handles ready audio, startup, Escape,
    quick retry, and speed and cover controls. Your theme draws the playfield.
    See the \l {../theme_tutorial_gameplay.html}{gameplay lesson} for an example.

    StandardGameplayInput, StandardGameplayAttemptState and
    StandardGameplaySpeedControls are already included. Do not add another
    copy beside the flow. It inherits the screen's enabled state, so inactive
    screens cannot start play or navigate.

    \section1 Add an intro or outro

    Keep \l startReady false until your intro finishes. The flow then plays
    the ready sound and waits for \l startDelayMillis before requesting play.

    For an outro, keep \l finishReady false, start the animation in
    \l finishRequested, and set finishReady to true when it ends. The flow
    still saves scores and opens results. An explicit \l exit skips this wait.
    Reset per-stage visuals in \l stageActivated.

    \section1 Handle courses and retries

    Each chart opens a normal result, including course stages. Closing a stage
    result advances the course. After the last stage result, the flow opens the
    course summary; closing it returns to selection. A single chart returns to
    selection after its result closes.

    Retry buttons bind to \l retryAvailable and call \l retry with false for a
    new random pattern or true for the same pattern. Standard quick retry is
    unavailable for courses, autoplay, replay, battle and Arena.

    In Arena, the flow requests play when presentation is ready; the session
    waits for the shared start time. The flow submits the score before showing
    results and releases the result presentation when that screen closes.

    \section1 Recover from a failed transition

    If the result screen cannot open, \l presentationFailed is emitted and the
    saved scores are kept. Call \l retryTransition to try again without saving
    twice. \l closing may therefore run more than once. Use it to stop or clear
    visuals; do not navigate from its handler.
*/
Item {
    id: root

    /*! Supplies the screen context for a single chart or a course. */
    property GameplayContext gameplay: null
    /*! Allows ready audio and startup when true. Bind it to the completion of your intro. */
    property bool startReady: true
    /*!
        Sets the delay in milliseconds after ready audio, or the full delay when no audio is
        available.
    */
    property int startDelayMillis: readySound.length > 0 ? 0 : 1000
    /*! Sets the ready sound. Use an empty URL to disable it. */
    property url readySoundSource: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "playready"
    /*! Controls whether a naturally finished chart may open its result. */
    property bool finishReady: true
    /*! Controls whether Escape and \l exit are accepted. */
    property bool exitEnabled: true
    /*! Controls whether quick retry and the Start+Select gesture are accepted. */
    property alias retryEnabled: input.retryEnabled
    /*! Enables standard Start, Select and scratch speed/cover controls. Disable for custom controls. */
    property alias speedControlsEnabled: input.speedControlsEnabled
    /*! Whether a supported retry can be requested from a button or gesture. */
    readonly property bool retryAvailable: input.retryAvailable
    /*! Calls retryAction(samePattern) instead of the standard restart. */
    property alias retryAction: input.retryAction
    /*! Start+Select hold time in milliseconds. */
    property alias retryHoldDurationMillis: input.retryHoldDurationMillis
    /*! Sets the exit sound for an attempted play. Use an empty URL to disable it. */
    property url exitFeedbackSource: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "playstop"
    /*!
        Called as \c dismissOverlayAction() before exit. Return true after closing an overlay
        to keep gameplay open.
    */
    property var dismissOverlayAction: null
    /*! Reports whether the retry gesture is waiting for a release choice. */
    readonly property bool retryChoosing: input.retryChoosing
    /*! Reports whether the current stage has received a scoring hit. */
    readonly property bool attempted: attemptState.attempted

    /*! Emitted once per active stage, before standard startup. */
    signal stageActivated()
    /*!
        Emitted when play finishes naturally. Start your outro here and bind finishReady to
        its completion.
    */
    signal finishRequested()
    /*!
        Emitted before leaving so the theme can clean up. It can repeat if result creation
        fails.
    */
    signal closing()
    /*! Emitted when result creation fails. Call retryTransition() to reuse the saved scores. */
    signal presentationFailed()

    /*!
        Retries with the same pattern when \a samePattern is true, otherwise a
        new one. Returns false when \l retryAvailable is false. A handled
        request returns true even if loading fails and the current screen stays.
    */
    function retry(samePattern) { return input.retry(samePattern); }
    /*! Cancels the held retry choice. */
    function cancelRetry() { input.cancelRetry(); }
    /*! Retries a failed result transition without saving the score again. */
    function retryTransition() { return session.retryTransition(); }

    /*!
        Leaves an unattempted local chart without saving. If a scoring hit was
        made, finishes the attempt and opens results. Arena follows its session's
        exit rules. Returns false while disabled, choosing a retry, or unable to leave.
        \l dismissOverlayAction may handle the request while keeping play open.
    */
    function exit() {
        if (!root.enabled || !root.exitEnabled || root.retryChoosing || !root.gameplay) {
            return false;
        }
        if (typeof root.dismissOverlayAction === "function" && root.dismissOverlayAction()) {
            return true;
        }
        if (root.gameplay.isArena && Rg.arenaSession.chatOpen) {
            Rg.arenaSession.setChatOpen(false);
            return true;
        }
        if (session.retryTransition()) {
            return true;
        }
        if (root.gameplay.isArena && root.gameplay.status < ChartRunner.Running) {
            Rg.arenaSession.abandonCurrentRound();
            return true;
        }
        if (!root.attempted && !root.gameplay.isArena && root.gameplay.status !== ChartRunner.Finished) {
            return session.leave();
        }
        if (!session.complete()) {
            return false;
        }
        stopSound.stop();
        stopSound.play();
        return true;
    }

    GameplaySession {
        id: session
        gameplay: root.gameplay
        navigation: globalRoot
        arenaSession: Rg.arenaSession
        active: root.enabled && !root.retryChoosing
        startReady: startup.complete && root.startReady
        finishReady: root.finishReady
        onStageActivated: {
            startup.reset();
            root.stageActivated();
            Qt.callLater(startup.begin);
        }
        onFinishRequested: root.finishRequested()
        onReadyChanged: Qt.callLater(startup.begin)
        onPlayingChanged: Qt.callLater(startup.begin)
        onClosing: {
            startup.reset();
            root.closing();
        }
        onPresentationFailed: root.presentationFailed()
    }

    QtObject {
        id: startup
        property bool armed: false
        property bool complete: false
        function reset() {
            startup.armed = false;
            startup.complete = false;
            startTimer.stop();
            readySound.stop();
        }
        function begin() {
            if (!root.enabled || !root.startReady || session.startRequested
                    || !session.playing || !session.ready || startup.armed || startup.complete) {
                return;
            }
            startup.armed = true;
            if (readySound.length > 0) {
                readySound.play();
            } else {
                startup.delay();
            }
        }
        function delay() {
            if (!startup.armed || !root.enabled) {
                return;
            }
            if (root.startDelayMillis <= 0) {
                startup.complete = true;
            } else {
                startTimer.restart();
            }
        }
    }

    AudioPlayer {
        id: readySound
        source: root.readySoundSource
        onPlayingChanged: if (!readySound.playing) startup.delay()
    }
    AudioPlayer {
        id: stopSound
        source: root.exitFeedbackSource
    }
    Timer {
        id: startTimer
        interval: Math.max(1, root.startDelayMillis)
        onTriggered: {
            if (startup.armed && root.enabled && root.startReady && session.playing && session.ready) {
                startup.complete = true;
            }
        }
    }
    StandardGameplayInput {
        id: input
        gameplay: root.gameplay
        exitEnabled: root.exitEnabled
        onExitRequested: root.exit()
    }
    StandardGameplayAttemptState {
        id: attemptState
        gameplay: root.gameplay
    }

    onEnabledChanged: {
        if (!root.enabled) {
            startup.reset();
            stopSound.stop();
        } else {
            Qt.callLater(startup.begin);
        }
    }
    onStartReadyChanged: {
        if (!root.startReady) startup.reset();
        else Qt.callLater(startup.begin);
    }
    onGameplayChanged: startup.reset()
}
