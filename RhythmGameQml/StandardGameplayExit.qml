import QtQuick
import RhythmGameQml

// Standard Escape handling for gameplay. It distinguishes abandoning before a
// hit from finishing an attempted play, while leaving popup cleanup overridable.
Item {
    id: root

    property var chart: null
    property var chartData: null
    property var exitAction: null
    property var closePresentationAction: null
    property var openResultAction: null
    property bool arenaOwned: false
    property bool resultOpened: false
    property alias attempted: attemptState.attempted

    StandardGameplayAttemptState {
        id: attemptState
        chart: root.chart
    }

    function closePresentation() {
        if (typeof closePresentationAction === "function") {
            closePresentationAction();
        }
    }

    function openResult(scores, profiles) {
        if (typeof openResultAction === "function") {
            openResultAction(scores, profiles, chartData);
            return true;
        }
        globalRoot.openResult(scores, profiles, chartData);
        return true;
    }

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
