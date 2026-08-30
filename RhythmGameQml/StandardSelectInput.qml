import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectInput
    \inqmlmodule RhythmGameQml
    \brief Maps standard keyboard and BMS-controller selection actions.

    Directional policy is exposed through \l navigation so skins can use it
    without the standard activation, replay, sorting, or back-button mappings.
*/
Item {
    id: root

    /*! Selection state that receives activation and history actions. */
    property var selectState: null
    /*! Optional autoplay pre-handler. True consumes; false or undefined continues. */
    property var tryAutoplayAction: null
    /*! Optional replay pre-handler. True consumes; false or undefined continues. */
    property var tryReplayAction: null
    /*! Optional replacement for cycling the selected replay type. */
    property var cycleReplayTypeAction: null
    /*! Optional sort pre-handler. True consumes; false or undefined continues. */
    property var tryCycleSortModeAction: null
    /*! Directional policy and timing component. */
    property alias navigation: navigation

    /*!
        Requests relative focus movement by \a steps. \a repeated identifies
        held input and \a analog identifies analog-scratch input.
    */
    signal moveRequested(int steps, bool repeated, bool analog)

    StandardSelectNavigation {
        id: navigation

        enabled: root.enabled
        onMoveRequested: (steps, repeated, analog) => {
            root.moveRequested(steps, repeated, analog);
        }
    }

    /*!
        Forwards \a tickNumber and \a tickType for directional \a key to
        \l navigation. \a up selects the movement direction.
    */
    function navigate(tickNumber, tickType, up, key) {
        navigation.navigate(tickNumber, tickType, up, key);
    }

    /*! Records a controller direction press for \a key. */
    function pressDirection(key) {
        navigation.pressDirection(key);
    }

    /*! Records a release for directional \a key; \a up identifies its direction. */
    function releaseDirection(key, up) {
        navigation.releaseDirection(key, up);
    }

    /*! Activates the focused item. */
    function activate() {
        if (!root.enabled || !selectState) {
            return false;
        }
        selectState.goForward(selectState.focusedItem);
        return true;
    }

    /*! Activates replay for the focused chart. */
    function activateReplay() {
        if (typeof tryReplayAction !== "function"
                || !tryReplayAction(Qt.LeftButton)) {
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
            pressDirection(Qt.Key_Up);
        }
        navigate(event.isAutoRepeat, null, true, Qt.Key_Up);
    }

    /*! Handles a Down key \a event. */
    function handleDownPressed(event) {
        event.accepted = true;
        if (!event.isAutoRepeat) {
            pressDirection(Qt.Key_Down);
        }
        navigate(event.isAutoRepeat, null, false, Qt.Key_Down);
    }

    /*! Handles a keyboard direction-release \a event. */
    function handleReleased(event) {
        if (event.key === Qt.Key_Up) {
            if (!event.isAutoRepeat) {
                releaseDirection(Qt.Key_Up, true);
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Down) {
            if (!event.isAutoRepeat) {
                releaseDirection(Qt.Key_Down, false);
            }
            event.accepted = true;
        }
    }

    /*! Handles top-level sort-mode input for \a key. */
    function handleTopLevelSortKey(key) {
        if (!selectState || selectState.historyStack.length > 1
                || typeof tryCycleSortModeAction !== "function") {
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

    Input.onCol1sDownTicked: (number, type) => navigate(number, type, false, BmsKey.Col1sDown)
    Input.onCol1sUpTicked: (number, type) => navigate(number, type, true, BmsKey.Col1sUp)
    Input.onCol2sDownTicked: (number, type) => navigate(number, type, false, BmsKey.Col2sDown)
    Input.onCol2sUpTicked: (number, type) => navigate(number, type, true, BmsKey.Col2sUp)
    Input.onCol1sDownPressed: pressDirection(BmsKey.Col1sDown)
    Input.onCol1sUpPressed: pressDirection(BmsKey.Col1sUp)
    Input.onCol2sDownPressed: pressDirection(BmsKey.Col2sDown)
    Input.onCol2sUpPressed: pressDirection(BmsKey.Col2sUp)
    Input.onCol1sDownReleased: releaseDirection(BmsKey.Col1sDown, false)
    Input.onCol1sUpReleased: releaseDirection(BmsKey.Col1sUp, true)
    Input.onCol2sDownReleased: releaseDirection(BmsKey.Col2sDown, false)
    Input.onCol2sUpReleased: releaseDirection(BmsKey.Col2sUp, true)
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
            if (selectState) {
                selectState.goBack();
            }
        }
    }
}
