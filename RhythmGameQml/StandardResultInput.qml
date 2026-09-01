import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardResultInput
    \inqmlmodule RhythmGameQml
    \brief Provides common result-screen dismissal and retry input.

    Result presentation and optional button actions such as gauge cycling
    remain with the skin.
*/
Item {
    id: root

    /*! Optional replacement for closing the result screen. */
    property var closeAction: null
    /*! Optional retry pre-handler. True consumes; false or undefined continues. */
    property var tryRetryAction: null
    /*! Optional button pre-handler. True consumes; false or undefined continues. */
    property var tryHandleButtonAction: null
    /*! Delay before result input becomes active, in milliseconds. */
    property int inputDelayMillis: 500
    /*! Whether result input currently accepts actions. */
    readonly property bool acceptsInput: inputDelayMillis <= 0
        || inputState.delayElapsed
    /*! Whether semantic keyboard or pointer confirmation is active. */
    property bool confirmEnabled: true
    /*! Whether BMS-controller input is active. */
    property bool controllerEnabled: true

    QtObject {
        id: inputState

        property bool delayElapsed: false

        function closeFromController() {
            return root.controllerEnabled && root.close();
        }
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

    /*! Retries using the play-side indicated by \a key. */
    function retry(key) {
        if (typeof tryRetryAction === "function" && tryRetryAction(key)) {
            return true;
        }
        return globalRoot.retryResultForKey(key);
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
