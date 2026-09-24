import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayAttemptState
    \inqmlmodule RhythmGameQml
    \brief Reports whether either player has made a scoring hit.

    Use this component when writing your own gameplay exit or result flow.
    StandardGameplayFlow already includes it.

    Bind \l gameplay to the supplied context. A hit sets \l attempted unless its
    judgement is Poor, EmptyPoor, MineHit or MineAvoided. Changing the context or
    returning the current chart to \c ChartRunner.Ready resets the flag.
    The component observes both players when a second player is present.
*/
Item {
    id: root

    /*! Supplies the GameplayContext whose player scores are observed. */
    property GameplayContext gameplay: null
    /*! Reports whether either player has produced a scoring hit. */
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

    onGameplayChanged: reset()

    Connections {
        target: root.gameplay?.players[0]?.score || null
        function onHit(tap) {
            root.observeHit(tap);
        }
    }

    Connections {
        target: root.gameplay?.players[1]?.score || null
        function onHit(tap) {
            root.observeHit(tap);
        }
    }

    Connections {
        target: root.gameplay
        function onStageChanged() { root.reset(); }
        function onStatusChanged() {
            if (root.gameplay.status === ChartRunner.Ready) {
                root.reset();
            }
        }
    }
}
