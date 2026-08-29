import QtQuick
import RhythmGameQml

// Standard keyboard and BMS-controller navigation for a selection view.
// The target owns presentation details such as its current item and scrolling.
Item {
    id: root

    required property var target
    property var selectState: null
    property var autoplayAction: null
    property var replayAction: null
    property var cycleReplayTypeAction: null
    property var cycleSortModeAction: null
    property var lastKey: []

    function navigate(number, type, up, key) {
        if (!root.enabled) {
            return;
        }
        if (type === InputTranslator.AnalogScratchTick) {
            target.queueAnalogScratchTick(up);
            return;
        }
        if (lastKey[lastKey.length - 1] !== key) {
            return;
        }
        if (type === InputTranslator.ButtonTick
                || type === InputTranslator.ClassicScratchTick) {
            target.handleScratchRepeat(up, number);
            return;
        }
        let func = up ? target.decrementViewIndex : target.incrementViewIndex;
        func(!!number);
    }

    function pressDirection(key) {
        if (!root.enabled) {
            return;
        }
        lastKey.push(key);
    }

    function releaseDirection(key, up) {
        lastKey = lastKey.filter(pressedKey => pressedKey !== key);
        target.releaseScratchRepeat(up);
    }

    function activate() {
        if (!root.enabled) {
            return false;
        }
        if (!selectState) {
            return false;
        }
        selectState.goForward(selectState.focusedItem);
        return true;
    }

    function activateReplay() {
        if (typeof replayAction !== "function" || !replayAction(Qt.LeftButton)) {
            activate();
        }
    }

    function activateAutoplay() {
        if (typeof autoplayAction !== "function" || !autoplayAction()) {
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
                lastKey = lastKey.filter(key => key !== Qt.Key_Up);
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Down) {
            if (!event.isAutoRepeat) {
                lastKey = lastKey.filter(key => key !== Qt.Key_Down);
            }
            event.accepted = true;
        }
    }

    function handleTopLevelSortKey(key) {
        if (!selectState || selectState.historyStack.length > 1
                || typeof cycleSortModeAction !== "function") {
            return false;
        }
        if (key === BmsKey.Col12 || key === BmsKey.Col22) {
            return cycleSortModeAction(-1);
        }
        if (key === BmsKey.Col14 || key === BmsKey.Col24) {
            return cycleSortModeAction(1);
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

    onEnabledChanged: {
        if (!enabled && target) {
            lastKey = [];
            target.releaseScratchRepeat(false);
            target.releaseScratchRepeat(true);
        }
    }
}
