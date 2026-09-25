import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardDecideFlow
    \inqmlmodule RhythmGameQml
    \brief Starts or cancels play from a decide screen.

    Bind \l gameplay to the GameplayContext supplied to the screen. The same
    property supports a single chart or a course. Your theme draws the title and
    other details, while the component handles input and the timeout.

    The default controls are:
    \table
        \header \li Input \li Operation
        \row \li Return, Enter, left click or any play key \li \l start
        \row \li Escape, right click or Start+Select \li \l cancel
        \row \li The \l timeoutMillis delay expires \li \l start
    \endtable

    The pointer area fills the parent. Set \l pointerEnabled to false when your
    theme has its own buttons, and call \l start or \l cancel from those buttons.
    Only the first transition request is accepted. If the standard transition
    fails, the component accepts another attempt.

    Starting replaces decide with gameplay, passing along the same context.
    Cancelling returns to the previous screen. The game keeps the play objects
    alive for the screen that needs them and destroys them when that screen leaves.
    The theme must not destroy them itself.

    A replacement \l startAction or \l cancelAction handles the whole transition.
    The standard action does not run afterward. See the
    \l {../theme_tutorial_decide.html}{decide lesson} for an installable example.
*/
Item {
    id: root

    anchors.fill: parent

    /*! Supplies the GameplayContext received by the decide screen. */
    property GameplayContext gameplay: null
    /*! Calls \c startAction() to handle the whole start transition. */
    property var startAction: null
    /*! Calls \c cancelAction() to handle the whole cancel transition. */
    property var cancelAction: null
    /*!
        Sets the delay in milliseconds before starting automatically. Use zero to disable the
        timeout.
    */
    property int timeoutMillis: 5000
    /*! Controls whether keyboard input is active. */
    property bool keyboardEnabled: true
    /*! Controls whether BMS-controller input is active. */
    property bool controllerEnabled: true
    /*! Controls whether pointer input is active. */
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
        if (!root.gameplay) {
            return false;
        }
        return flowState.run(startAction,
                             () => globalRoot.replaceGameplay(root.gameplay));
    }

    /*! Cancels the chart and returns to the previous screen. */
    function cancel() {
        return flowState.run(cancelAction,
                             () => globalRoot.returnToPreviousScreen());
    }

    onGameplayChanged: {
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
