import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectInput
    \inqmlmodule RhythmGameQml
    \brief Maps standard keyboard and BMS-controller selection actions.

    The component extends \l StandardSelectNavigation with activation, replay,
    autoplay, sorting, and back-button mappings. Skins that only want movement
    policy can instantiate \l StandardSelectNavigation directly.
*/
StandardSelectNavigation {
    id: root

    /*! Standard selection state that receives activation and history actions. */
    property StandardSelectState selectState: null
    /*! Optional replacement for activating the focused item. */
    property var activateAction: null
    /*! Optional replacement for leaving the current selection entry. */
    property var goBackAction: null
    /*! Whether sort-key handling is currently at the top selection level. */
    property bool atTopLevel: selectState
        ? selectState.historyStack.length <= 1 : false
    /*! Optional autoplay pre-handler. True consumes; false or undefined continues. */
    property var tryAutoplayAction: null
    /*! Optional replay pre-handler. True consumes; false or undefined continues. */
    property var tryReplayAction: null
    /*! Optional replacement for cycling the selected replay type. */
    property var cycleReplayTypeAction: null
    /*! Optional sort pre-handler. True consumes; false or undefined continues. */
    property var tryCycleSortModeAction: null
    /*! Activates the focused item. */
    function activate() {
        if (!root.enabled) {
            return false;
        }
        if (typeof activateAction === "function") {
            activateAction();
            return true;
        }
        if (!selectState) {
            return false;
        }
        selectState.goForward(selectState.focusedItem);
        return true;
    }

    /*! Leaves the current selection entry. */
    function goBack() {
        if (!root.enabled) {
            return false;
        }
        if (typeof goBackAction === "function") {
            goBackAction();
            return true;
        }
        return selectState ? selectState.goBack() : false;
    }

    /*! Activates replay for the focused chart. */
    function activateReplay() {
        if (typeof tryReplayAction !== "function"
                || !tryReplayAction()) {
            activate();
        }
    }

    /*! Activates autoplay for the focused chart. */
    function activateAutoplay() {
        if (typeof tryAutoplayAction !== "function" || !tryAutoplayAction()) {
            activate();
        }
    }

    /*! Handles an Up key \a event. */
    function handleUpPressed(event) {
        event.accepted = true;
        if (!event.isAutoRepeat) {
            root.pressDirection(Qt.Key_Up);
        }
        root.navigate(event.isAutoRepeat, null, true, Qt.Key_Up);
    }

    /*! Handles a Down key \a event. */
    function handleDownPressed(event) {
        event.accepted = true;
        if (!event.isAutoRepeat) {
            root.pressDirection(Qt.Key_Down);
        }
        root.navigate(event.isAutoRepeat, null, false, Qt.Key_Down);
    }

    /*! Handles a keyboard direction-release \a event. */
    function handleReleased(event) {
        if (event.key === Qt.Key_Up) {
            if (!event.isAutoRepeat) {
                root.releaseDirection(Qt.Key_Up, true);
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Down) {
            if (!event.isAutoRepeat) {
                root.releaseDirection(Qt.Key_Down, false);
            }
            event.accepted = true;
        }
    }

    /*! Handles top-level sort-mode input for \a key. */
    function handleTopLevelSortKey(key) {
        if (!atTopLevel || typeof tryCycleSortModeAction !== "function") {
            return false;
        }
        if (key === BmsKey.Col12 || key === BmsKey.Col22) {
            return tryCycleSortModeAction(-1);
        }
        if (key === BmsKey.Col14 || key === BmsKey.Col24) {
            return tryCycleSortModeAction(1);
        }
        return false;
    }

    Input.onCol1sDownTicked: (number, type) => root.navigate(number, type, false, BmsKey.Col1sDown)
    Input.onCol1sUpTicked: (number, type) => root.navigate(number, type, true, BmsKey.Col1sUp)
    Input.onCol2sDownTicked: (number, type) => root.navigate(number, type, false, BmsKey.Col2sDown)
    Input.onCol2sUpTicked: (number, type) => root.navigate(number, type, true, BmsKey.Col2sUp)
    Input.onCol1sDownPressed: root.pressDirection(BmsKey.Col1sDown)
    Input.onCol1sUpPressed: root.pressDirection(BmsKey.Col1sUp)
    Input.onCol2sDownPressed: root.pressDirection(BmsKey.Col2sDown)
    Input.onCol2sUpPressed: root.pressDirection(BmsKey.Col2sUp)
    Input.onCol1sDownReleased: root.releaseDirection(BmsKey.Col1sDown, false)
    Input.onCol1sUpReleased: root.releaseDirection(BmsKey.Col1sUp, true)
    Input.onCol2sDownReleased: root.releaseDirection(BmsKey.Col2sDown, false)
    Input.onCol2sUpReleased: root.releaseDirection(BmsKey.Col2sUp, true)
    Input.onCol11Pressed: activate()
    Input.onCol17Pressed: activateReplay()
    Input.onCol13Pressed: activate()
    Input.onCol15Pressed: activateAutoplay()
    Input.onCol21Pressed: activate()
    Input.onCol27Pressed: activateReplay()
    Input.onCol23Pressed: activate()
    Input.onCol25Pressed: activateAutoplay()

    Input.onButtonPressed: key => {
        if (!root.enabled) {
            return;
        }
        if ((key === BmsKey.Col16 || key === BmsKey.Col26)
                && typeof cycleReplayTypeAction === "function") {
            cycleReplayTypeAction();
            return;
        }
        if (handleTopLevelSortKey(key)) {
            return;
        }
        if (key === BmsKey.Col12 || key === BmsKey.Col14
                || key === BmsKey.Col22 || key === BmsKey.Col24) {
            goBack();
        }
    }
}
