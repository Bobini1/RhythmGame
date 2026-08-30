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
    /*! Whether the input delay has elapsed. */
    property bool delayElapsed: false
    /*! Whether result input currently accepts actions. */
    readonly property bool acceptsInput: inputDelayMillis <= 0 || delayElapsed
    /*! Whether keyboard and pointer confirmation is active. */
    property bool confirmEnabled: true
    /*! Whether BMS-controller input is active. */
    property bool controllerEnabled: true

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
            && !root.delayElapsed
        repeat: false
        onTriggered: root.delayElapsed = true
    }

    onEnabledChanged: {
        if (!enabled) {
            delayElapsed = false;
        }
    }

    onInputDelayMillisChanged: delayElapsed = false

    Shortcut {
        enabled: root.enabled && root.acceptsInput
        sequence: "Esc"
        onActivated: root.close()
    }

    Shortcut {
        enabled: root.enabled && root.acceptsInput && root.confirmEnabled
        sequence: "Return"
        onActivated: root.close()
    }

    Input.onButtonPressed: key => root.handleButton(key)
    Input.onStart1Pressed: root.close()
    Input.onStart2Pressed: root.close()
}
