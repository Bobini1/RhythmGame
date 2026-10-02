pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectInput
    \inqmlmodule RhythmGameQml
    \brief Maps bound selection actions and forwards movement requests.

    The component extends StandardSelectNavigation with activation, autoplay,
    replay, sorting and leaving selection. Use
    StandardSelectNavigation alone if you
    only need movement. StandardSelectController already includes both.

    The bound controller keys work on either player side:
    \table
        \header \li Input \li Action
        \row \li Keys 1 and 3 \li Activate the focused item.
        \row \li Key 5 \li Start autoplay.
        \row \li Key 7 \li Open the selected replay.
        \row \li Key 6 \li Cycle newest, best score, best clear and best combo.
        \row \li Keys 2 and 4 at the root
             \li Cycle the sort mode backward or forward.
        \row \li Keys 2 and 4 below the root \li Go back.
        \row \li Scratch directions \li Emit \l moveRequested.
        \row \li Escape \li Leave selection or the current Arena room.
    \endtable

    Autoplay calls StandardSelectState::openAutoplay unless \l tryAutoplayAction
    handles it first. On folders, the autoplay key enters the folder.
    Replay calls StandardSelectState::openReplay unless
    \l tryReplayAction handles it first. Key 6 calls
    StandardSelectState::cycleReplayType unless \l cycleReplayTypeAction is supplied.

    Keyboard arrows must come from the focused visual item. Forward presses and
    releases to \l handleUpPressed, \l handleDownPressed and \l handleReleased.
    Assign \l selectState for standard activation and back behavior. For your
    own browsing state, supply \l activateAction, \l goBackAction and \l atTopLevel.
    Assign \l actions to a StandardSelectActions instance for Escape, or
    disable it with \l exitEnabled. StandardArenaSelectOverlay supplies Arena
    ready input.
*/
StandardSelectNavigation {
    id: root

    /*! Standard selection state that receives activation and history actions. */
    property StandardSelectState selectState: null
    /*! Shared selector actions. Defaults to the supplied state's actions. */
    property StandardSelectActions actions: selectState ? selectState.actions : null
    /*! Controls the Escape shortcut for leaving selection or its Arena room. */
    property bool exitEnabled: true
    /*! Calls \c activateAction() in place of focused-item activation. */
    property var activateAction: null
    /*! Calls \c goBackAction() in place of leaving the current entry. */
    property var goBackAction: null
    /*! Controls whether sort-key handling is currently at the top selection level. */
    property bool atTopLevel: selectState
        ? selectState.historyStack.length <= 1 : false
    /*!
        Called as \c tryAutoplayAction(). Return true to handle the request, or false to
        continue with the standard action.
    */
    property var tryAutoplayAction: null
    /*!
        Called as \c tryReplayAction() to choose and open a replay. Return true when handled.
        Return false to continue with StandardSelectState::openReplay.
    */
    property var tryReplayAction: null
    /*! Calls \c cycleReplayTypeAction() instead of StandardSelectState::cycleReplayType. */
    property var cycleReplayTypeAction: null
    /*!
        Called as \c tryCycleSortModeAction(delta). Return true to handle the request, or
        false to continue with the standard action.
    */
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
        if (!root.enabled) return false;
        if (typeof root.tryReplayAction === "function" && root.tryReplayAction()) {
            return true;
        }
        return root.selectState ? root.selectState.openReplay() : false;
    }

    /*! Activates autoplay for the focused chart. */
    function activateAutoplay() {
        if (!root.enabled) {
            return false;
        }
        if (typeof tryAutoplayAction === "function" && tryAutoplayAction()) {
            return true;
        }
        if (!selectState) return false;
        if (selectState.openAutoplay()) return true;
        const item = selectState.focusedItem;
        return !(item instanceof ChartData) && !(item instanceof course)
            && selectState.goForward(item);
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
        if (!root.enabled || !root.atTopLevel) return false;
        const delta = key === BmsKey.Col12 || key === BmsKey.Col22 ? -1
            : key === BmsKey.Col14 || key === BmsKey.Col24 ? 1 : 0;
        if (!delta) return false;
        if (typeof root.tryCycleSortModeAction === "function"
                && root.tryCycleSortModeAction(delta)) return true;
        return root.selectState ? root.selectState.cycleSortMode(delta) : false;
    }

    Shortcut {
        sequence: "Esc"
        autoRepeat: false
        enabled: root.enabled && root.exitEnabled && !!root.actions
        onActivated: root.actions.exit()
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
        if (key === BmsKey.Col16 || key === BmsKey.Col26) {
            if (typeof root.cycleReplayTypeAction === "function") {
                root.cycleReplayTypeAction();
            } else if (root.selectState) {
                root.selectState.cycleReplayType();
            }
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
