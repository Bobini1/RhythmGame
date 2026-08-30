import QtQuick
import RhythmGameQml

// Common result-screen dismissal and retry input. Result presentation and
// optional button actions such as gauge cycling remain with the skin.
Item {
    id: root

    property var closeAction: null
    property var tryRetryAction: null
    property var tryHandleButtonAction: null
    property int inputDelayMillis: 500
    property bool delayElapsed: false
    readonly property bool acceptsInput: inputDelayMillis <= 0 || delayElapsed
    property bool confirmEnabled: true
    property bool controllerEnabled: true

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

    function retry(key) {
        if (typeof tryRetryAction === "function" && tryRetryAction(key)) {
            return true;
        }
        return globalRoot.retryResultForKey(key);
    }

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
