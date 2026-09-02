import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectInput
    \inqmlmodule RhythmGameQml
    \brief Maps standard keyboard and BMS-controller selection actions.

    The component extends \l StandardSelectNavigation with activation, replay,
    autoplay, sorting, and back-button mappings. Skins that only want movement
    policy can instantiate \l StandardSelectNavigation directly.

    Controller mapping:

    \table
        \header
            \li Input
            \li Operation
        \row
            \li 1/3 keys
            \li Activate the focused item
        \row
            \li 5 key
            \li Autoplay
        \row
            \li 7 key
            \li Replay
        \row
            \li 6 key
            \li Cycle the selected replay
        \row
            \li 2/4 keys at the root
            \li Call \l tryCycleSortModeAction with -1/+1; if it does not
                consume the input, go back
        \row
            \li 2/4 keys below the root
            \li Go back
        \row
            \li Scratch directions
            \li Emit inherited \l moveRequested signals
    \endtable

    Autoplay uses \l StandardSelectState::openPlayable when its pre-handler
    does not consume the input. Replay requires \l tryReplayAction because the
    choice of replay score is deliberately outside selection input; without a
    successful handler, replay input does nothing.

    The numeric key descriptions apply to both players. Keyboard Up and Down
    are not global shortcuts: the focused visual item must forward its
    pressed/released events through \l handleUpPressed,
    \l handleDownPressed, and \l handleReleased. Assign \l selectState for the
    built-in activate/back behavior, or provide \l activateAction,
    \l goBackAction, and \l atTopLevel for custom state.
*/
StandardSelectNavigation {
    id: root

    /*! Standard selection state that receives activation and history actions. */
    property StandardSelectState selectState: null
    /*! Optional \c activateAction() replacement for focused-item activation. */
    property var activateAction: null
    /*! Optional \c goBackAction() replacement for leaving the current entry. */
    property var goBackAction: null
    /*! Whether sort-key handling is currently at the top selection level. */
    property bool atTopLevel: selectState
        ? selectState.historyStack.length <= 1 : false
    /*!
        Optional \c tryAutoplayAction() pre-handler. True consumes the input;
        false continues with standard autoplay.
    */
    property var tryAutoplayAction: null
    /*!
        Optional \c tryReplayAction() handler. Replay input is ignored when it
        is absent or returns false because selecting a replay requires skin- or
        application-owned replay state.
    */
    property var tryReplayAction: null
    /*! Optional \c cycleReplayTypeAction() replacement. */
    property var cycleReplayTypeAction: null
    /*! Optional \c tryCycleSortModeAction(delta) pre-handler. True consumes. */
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
        if (!root.enabled || typeof tryReplayAction !== "function") {
            return false;
        }
        return !!tryReplayAction();
    }

    /*! Activates autoplay for the focused chart. */
    function activateAutoplay() {
        if (!root.enabled) {
            return false;
        }
        if (typeof tryAutoplayAction === "function" && tryAutoplayAction()) {
            return true;
        }
        if (selectState && selectState.openPlayable(
                selectState.focusedItem, true, false, null)) {
            return true;
        }
        return false;
    }

    /*! Handles an Up key \a event. */
    function handleUpPressed(event) {
        if (!root.enabled) {
            return false;
        }
        event.accepted = true;
        if (!event.isAutoRepeat) {
            root.pressDirection(Qt.Key_Up);
        }
        root.navigate(event.isAutoRepeat, null, true, Qt.Key_Up);
        return true;
    }

    /*! Handles a Down key \a event. */
    function handleDownPressed(event) {
        if (!root.enabled) {
            return false;
        }
        event.accepted = true;
        if (!event.isAutoRepeat) {
            root.pressDirection(Qt.Key_Down);
        }
        root.navigate(event.isAutoRepeat, null, false, Qt.Key_Down);
        return true;
    }

    /*! Handles a keyboard direction-release \a event. */
    function handleReleased(event) {
        if (!root.enabled) {
            return false;
        }
        if (event.key === Qt.Key_Up) {
            if (!event.isAutoRepeat) {
                root.releaseDirection(Qt.Key_Up, true);
            }
            event.accepted = true;
            return true;
        } else if (event.key === Qt.Key_Down) {
            if (!event.isAutoRepeat) {
                root.releaseDirection(Qt.Key_Down, false);
            }
            event.accepted = true;
            return true;
        }
        return false;
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
