import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayExit
    \inqmlmodule RhythmGameQml
    \brief Provides standard Escape handling for gameplay.

    The component distinguishes abandoning before a hit from finishing an
    attempted play, while leaving presentation cleanup and result opening
    overridable.
*/
Item {
    id: root

    /*! Gameplay runner being exited. */
    property var chart: null
    /*! Chart data passed to the result screen. */
    property var chartData: null
    /*! Optional replacement for the entire exit decision. */
    property var exitAction: null
    /*! Optional action that closes skin-owned overlays before exiting. */
    property var closePresentationAction: null
    /*! Optional replacement for opening the result screen. */
    property var openResultAction: null
    /*! Optional replacement for attempted-exit audio feedback. */
    property var exitFeedbackAction: null
    /*! Default attempted-exit audio source. */
    property url exitFeedbackSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "playstop"
    /*! Whether attempted-exit feedback is active. */
    property bool exitFeedbackEnabled: true
    /*! Whether Arena owns the gameplay completion transition. */
    property bool arenaOwned: false
    /*! Whether this component has already opened a result. */
    property bool resultOpened: false
    /*! Whether gameplay has produced a scoring hit. */
    property alias attempted: attemptState.attempted

    QtObject {
        id: exitState

        property bool completionPending: false
    }

    StandardGameplayAttemptState {
        id: attemptState
        chart: root.chart
    }

    /*! Closes skin-owned gameplay presentation, if configured. */
    function closePresentation() {
        if (typeof closePresentationAction === "function") {
            closePresentationAction();
        }
    }

    /*! Opens the result for \a scores and \a profiles. */
    function openResult(scores, profiles) {
        if (typeof openResultAction === "function") {
            openResultAction(scores, profiles, chartData);
            return true;
        }
        globalRoot.openResult(scores, profiles, chartData);
        return true;
    }

    /*! Completes attempted gameplay and opens or delegates its result. */
    function complete() {
        if (!chart || resultOpened) {
            return false;
        }
        exitState.completionPending = false;
        resultOpened = true;
        closePresentation();
        let profiles = [chart.player1.profile,
                        chart.player2 ? chart.player2.profile : null];
        let scores = chart instanceof ChartRunner
            ? chart.finish()
            : chart.proceed();
        openResult(scores, profiles);
        return true;
    }

    /*! Plays or invokes attempted-exit feedback. */
    function playExitFeedback() {
        if (!exitFeedbackEnabled) {
            return;
        }
        if (typeof exitFeedbackAction === "function") {
            exitFeedbackAction();
            return;
        }
        playstopSound.stop();
        playstopSound.play();
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
            closePresentation();
            globalRoot.returnToPreviousScreen();
            return true;
        }
        if (!chart) {
            return false;
        }
        playExitFeedback();
        return complete();
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
        resultOpened = false;
        exitState.completionPending = false;
    }

    onEnabledChanged: {
        if (enabled && exitState.completionPending) {
            Qt.callLater(function() {
                if (root.enabled && exitState.completionPending) {
                    root.complete();
                }
            });
        }
    }

    Connections {
        target: root.chart
        ignoreUnknownSignals: true
        function onStatusChanged() {
            if (root.chart?.status === ChartRunner.Ready) {
                root.resultOpened = false;
                exitState.completionPending = false;
            } else if (root.chart?.status === ChartRunner.Finished) {
                if (root.enabled) {
                    root.complete();
                } else {
                    exitState.completionPending = true;
                }
            }
        }
    }
}
