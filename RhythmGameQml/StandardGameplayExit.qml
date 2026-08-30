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
    property bool nothingWasHit: true
    property bool used: false

    function reset() {
        nothingWasHit = true;
        used = false;
    }

    function markHit() {
        nothingWasHit = false;
    }

    function closePresentation() {
        if (typeof closePresentationAction === "function") {
            closePresentationAction();
        }
    }

    function openResult(scores, profiles) {
        if (typeof openResultAction === "function") {
            return openResultAction(scores, profiles, chartData);
        }
        return globalRoot.openResult(scores, profiles, chartData);
    }

    function exit() {
        if (!enabled) {
            return false;
        }
        if (typeof exitAction === "function") {
            return exitAction();
        }
        if (arenaOwned && Rg.arenaSession.chatOpen === true) {
            Rg.arenaSession.setChatOpen(false);
            return true;
        }
        closePresentation();
        if (nothingWasHit && !arenaOwned) {
            globalRoot.returnToPreviousScreen();
            return true;
        }
        if (!chart) {
            return false;
        }
        playstopSound.play();
        used = true;
        let profiles = [chart.player1.profile,
                        chart.player2 ? chart.player2.profile : null];
        let scores = chart instanceof ChartRunner
            ? chart.finish()
            : chart.proceed();
        openResult(scores, profiles);
        return true;
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
}
