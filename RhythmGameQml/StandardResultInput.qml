import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardResultInput
    \inqmlmodule RhythmGameQml
    \brief Handles Continue and Retry actions on a result screen.

    Bind \l result to the supplied ResultContext or CourseResultContext. The
    component draws no controls. A Continue button calls \l confirm and binds
    its enabled state to \c {acceptsInput && confirmEnabled}. See the
    \l {../theme_tutorial_results.html}{result lesson} for complete screens.

    \section1 Confirm or close

    Input waits for \l inputDelayMillis after the screen becomes active.
    \l acceptsInput reports when this delay has passed.

    Return and \l confirm also require \l confirmEnabled. By default this is
    false while the matching Arena result's chat is open, so Return sends a
    message without closing the result. Escape and \l close bypass that
    confirmation rule, but still wait for the input delay.

    \l controllerEnabled controls bound game buttons independently of Return,
    Escape and calls from your own buttons. Start closes the result. Other
    bound keys first call \l tryHandleButtonAction, then try retry, then close
    for a standard play key. Use that callback for actions such as cycling gauges.

    \section1 Retry a chart

    Bind retry buttons to \l retryAvailable. Call \l retry with false for a new
    random pattern or true for the same pattern. Bound keys 5 and 7 use these
    same actions on either player side.

    Standard retry is unavailable for courses, autoplay, replay, battle and
    Arena. \l tryRetryAction can handle a request before StandardChartRetry.
    \l closeAction replaces closing the result entirely. Return true from a
    \c try...Action callback when it handled the request; return false to let
    the standard action continue.
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
