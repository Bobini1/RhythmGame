import QtQuick
import RhythmGameQml

// Standard decide-screen lifetime and input. The surrounding skin owns every
// visual; this component owns accepting, cancelling and runner destruction.
Item {
    id: root

    property var chart: null
    property var startAction: null
    property var cancelAction: null
    property int timeoutMillis: 5000
    property bool keyboardEnabled: true
    property bool controllerEnabled: true
    property bool pointerEnabled: true
    property bool destroyChartOnDestruction: true
    property bool transitionRequested: false

    function run(action, defaultAction) {
        if (transitionRequested || !enabled) {
            return false;
        }
        transitionRequested = true;
        let result = typeof action === "function"
            ? action()
            : defaultAction();
        if (result === false) {
            transitionRequested = false;
            return false;
        }
        return true;
    }

    function start() {
        if (!chart) {
            return false;
        }
        return run(startAction, () => globalRoot.openGameplay(chart));
    }

    function cancel() {
        return run(cancelAction,
                   () => globalRoot.returnToPreviousScreen());
    }

    function isStartSelectCombo(key) {
        return (key === BmsKey.Start1 && Input.select1)
            || (key === BmsKey.Select1 && Input.start1)
            || (key === BmsKey.Start2 && Input.select2)
            || (key === BmsKey.Select2 && Input.start2);
    }

    onEnabledChanged: {
        if (enabled && transitionRequested) {
            Qt.callLater(globalRoot.returnToPreviousScreen);
        }
    }

    Timer {
        interval: Math.max(1, root.timeoutMillis)
        running: root.enabled && root.timeoutMillis > 0
            && !root.transitionRequested
        repeat: false
        onTriggered: root.start()
    }

    Shortcut {
        enabled: root.enabled && root.keyboardEnabled
        sequence: "Esc"
        onActivated: root.cancel()
    }

    Shortcut {
        enabled: root.enabled && root.keyboardEnabled
        sequence: "Return"
        onActivated: root.start()
    }

    Shortcut {
        enabled: root.enabled && root.keyboardEnabled
        sequence: "Enter"
        onActivated: root.start()
    }

    Input.onButtonPressed: key => {
        if (!root.enabled || !root.controllerEnabled) {
            return;
        }
        if (root.isStartSelectCombo(key)) {
            root.cancel();
        } else if (StandardInputKeys.isPlayKey(key)) {
            root.start();
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        enabled: root.enabled && root.pointerEnabled

        onPressed: mouse => {
            mouse.accepted = true;
            if (mouse.button === Qt.LeftButton) {
                root.start();
            } else if (mouse.button === Qt.RightButton) {
                root.cancel();
            }
        }
    }

    Component.onDestruction: {
        if (destroyChartOnDestruction && chart
                && typeof chart.destroy === "function") {
            chart.destroy();
        }
    }
}
