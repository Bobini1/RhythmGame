import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardDecideFlow
    \inqmlmodule RhythmGameQml
    \brief Provides standard decide-screen lifetime and input behavior.

    The surrounding skin owns every visual. This component owns accepting,
    cancelling and timeouts, with override actions for
    skins that need different transitions.

    Default input mapping:

    \table
        \header
            \li Input
            \li Operation
        \row
            \li Return, Enter, left click, or any play key
            \li \l start
        \row
            \li Escape, right click, or Start+Select
            \li \l cancel
        \row
            \li \l timeoutMillis expires
            \li \l start
    \endtable

    Only the first start/cancel request is accepted for a chart. The built-in
    start replaces decide with gameplay. If a built-in transition cannot create
    or remove a screen, the request is released so the skin can try again. A replacement
    \l startAction or \l cancelAction owns its complete transition; it is not
    followed by the built-in action.

    The content frame owns runner lifetime: cancelling destroys the runner with
    decide; starting transfers it to gameplay. This component does not destroy
    the runner itself, including when a replacement action is supplied.
*/
Item {
    id: root

    anchors.fill: parent

    /*! Chart runner being accepted or cancelled. */
    property var chart: null
    /*! Optional \c startAction() replacement that owns the start transition. */
    property var startAction: null
    /*! Optional \c cancelAction() replacement that owns the cancel transition. */
    property var cancelAction: null
    /*! Automatic acceptance timeout in milliseconds; zero disables it. */
    property int timeoutMillis: 5000
    /*! Whether keyboard input is active. */
    property bool keyboardEnabled: true
    /*! Whether BMS-controller input is active. */
    property bool controllerEnabled: true
    /*! Whether pointer input is active. */
    property bool pointerEnabled: true
    QtObject {
        id: flowState

        property bool transitionRequested: false

        function run(action, defaultAction) {
            if (flowState.transitionRequested || !root.enabled) {
                return false;
            }
            flowState.transitionRequested = true;
            if (typeof action === "function") {
                action();
                return true;
            }
            let result = defaultAction();
            if (!result) {
                flowState.transitionRequested = false;
                return false;
            }
            return true;
        }

        function isStartSelectCombo(key) {
            return (key === BmsKey.Start1 && root.Input.select1)
                || (key === BmsKey.Select1 && root.Input.start1)
                || (key === BmsKey.Start2 && root.Input.select2)
                || (key === BmsKey.Select2 && root.Input.start2);
        }
    }

    /*! Accepts the chart and begins gameplay. */
    function start() {
        if (!chart) {
            return false;
        }
        return flowState.run(startAction,
                             () => globalRoot.openGameplay(chart));
    }

    /*! Cancels the chart and returns to the previous screen. */
    function cancel() {
        return flowState.run(cancelAction,
                             () => globalRoot.returnToPreviousScreen());
    }

    onChartChanged: {
        flowState.transitionRequested = false;
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
}
