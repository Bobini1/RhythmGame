import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayAttemptState
    \inqmlmodule RhythmGameQml
    \brief Tracks whether a gameplay runner has received a scoring hit.

    This lower-level state remains useful when a skin implements its own
    gameplay exit or result transition.
*/
Item {
    id: root

    /*! Gameplay runner whose score signals are observed. */
    property var chart: null
    /*! Whether either player has produced a scoring hit. */
    property bool attempted: false

    /*! Clears the attempted state. */
    function reset() {
        attempted = false;
    }

    /*! Returns whether \a tap counts as attempting the chart. */
    function hitCountsAsPlayed(tap) {
        let judgement = tap?.points?.judgement;
        return judgement !== undefined
            && judgement !== Judgement.Poor
            && judgement !== Judgement.EmptyPoor
            && judgement !== Judgement.MineHit
            && judgement !== Judgement.MineAvoided;
    }

    /*! Updates \l attempted from \a tap. */
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
