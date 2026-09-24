import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardResultInput
    \inqmlmodule RhythmGameQml
    \brief Handles confirmation and retry on a result screen.

    Bind \l result to the ResultContext or CourseResultContext supplied to the
    screen. The skin draws the result and any buttons. Call \l confirm from a
    Continue button to use the same delay as keyboard confirmation.

    Actions wait until \l acceptsInput is true. Escape then closes the result
    regardless of \l confirmEnabled. Return and calls to \l confirm also require
    \l confirmEnabled. \l controllerEnabled controls bound controller input
    without disabling keyboard or pointer confirmation.

    Controller button handling first calls \l tryHandleButtonAction. If it doesn't
    handle the button, the component tries retry, then closes for a standard play
    key. Start closes directly. Use the first callback for a display action such
    as cycling gauges, and return true when it handles the button.

    \l tryRetryAction can handle retry before StandardChartRetry runs. Otherwise,
    key 5 requests fresh randomization and key 7 requests the same pattern on either
    side. The originating play comes from \l result. Course summaries can be
    closed but cannot use single-chart retry.

    See the \l {../skin_tutorial_results.html}{result lesson} for separate chart
    and course result examples.
*/
Item {
    id: root

    /*! Supplies the ResultContext or CourseResultContext received by the screen. */
    property QtObject result: null
    /*! Calls \c closeAction() in place of closing the result. */
    property var closeAction: null
    /*!
        Called as \c tryRetryAction(key). Return true to handle the request, or false to
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
    /*! Controls whether semantic keyboard or pointer confirmation is active. */
    property bool confirmEnabled: true
    /*! Controls whether BMS-controller input is active. */
    property bool controllerEnabled: true

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

    /*! Confirms and closes from keyboard or skin-provided pointer input. */
    function confirm() {
        return confirmEnabled && close();
    }

    /*! Uses \a key to retry with a fresh pattern for key 5 or the same pattern for key 7 on either side. */
    function retry(key) {
        if (!enabled || !acceptsInput) {
            return false;
        }
        if (typeof tryRetryAction === "function" && tryRetryAction(key)) {
            return true;
        }
        switch (key) {
        case BmsKey.Col15:
        case BmsKey.Col25:
            return chartRetry.retry(false);
        case BmsKey.Col17:
        case BmsKey.Col27:
            return chartRetry.retry(true);
        default:
            return false;
        }
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
        if (retry(key)) {
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
