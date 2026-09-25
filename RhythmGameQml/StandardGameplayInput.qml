import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardGameplayInput
    \inqmlmodule RhythmGameQml
    \brief Maps Escape and the Start+Select retry gesture for gameplay.

    Use StandardGameplayFlow for a complete gameplay screen. It already includes
    this component. Use StandardGameplayInput separately only when your theme
    supplies its own gameplay flow. Handle \l exitRequested to leave gameplay
    or finish the stage. This component does not save scores or open results.

    Hold Start+Select for \l retryHoldDurationMillis to enter \l retryChoosing.
    Release Start for the same pattern, Select for fresh randomization, or both
    to cancel. While choosing, exit is blocked. Delay completion and disable your
    other controls with \c {!retryChoosing}, but leave this component enabled so
    it can receive the releases.

    \l retryEnabled and \l exitEnabled disable the corresponding methods and
    input mappings. \l retryAction replaces restarting, and \l retry can also be
    called by a button. Standard retry excludes courses, autoplay, replay, battle
    and Arena. Use StandardChartRetry if you only need the retry operation.
*/
Item {
    id: root

    /*! Supplies the GameplayContext received by the gameplay screen. */
    property GameplayContext gameplay: null
    /*! Controls whether Escape and calls to \l exit are accepted. */
    property bool exitEnabled: true
    /*! Controls whether retry and the Start+Select gesture are enabled. */
    property bool retryEnabled: true
    /*! Start+Select hold duration before choosing a retry, in milliseconds. */
    property int retryHoldDurationMillis: 1000
    /*! Calls \c retryAction(samePattern) in place of restarting. */
    property var retryAction: null
    /*! Reports whether the Start+Select gesture is waiting for a release choice. */
    readonly property bool retryChoosing: inputState.choosing

    /*! Emitted by Escape or \l exit. The owning flow decides how to leave. */
    signal exitRequested()

    StandardChartRetry {
        id: chartRetry
        gameplay: root.gameplay
    }

    QtObject {
        id: inputState
        property int side: 0
        property bool choosing: false
        property bool retrying: false
        readonly property bool retryAvailable: root.retryEnabled && !root.gameplay?.isArena
            && (typeof root.retryAction === "function" || chartRetry.available)
        readonly property int buttons: (root.Input.start1 ? 1 : 0)
            | (root.Input.select1 ? 2 : 0)
            | (root.Input.start2 ? 4 : 0)
            | (root.Input.select2 ? 8 : 0)

        onRetryAvailableChanged: if (!inputState.retryAvailable) root.cancelRetry()

        onButtonsChanged: {
            if (!root.enabled || !inputState.retryAvailable) {
                return;
            }
            if (root.retryChoosing) {
                Qt.callLater(inputState.evaluateChoice);
                return;
            }
            const side = (inputState.buttons & 3) === 3 ? 1
                : ((inputState.buttons & 12) === 12 ? 2 : 0);
            if (!side) {
                root.cancelRetry();
            } else if (side !== inputState.side || !holdTimer.running) {
                inputState.side = side;
                holdTimer.restart();
            }
        }

        function heldButtons() {
            return inputState.side === 1
                ? inputState.buttons & 3 : (inputState.buttons >> 2) & 3;
        }

        function evaluateChoice() {
            if (!root.enabled || !root.retryChoosing) {
                return;
            }
            const buttons = inputState.heldButtons();
            if (buttons === 0) {
                root.cancelRetry();
            } else if (buttons !== 3) {
                root.retry(buttons === 2);
            }
        }
    }

    /*! Cancels the hold/release choice. */
    function cancelRetry() {
        holdTimer.stop();
        inputState.side = 0;
        inputState.choosing = false;
    }

    /*! Retries with the same pattern if \a samePattern is true, or a fresh one. */
    function retry(samePattern) {
        if (!root.enabled || !inputState.retryAvailable || inputState.retrying) {
            return false;
        }
        inputState.retrying = true;
        try {
            if (typeof root.retryAction === "function") {
                root.retryAction(samePattern);
                return true;
            }
            return chartRetry.retry(samePattern);
        } finally {
            inputState.retrying = false;
            root.cancelRetry();
        }
    }

    Timer {
        id: holdTimer
        interval: Math.max(1, root.retryHoldDurationMillis)
        onTriggered: {
            if (root.enabled && inputState.retryAvailable
                    && root.gameplay?.status !== ChartRunner.Finished
                    && inputState.heldButtons() === 3) {
                inputState.choosing = true;
            }
        }
    }

    /*! Emits exitRequested() if exit input is allowed. */
    function exit() {
        if (!enabled || !root.exitEnabled || root.retryChoosing || inputState.retrying) {
            return false;
        }
        root.exitRequested();
        return true;
    }

    Shortcut {
        enabled: root.enabled && root.exitEnabled && !root.retryChoosing
        sequence: "Esc"
        onActivated: root.exit()
    }

    onGameplayChanged: root.cancelRetry()
    onEnabledChanged: if (!root.enabled) root.cancelRetry()
}
