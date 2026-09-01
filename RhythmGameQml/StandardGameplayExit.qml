import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayExit
    \inqmlmodule RhythmGameQml
    \brief Provides standard Escape handling for gameplay.

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
    chart.
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
    /*! Default attempted-exit audio source. */
    property url exitFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "playstop"
    /*! Whether attempted-exit feedback is active. */
    property bool exitFeedbackEnabled: true
    /*! Whether Arena owns the gameplay completion transition. */
    property bool arenaOwned: false
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
    }

    StandardGameplayAttemptState {
        id: attemptState
        chart: root.chart
    }

    /*! Applies the standard abandon-or-complete decision. */
    function exit() {
        if (!enabled) {
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

        source: root.exitFeedbackSource
    }

    Shortcut {
        enabled: root.enabled
        sequence: "Esc"
        onActivated: root.exit()
    }

    onChartChanged: {
        exitState.resultOpened = false;
        exitState.completionPending = false;
    }

    onEnabledChanged: {
        if (enabled && exitState.completionPending) {
            Qt.callLater(function() {
                if (root.enabled && exitState.completionPending) {
                    exitState.complete();
                }
            });
        }
    }

    Connections {
        target: root.chart
        ignoreUnknownSignals: true
        function onStatusChanged() {
            if (root.chart?.status === ChartRunner.Ready) {
                exitState.resultOpened = false;
                exitState.completionPending = false;
            } else if (root.chart?.status === ChartRunner.Finished) {
                if (root.enabled) {
                    exitState.complete();
                } else {
                    exitState.completionPending = true;
                }
            }
        }
    }
}
