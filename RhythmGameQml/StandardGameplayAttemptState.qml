import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayAttemptState
    \inqmlmodule RhythmGameQml
    \brief Tracks whether a gameplay runner has received a scoring hit.

    This lower-level state remains useful when a skin implements its own
    gameplay exit or result transition.

    A hit marks the play attempted unless its judgement is Poor, EmptyPoor,
    MineHit, or MineAvoided. Changing \l chart, or the current chart returning
    to \c ChartRunner.Ready, resets \l attempted. The component observes both
    player scores when present.
*/
Item {
    id: root

    /*! Gameplay runner whose score signals are observed. */
    property var chart: null
    /*! Whether either player has produced a scoring hit. */
    readonly property bool attempted: attemptState.attempted

    QtObject {
        id: attemptState

        property bool attempted: false

        function hitCountsAsPlayed(tap) {
            let judgement = tap?.points?.judgement;
            return judgement !== undefined
                && judgement !== Judgement.Poor
                && judgement !== Judgement.EmptyPoor
                && judgement !== Judgement.MineHit
                && judgement !== Judgement.MineAvoided;
        }
    }

    /*! Clears the attempted state. */
    function reset() {
        attemptState.attempted = false;
    }

    /*! Updates \l attempted from \a tap. */
    function observeHit(tap) {
        if (attemptState.hitCountsAsPlayed(tap)) {
            attemptState.attempted = true;
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
