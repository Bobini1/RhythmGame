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

        function reset() {
            attemptState.attempted = false;
        }

        function observeHit(tap) {
            const judgement = tap?.points?.judgement;
            if (judgement !== undefined
                    && judgement !== Judgement.Poor
                    && judgement !== Judgement.EmptyPoor
                    && judgement !== Judgement.MineHit
                    && judgement !== Judgement.MineAvoided) {
                attemptState.attempted = true;
            }
        }
    }

    onGameplayChanged: attemptState.reset()

    Connections {
        target: root.gameplay?.players[0]?.score || null
        function onHit(tap) {
            attemptState.observeHit(tap);
        }
    }

    Connections {
        target: root.gameplay?.players[1]?.score || null
        function onHit(tap) {
            attemptState.observeHit(tap);
        }
    }

    Connections {
        target: root.gameplay
        function onStageChanged() { attemptState.reset(); }
        function onStatusChanged() {
            if (root.gameplay.status === ChartRunner.Ready) {
                attemptState.reset();
            }
        }
    }
}
