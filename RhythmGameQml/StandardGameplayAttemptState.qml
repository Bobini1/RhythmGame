import QtQuick
import RhythmGameQml

// Tracks whether a gameplay runner has received a scoring hit. This lower-level
// state is reusable even when a skin implements its own exit/result transition.
Item {
    id: root

    property var chart: null
    property bool attempted: false

    function reset() {
        attempted = false;
    }

    function hitCountsAsPlayed(tap) {
        let judgement = tap?.points?.judgement;
        return judgement !== undefined
            && judgement !== Judgement.Poor
            && judgement !== Judgement.EmptyPoor
            && judgement !== Judgement.MineHit
            && judgement !== Judgement.MineAvoided;
    }

    function observeHit(tap) {
        if (hitCountsAsPlayed(tap)) {
            attempted = true;
        }
    }

    onChartChanged: reset()

    Connections {
        target: root.chart?.player1?.score || null
        ignoreUnknownSignals: true
        function onHit(tap) {
            root.observeHit(tap);
        }
    }

    Connections {
        target: root.chart?.player2?.score || null
        ignoreUnknownSignals: true
        function onHit(tap) {
            root.observeHit(tap);
        }
    }

    Connections {
        target: root.chart
        ignoreUnknownSignals: true
        function onStatusChanged() {
            if (root.chart?.status === ChartRunner.Ready) {
                root.reset();
            }
        }
    }
}
