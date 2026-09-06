import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayInput
    \inqmlmodule RhythmGameQml
    \brief Coordinates gameplay exit, quick retry and result completion.

    The component distinguishes abandoning before a hit from finishing an
    attempted play, while leaving presentation cleanup and result opening
    overridable.

    The default \l exit decision is:

    \table
        \header
            \li State
            \li Result
        \row
            \li Arena chat is open
            \li Close chat and remain in gameplay
        \row
            \li No scoring hit and not Arena-owned
            \li Run \l closePresentationAction and return without a result
        \row
            \li A scoring hit occurred, or Arena owns the runner
            \li Play exit feedback, close the presentation, finish/proceed the
                runner, and open results
    \endtable

    \l exitAction replaces that entire decision. \l openResultAction is called
    as \c openResultAction(scores, profiles, chartData). If a runner reaches
    \c ChartRunner.Finished while this component is disabled, completion is
    deferred until it becomes enabled; the result is opened at most once per
    chart. \l completionEnabled can disable automatic completion for skins
    that implement their own finish timing.

    Hold Start+Select on either side for \l retryHoldDurationMillis. During
    \l retryChoosing, release Start for the same pattern, Select for fresh
    randomization, or both to cancel. Gameplay stays visible and standard exit
    and completion wait until the choice ends. Gate other skin-owned controls
    with !retryChoosing, but keep this component enabled to receive releases.

    \l retryEnabled and \l exitEnabled disable the respective actions and key
    mappings. \l retryAction replaces restarting; \l retry can also be called
    from a skin button. Standard retry excludes courses, autoplay, replay,
    battle and Arena. Use \l StandardChartRetry for retry without these input
    mappings or completion handling.
*/
Item {
    id: root

    /*! Gameplay runner being exited. */
    property var chart: null
    /*! Chart data passed to the result screen. */
    property var chartData: null
    /*! Optional \c exitAction() replacement for the entire exit decision. */
    property var exitAction: null
    /*! Optional \c closePresentationAction() called before leaving gameplay. */
    property var closePresentationAction: null
    /*! Optional \c openResultAction(scores,profiles,chartData) replacement. */
    property var openResultAction: null
    /*! Optional \c exitFeedbackAction() replacement for attempted-exit audio. */
    property var exitFeedbackAction: null
    /*! Default attempted-exit audio source, loaded only for built-in feedback. */
    property url exitFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "playstop"
    /*! Whether attempted-exit feedback is active. */
    property bool exitFeedbackEnabled: true
    /*! Whether Arena owns the gameplay completion transition. */
    property bool arenaOwned: false
    /*! Whether Escape and calls to \l exit are accepted. */
    property bool exitEnabled: true
    /*! Whether finished charts automatically open their result. */
    property bool completionEnabled: true
    /*! Whether retry and the Start+Select gesture are enabled. */
    property bool retryEnabled: true
    /*! Start+Select hold duration before choosing a retry, in milliseconds. */
    property int retryHoldDurationMillis: 1000
    /*! Optional \c retryAction(samePattern) replacement for restarting. */
    property var retryAction: null
    /*! Whether the Start+Select gesture is waiting for a release choice. */
    readonly property bool retryChoosing: inputState.choosing
    /*! Whether gameplay has produced a scoring hit. */
    readonly property bool attempted: attemptState.attempted

    QtObject {
        id: exitState

        property bool completionPending: false
        property bool resultOpened: false

        function closePresentation() {
            if (typeof root.closePresentationAction === "function") {
                root.closePresentationAction();
            }
        }

        function openResult(scores, profiles) {
            if (typeof root.openResultAction === "function") {
                root.openResultAction(scores, profiles, root.chartData);
                return true;
            }
            globalRoot.openResult(scores, profiles, root.chartData);
            return true;
        }

        function complete() {
            if (!root.chart || exitState.resultOpened) {
                return false;
            }
            exitState.completionPending = false;
            exitState.resultOpened = true;
            exitState.closePresentation();
            let profiles = [root.chart.player1.profile,
                            root.chart.player2
                                ? root.chart.player2.profile : null];
            let scores = root.chart instanceof ChartRunner
                ? root.chart.finish()
                : root.chart.proceed();
            exitState.openResult(scores, profiles);
            return true;
        }

        function playExitFeedback() {
            if (!root.exitFeedbackEnabled) {
                return;
            }
            if (typeof root.exitFeedbackAction === "function") {
                root.exitFeedbackAction();
                return;
            }
            playstopSound.stop();
            playstopSound.play();
        }

        function completePending() {
            if (root.enabled && root.completionEnabled && !root.retryChoosing
                    && !inputState.retrying && exitState.completionPending) {
                exitState.complete();
            }
        }
    }

    StandardGameplayAttemptState {
        id: attemptState
        chart: root.chart
    }

    StandardChartRetry {
        id: chartRetry
        chart: root.chart
    }

    QtObject {
        id: inputState
        property int side: 0
        property bool choosing: false
        property bool retrying: false
        readonly property bool retryAvailable: root.retryEnabled && !root.arenaOwned
            && (typeof root.retryAction === "function" || chartRetry.available)
        readonly property int buttons: (root.Input.start1 ? 1 : 0)
            | (root.Input.select1 ? 2 : 0)
            | (root.Input.start2 ? 4 : 0)
            | (root.Input.select2 ? 8 : 0)

        onRetryAvailableChanged: if (!inputState.retryAvailable) root.cancelRetry()

        onButtonsChanged: {
            if (!root.enabled || !inputState.retryAvailable) {
                return;
            }
            if (root.retryChoosing) {
                Qt.callLater(inputState.evaluateChoice);
                return;
            }
            const side = (inputState.buttons & 3) === 3 ? 1
                : ((inputState.buttons & 12) === 12 ? 2 : 0);
            if (!side) {
                root.cancelRetry();
            } else if (side !== inputState.side || !holdTimer.running) {
                inputState.side = side;
                holdTimer.restart();
            }
        }

        function heldButtons() {
            return inputState.side === 1
                ? inputState.buttons & 3 : (inputState.buttons >> 2) & 3;
        }

        function evaluateChoice() {
            if (!root.enabled || !root.retryChoosing) {
                return;
            }
            const buttons = inputState.heldButtons();
            if (buttons === 0) {
                root.cancelRetry();
            } else if (buttons !== 3) {
                root.retry(buttons === 2);
            }
        }
    }

    /*! Cancels the hold/release choice and resumes any pending completion. */
    function cancelRetry() {
        holdTimer.stop();
        inputState.side = 0;
        inputState.choosing = false;
    }

    /*! Retries with the same pattern if \a samePattern is true, or a fresh one. */
    function retry(samePattern) {
        if (!root.enabled || !inputState.retryAvailable || inputState.retrying) {
            return false;
        }
        inputState.retrying = true;
        try {
            if (typeof root.retryAction === "function") {
                root.retryAction(samePattern);
                return true;
            }
            return chartRetry.retry(samePattern);
        } finally {
            inputState.retrying = false;
            root.cancelRetry();
        }
    }

    Timer {
        id: holdTimer
        interval: Math.max(1, root.retryHoldDurationMillis)
        onTriggered: {
            if (root.enabled && inputState.retryAvailable
                    && root.chart?.status !== ChartRunner.Finished
                    && inputState.heldButtons() === 3) {
                inputState.choosing = true;
            }
        }
    }

    /*! Applies the standard abandon-or-complete decision. */
    function exit() {
        if (!enabled || !root.exitEnabled || root.retryChoosing || inputState.retrying) {
            return false;
        }
        if (typeof exitAction === "function") {
            exitAction();
            return true;
        }
        if (arenaOwned && Rg.arenaSession.chatOpen === true) {
            Rg.arenaSession.setChatOpen(false);
            return true;
        }
        if (!attempted && !arenaOwned) {
            exitState.closePresentation();
            globalRoot.returnToPreviousScreen();
            return true;
        }
        if (!chart) {
            return false;
        }
        exitState.playExitFeedback();
        return exitState.complete();
    }

    AudioPlayer {
        id: playstopSound

        source: root.exitFeedbackEnabled
                && typeof root.exitFeedbackAction !== "function"
            ? root.exitFeedbackSource : ""
    }

    Shortcut {
        enabled: root.enabled && root.exitEnabled && !root.retryChoosing
        sequence: "Esc"
        onActivated: root.exit()
    }

    onChartChanged: {
        root.cancelRetry();
        exitState.resultOpened = false;
        exitState.completionPending = false;
    }

    onEnabledChanged: {
        if (!root.enabled) {
            root.cancelRetry();
        }
        Qt.callLater(exitState.completePending);
    }
    onRetryChoosingChanged: Qt.callLater(exitState.completePending)
    onCompletionEnabledChanged: Qt.callLater(exitState.completePending)

    Connections {
        target: root.chart
        ignoreUnknownSignals: true
        function onStatusChanged() {
            if (root.chart?.status === ChartRunner.Ready) {
                exitState.resultOpened = false;
                exitState.completionPending = false;
            } else if (root.chart?.status === ChartRunner.Finished) {
                exitState.completionPending = true;
                exitState.completePending();
            }
        }
    }
}
