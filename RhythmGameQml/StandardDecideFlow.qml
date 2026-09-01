import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardDecideFlow
    \inqmlmodule RhythmGameQml
    \brief Provides standard decide-screen lifetime and input behavior.

    The surrounding skin owns every visual. This component owns accepting,
    cancelling, timeouts, and runner destruction, with override actions for
    skins that need different transitions.
*/
Item {
    id: root

    anchors.fill: parent

    /*! Chart runner being accepted or cancelled. */
    property var chart: null
    /*! Optional replacement for accepting the chart. */
    property var startAction: null
    /*! Optional replacement for cancelling the chart. */
    property var cancelAction: null
    /*! Optional replacement for returning after standard gameplay closes. */
    property var returnAfterGameplayAction: null
    /*! Automatic acceptance timeout in milliseconds; zero disables it. */
    property int timeoutMillis: 5000
    /*! Whether keyboard input is active. */
    property bool keyboardEnabled: true
    /*! Whether BMS-controller input is active. */
    property bool controllerEnabled: true
    /*! Whether pointer input is active. */
    property bool pointerEnabled: true
    /*! Whether destruction of this component destroys \l chart. */
    property bool destroyChartOnDestruction: true
    QtObject {
        id: flowState

        property bool returnAfterGameplay: false
        property bool transitionRequested: false

        function run(action, defaultAction, returnAfterDefault = false) {
            if (flowState.transitionRequested || !root.enabled) {
                return false;
            }
            flowState.transitionRequested = true;
            if (typeof action === "function") {
                action();
            } else {
                flowState.returnAfterGameplay = returnAfterDefault;
                defaultAction();
            }
            return true;
        }

        function isStartSelectCombo(key) {
            return (key === BmsKey.Start1 && Input.select1)
                || (key === BmsKey.Select1 && Input.start1)
                || (key === BmsKey.Start2 && Input.select2)
                || (key === BmsKey.Select2 && Input.start2);
        }
    }

    /*! Accepts the chart and begins gameplay. */
    function start() {
        if (!chart) {
            return false;
        }
        return flowState.run(startAction,
                             () => globalRoot.openGameplay(chart), true);
    }

    /*! Cancels the chart and returns to the previous screen. */
    function cancel() {
        return flowState.run(cancelAction,
                             () => globalRoot.returnToPreviousScreen());
    }

    /*! Returns from the decide screen after standard gameplay closes. */
    function returnAfterGameplay() {
        if (typeof returnAfterGameplayAction === "function") {
            returnAfterGameplayAction();
        } else {
            globalRoot.returnToPreviousScreen();
        }
    }

    onEnabledChanged: {
        if (enabled && flowState.returnAfterGameplay) {
            flowState.returnAfterGameplay = false;
            Qt.callLater(root.returnAfterGameplay);
        }
    }

    onChartChanged: {
        flowState.transitionRequested = false;
        flowState.returnAfterGameplay = false;
    }

    Timer {
        interval: Math.max(1, root.timeoutMillis)
        running: root.enabled && root.timeoutMillis > 0
            && !flowState.transitionRequested
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
        if (flowState.isStartSelectCombo(key)) {
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
