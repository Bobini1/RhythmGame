import QtQuick
import RhythmGameQml

// Common result-screen dismissal and retry input. Result presentation and
// optional button actions such as gauge cycling remain with the skin.
Item {
    id: root

    property var closeAction: null
    property var retryAction: null
    property var buttonAction: null
    property int inputDelayMillis: 500
    property bool acceptsInput: inputDelayMillis <= 0
    property bool confirmEnabled: true
    property bool controllerEnabled: true

    function close() {
        if (!enabled || !acceptsInput) {
            return false;
        }
        if (typeof closeAction === "function") {
            return closeAction();
        }
        return globalRoot.returnToPreviousScreen();
    }

    function retry(key) {
        if (typeof retryAction === "function") {
            return retryAction(key);
        }
        return globalRoot.retryResultForKey(key);
    }

    function handleButton(key) {
        if (!enabled || !acceptsInput || !controllerEnabled) {
            return false;
        }
        if (typeof buttonAction === "function" && buttonAction(key)) {
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
        running: root.inputDelayMillis > 0
        repeat: false
        onTriggered: root.acceptsInput = true
    }

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
