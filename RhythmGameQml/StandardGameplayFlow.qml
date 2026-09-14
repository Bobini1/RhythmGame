import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayFlow
    \inqmlmodule RhythmGameQml
    \brief Owns the gameplay lifecycle while the skin owns its presentation.

    Place one instance directly inside a gameplay screen and bind \l chart to
    the injected runner. This includes ready audio and startup, Escape,
    attempted-play tracking, Start+Select retry, stage results, course
    continuation, course results and return navigation. Do not also instantiate
    StandardGameplayInput or StandardGameplayAttemptState on that screen.

    \l startReady gates startup; \l finishReady gates natural completion. Use
    \l finishRequested to start an outro and bind finishReady to its completion.
    These gates change presentation timing without replacing score saving or
    navigation. Explicit \l exit skips the natural-finish gate.

    \l stageActivated runs once when each stage becomes current, before startup.
    Use it for presentation resets or score queries. \l closing is a cleanup
    notification, not a request to navigate. Inactive screens cannot start a
    runner or change the stack. Arena starts its own runner and cannot retry.

    Failed result creation emits \l presentationFailed. The saved result is
    retained; \l retryTransition retries presentation without saving again.
*/
Item {
    id: root

    /*! The ChartRunner or CourseRunner supplied to the gameplay screen. */
    required property var chart
    /*! Owning screen; set explicitly when nesting this component. */
    property var screen: root.parent
    /*! Whether the screen's runner is managed by Arena. */
    property bool arenaManagedRunner: root.screen?.arenaManagedRunner === true
    /*! Presentation readiness before playing ready audio and starting. */
    property bool startReady: true
    /*! Delay after ready audio, or the whole delay when no sound is available. */
    property int startDelayMillis: readySound.length > 0 ? 0 : 1000
    /*! Ready audio; an empty URL disables it. */
    property url readySoundSource: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "playready"
    /*! Whether a naturally finished chart may open its result. */
    property bool finishReady: true
    /*! Whether Escape and \l exit are accepted. */
    property bool exitEnabled: true
    /*! Whether quick retry and the Start+Select gesture are accepted. */
    property alias retryEnabled: input.retryEnabled
    /*! Start+Select hold time in milliseconds. */
    property alias retryHoldDurationMillis: input.retryHoldDurationMillis
    /*! Attempted-exit audio; an empty URL disables it. */
    property url exitFeedbackSource: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "playstop"
    /*! Whether an exit should first dismiss a skin-owned overlay. */
    property var dismissOverlayAction: null
    /*! Whether the retry gesture is waiting for a release choice. */
    readonly property bool retryChoosing: input.retryChoosing
    /*! Whether the current stage has received a scoring hit. */
    readonly property bool attempted: input.attempted
    /*! Current stage data, including the last stage after course completion. */
    readonly property var chartData: session.chartData
    /*! Whether this runner contains a course. */
    readonly property bool isCourse: session.isCourse
    /*! Whether this component owns the currently presented screen. */
    readonly property bool active: root.enabled && !!root.screen
        && globalRoot.currentScreen === root.screen

    /*! Emitted once per active stage, before standard startup. */
    signal stageActivated()
    /*! Emitted on natural finish; start an outro and bind finishReady to it. */
    signal finishRequested()
    /*! Cleanup before departure; can repeat after failed result creation. */
    signal closing()
    /*! Result creation failed; retryTransition() reuses the saved scores. */
    signal presentationFailed()

    /*! Retries with the same pattern when \a samePattern is true, otherwise a fresh one. */
    function retry(samePattern) { return input.retry(samePattern); }
    /*! Cancels the held retry choice. */
    function cancelRetry() { input.cancelRetry(); }
    /*! Retries a failed result transition without saving the score again. */
    function retryTransition() { return session.retryTransition(); }

    /*! Abandons untouched play, or completes an attempted play. */
    function exit() {
        if (!root.active || !root.exitEnabled || root.retryChoosing || !root.chart) {
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
        if (!input.attempted && !root.arenaManagedRunner && root.chart.status !== ChartRunner.Finished) {
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
        chart: root.chart
        navigation: globalRoot
        active: root.active && !root.retryChoosing
        arenaManaged: root.arenaManagedRunner
        ready: root.chart?.status === ChartRunner.Ready
        finished: root.chart?.status === ChartRunner.Finished
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
        chart: root.chart
        chartData: root.chartData
        enabled: root.active
        arenaOwned: root.arenaManagedRunner
        exitEnabled: root.exitEnabled
        exitAction: () => root.exit()
        completionEnabled: false
        exitFeedbackEnabled: false
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
    onChartChanged: startup.reset()
}
