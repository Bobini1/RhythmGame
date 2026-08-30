import QtQuick
import RhythmGameQml

// Standard keyboard and BMS-controller actions for a selection screen. The
// directional policy is exposed separately so skins can use it without the
// standard activation, replay, sorting or back-button mappings.
Item {
    id: root

    property var selectState: null
    property var tryAutoplayAction: null
    property var tryReplayAction: null
    property var cycleReplayTypeAction: null
    property var tryCycleSortModeAction: null
    property alias navigation: navigation

    signal moveRequested(int steps, bool repeated, bool analog)

    StandardSelectNavigation {
        id: navigation

        enabled: root.enabled
        onMoveRequested: (steps, repeated, analog) => {
            root.moveRequested(steps, repeated, analog);
        }
    }

    function navigate(tickNumber, tickType, up, key) {
        navigation.navigate(tickNumber, tickType, up, key);
    }

    function pressDirection(key) {
        navigation.pressDirection(key);
    }

    function releaseDirection(key, up) {
        navigation.releaseDirection(key, up);
    }

    function activate() {
        if (!root.enabled || !selectState) {
            return false;
        }
        selectState.goForward(selectState.focusedItem);
        return true;
    }

    function activateReplay() {
        if (typeof tryReplayAction !== "function"
                || !tryReplayAction(Qt.LeftButton)) {
            activate();
        }
    }

    function activateAutoplay() {
        if (typeof tryAutoplayAction !== "function" || !tryAutoplayAction()) {
            activate();
        }
    }

    function handleUpPressed(event) {
        event.accepted = true;
        if (!event.isAutoRepeat) {
            pressDirection(Qt.Key_Up);
        }
        navigate(event.isAutoRepeat, null, true, Qt.Key_Up);
    }

    function handleDownPressed(event) {
        event.accepted = true;
        if (!event.isAutoRepeat) {
            pressDirection(Qt.Key_Down);
        }
        navigate(event.isAutoRepeat, null, false, Qt.Key_Down);
    }

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
