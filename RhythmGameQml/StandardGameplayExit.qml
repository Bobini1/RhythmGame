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
    /*! Whether Arena owns the gameplay completion transition. */
    property bool arenaOwned: false
    /*! Whether this component has already opened a result. */
    property bool resultOpened: false
    /*! Whether gameplay has produced a scoring hit. */
    property alias attempted: attemptState.attempted

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
        playstopSound.play();
        return complete();
    }

    AudioPlayer {
        id: playstopSound

        source: Rg.profileList.mainProfile.vars.generalVars.soundsetPath
            + "playstop"
    }

    Shortcut {
        enabled: root.enabled
        sequence: "Esc"
        onActivated: root.exit()
    }

    onChartChanged: resultOpened = false

    Connections {
        target: root.chart
        ignoreUnknownSignals: true
        function onStatusChanged() {
            if (root.chart?.status === ChartRunner.Ready) {
                root.resultOpened = false;
            } else if (root.enabled
                       && root.chart?.status === ChartRunner.Finished) {
                root.complete();
            }
        }
    }
}
