import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayFlow
    \inqmlmodule RhythmGameQml
    \brief Starts gameplay, handles exit and retry, and opens results.

    Place one instance inside the gameplay screen and bind \l gameplay to the
    supplied context. The component uses its parent as the owning screen. Set
    \l screen explicitly when you place it inside another item.

    The flow handles ready audio and startup, Escape, and Start+Select retry.
    It opens a normal result after every chart, including each course stage.
    Closing a stage result continues the course. After the last stage result,
    it opens the separate course summary. Closing that summary returns to selection.
    For a single chart, closing its result returns to selection.
    StandardGameplayInput and StandardGameplayAttemptState are already included.
    Don't add another instance of either component to the same screen.

    Use \l startReady to wait for an intro before starting the ready sequence.
    For an outro, start the animation in \l finishRequested and bind \l finishReady
    to its completion. The flow still saves scores and opens results. An explicit
    \l exit skips the wait for a natural-finish animation.

    \l stageActivated is emitted once for each stage before startup. Use it to
    reset visuals or request score data. \l closing lets the skin clean up its
    presentation before leaving. It can repeat if result creation fails, so the
    handler must tolerate another call and must not navigate away itself.

    Inactive screens cannot start play or navigate. Arena controls its own startup
    and does not allow quick retry. The flow submits Arena scores and ends the
    result presentation when its result screen closes. If result creation fails,
    \l presentationFailed is emitted and the saved scores are kept. Call \l retryTransition to try
    opening the result again without saving it twice.

    See the \l {../skin_tutorial_gameplay.html}{gameplay lesson} for a complete
    example with notes and course support.
*/
Item {
    id: root

    /*! Supplies the screen context for a single chart or a course. */
    property GameplayContext gameplay: null
    /*!
        Sets the owning screen. The default is the parent, so set it explicitly when nesting
        the component.
    */
    property var screen: root.parent
    /*! Reports whether Arena manages this play. */
    readonly property bool arenaManagedRunner: root.gameplay ? root.gameplay.isArena : false
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
    /*! Current stage data, including the last stage after course completion. */
    readonly property var chartData: root.gameplay ? root.gameplay.chartData : null
    /*! Reports whether this runner contains a course. */
    readonly property bool isCourse: root.gameplay ? root.gameplay.isCourse : false
    /*! Reports whether this component owns the currently presented screen. */
    readonly property bool active: root.enabled && !!root.screen
        && globalRoot.currentScreen === root.screen

    /*! Emitted once per active stage, before standard startup. */
    signal stageActivated()
    /*!
        Emitted when play finishes naturally. Start your outro here and bind finishReady to
        its completion.
    */
    signal finishRequested()
    /*!
        Emitted before leaving so the skin can clean up. It can repeat if result creation
        fails.
    */
    signal closing()
    /*! Emitted when result creation fails. Call retryTransition() to reuse the saved scores. */
    signal presentationFailed()

    /*! Retries with the same pattern when \a samePattern is true, otherwise a fresh one. */
    function retry(samePattern) { return input.retry(samePattern); }
    /*! Cancels the held retry choice. */
    function cancelRetry() { input.cancelRetry(); }
    /*! Retries a failed result transition without saving the score again. */
    function retryTransition() { return session.retryTransition(); }

    /*! Abandons untouched play, or completes an attempted play. */
    function exit() {
        if (!root.active || !root.exitEnabled || root.retryChoosing || !root.gameplay) {
            return false;
        }
        if (typeof root.dismissOverlayAction === "function" && root.dismissOverlayAction()) {
            return true;
        }
        if (root.arenaManagedRunner && Rg.arenaSession.chatOpen) {
            Rg.arenaSession.setChatOpen(false);
            return true;
        }
        if (session.retryTransition()) {
            return true;
        }
        if (!root.attempted && !root.arenaManagedRunner && root.gameplay.status !== ChartRunner.Finished) {
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
        active: root.active && !root.retryChoosing
        startReady: startup.complete && root.startReady
        finishReady: root.finishReady
        onStageActivated: {
            startup.reset();
            root.stageActivated();
            Qt.callLater(startup.begin);
        }
        onFinishRequested: root.finishRequested()
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
            if (!root.active || !root.startReady || root.arenaManagedRunner
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
            if (!startup.armed || !root.active) {
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
        source: root.arenaManagedRunner ? "" : root.readySoundSource
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
            if (startup.armed && root.active && root.startReady && session.playing && session.ready) {
                startup.complete = true;
            }
        }
    }
    StandardGameplayInput {
        id: input
        gameplay: root.gameplay
        enabled: root.active
        exitEnabled: root.exitEnabled
        onExitRequested: root.exit()
    }
    StandardGameplayAttemptState {
        id: attemptState
        gameplay: root.gameplay
    }

    onActiveChanged: {
        if (!root.active) {
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
