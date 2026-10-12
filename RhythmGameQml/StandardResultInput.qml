import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardResultInput
    \inqmlmodule RhythmGameQml
    \brief Handles confirmation and retry on a result screen.

    Bind \l result to the ResultContext or CourseResultContext supplied to the
    screen. The theme draws the result and any buttons. Call \l confirm from a
    Continue button to use the same delay as keyboard confirmation.

    Actions wait until \l acceptsInput is true. Escape then closes the result
    regardless of \l confirmEnabled. Return and calls to \l confirm also require
    \l confirmEnabled. \l controllerEnabled controls bound controller input
    without disabling keyboard or pointer confirmation. By default, confirmEnabled
    follows the matching Arena round's chat state, so Return can send a message
    without closing the result. Override it to supply another confirmation rule.

    Controller button handling first calls \l tryHandleButtonAction. If it doesn't
    handle the button, the component tries retry, then closes for a standard play
    key. Start closes directly. Use the first callback for a display action such
    as cycling gauges, and return true when it handles the button.

    \l tryRetryAction can handle retry before StandardChartRetry runs. Otherwise,
    key 5 requests fresh randomization and key 7 requests the same pattern on either
    side. The originating play comes from \l result. Course summaries can be
    closed but cannot use single-chart retry.

    Retry buttons bind enabled to \l retryAvailable and call \l retry with false
    for fresh randomization or true for the same pattern. The same method and
    optional override serve both buttons and bound input.

    See the \l {../theme_tutorial_results.html}{result lesson} for separate chart
    and course result examples.
*/
Item {
    id: root

    /*! Supplies the ResultContext or CourseResultContext received by the screen. */
    property QtObject result: null
    /*! Calls \c closeAction() in place of closing the result. */
    property var closeAction: null
    /*!
        Called as \c tryRetryAction(samePattern). Return true to handle the request, or false to
        continue with the standard action.
    */
    property var tryRetryAction: null
    /*!
        Called as \c tryHandleButtonAction(key). Return true to handle the request, or false
        to continue with the standard action.
    */
    property var tryHandleButtonAction: null
    /*! Delay before result input becomes active, in milliseconds. */
    property int inputDelayMillis: 500
    /*! Reports whether result input currently accepts actions. */
    readonly property bool acceptsInput: inputDelayMillis <= 0
        || inputState.delayElapsed
    /*! Enables keyboard and pointer confirmation. Defaults to false while this result's Arena chat is open. */
    property bool confirmEnabled: !((root.result as ResultContext)?.arenaActive
        && Rg.arenaSession.chatOpen)
    /*! Controls whether BMS-controller input is active. */
    property bool controllerEnabled: true
    /*! Controls retries from both buttons and bound controller input. */
    property bool retryEnabled: true
    /*! Whether a supported retry can be requested now, including the input delay. */
    readonly property bool retryAvailable: enabled && acceptsInput && retryEnabled
        && (chartRetry.available || typeof tryRetryAction === "function")

    QtObject {
        id: inputState

        property bool delayElapsed: false

        function closeFromController() {
            return root.controllerEnabled && root.close();
        }
    }

    StandardChartRetry {
        id: chartRetry
        result: root.result
    }

    /*! Closes the result screen when input is accepted. */
    function close() {
        if (!enabled || !acceptsInput) {
            return false;
        }
        if (typeof closeAction === "function") {
            closeAction();
            return true;
        }
        globalRoot.returnToPreviousScreen();
        return true;
    }

    /*! Confirms and closes from keyboard or theme-provided pointer input. */
    function confirm() {
        return confirmEnabled && close();
    }

    /*! Retries with the same pattern when \a samePattern is true, or fresh randomization otherwise. */
    function retry(samePattern) {
        if (!enabled || !acceptsInput || !retryEnabled) {
            return false;
        }
        if (typeof tryRetryAction === "function" && tryRetryAction(samePattern)) {
            return true;
        }
        return chartRetry.retry(samePattern);
    }

    /*! Handles a standard result-screen \a key. */
    function handleButton(key) {
        if (!enabled || !acceptsInput || !controllerEnabled) {
            return false;
        }
        if (typeof tryHandleButtonAction === "function"
                && tryHandleButtonAction(key)) {
            return true;
        }
        const freshPattern = key === BmsKey.Col15 || key === BmsKey.Col25;
        const samePattern = key === BmsKey.Col17 || key === BmsKey.Col27;
        if ((freshPattern || samePattern) && retry(samePattern)) {
            return true;
        }
        if (StandardInputKeys.isPlayKey(key)) {
            close();
            return true;
        }
        return false;
    }

    Timer {
        interval: Math.max(1, root.inputDelayMillis)
        running: root.enabled && root.inputDelayMillis > 0
            && !inputState.delayElapsed
        repeat: false
        onTriggered: inputState.delayElapsed = true
    }

    onEnabledChanged: {
        if (!enabled) {
            inputState.delayElapsed = false;
        }
    }

    onInputDelayMillisChanged: inputState.delayElapsed = false

    Shortcut {
        enabled: root.enabled && root.acceptsInput
        sequence: "Esc"
        onActivated: root.close()
    }

    Shortcut {
        enabled: root.enabled && root.acceptsInput && root.confirmEnabled
        sequence: "Return"
        onActivated: root.confirm()
    }

    Input.onButtonPressed: key => root.handleButton(key)
    Input.onStart1Pressed: inputState.closeFromController()
    Input.onStart2Pressed: inputState.closeFromController()
}
