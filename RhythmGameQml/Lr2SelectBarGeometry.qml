pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml 1.0

QtObject {
    id: geometry

    required property var screenRoot
    required property var skinModel
    required property var selectContext

    readonly property var root: screenRoot

    readonly property var fastBarScrollStep: fastBarScrollStepFor(cachedBarBaseStates)
    readonly property bool fastBarScrollActive: fastBarScrollStep.valid
    readonly property real fastBarScrollX: fastBarScrollActive && selectContext
        ? -fastBarScrollStep.dx * selectContext.visualStateObject.offset
        : 0
    readonly property real fastBarScrollY: fastBarScrollActive && selectContext
        ? -fastBarScrollStep.dy * selectContext.visualStateObject.offset
        : 0
    readonly property real selectedFastBarDrawX: 0
    readonly property real selectedFastBarDrawY: 0
    readonly property int selectedRow: skinModel.barCenter + selectContext.selectedOffset
    readonly property int clickStartRow: Math.max(0, skinModel.barAvailableStart - 3)
    readonly property int clickEndRow: Math.min(
        (skinModel.barRows ? skinModel.barRows.length : 0) - 1,
        skinModel.barAvailableEnd + 3)
    readonly property var cachedBarBaseStates: barBaseStateCache.baseStates

    property Lr2BarBaseStateCache barBaseStateCache: Lr2BarBaseStateCache {
        barRows: geometry.skinModel.barRows
        selectedRow: geometry.selectedRow
        skinTime: geometry.root.barSkinTime
        timers: geometry.root.barTimers
        activeOptions: geometry.root.barActiveOptions
    }

    property Lr2BarPositionCache barPositionCache: Lr2BarPositionCache {
        baseStates: geometry.cachedBarBaseStates
        slotOffset: geometry.selectContext ? geometry.selectContext.visibleBarSlotOffset : 0
        slotCount: geometry.skinModel.barRows ? geometry.skinModel.barRows.length : 0
        visualState: geometry.fastBarScrollActive || !geometry.selectContext
            ? null
            : geometry.selectContext.visualStateObject
    }

    function stateNumber(state, name, fallback) {
        if (!state) {
            return fallback;
        }
        let value = state[name];
        return value === undefined || value === null ? fallback : Number(value);
    }

    function stateHasPosition(state) {
        return !!state && (state.x !== undefined || state.y !== undefined);
    }

    function sameStateNumber(lhs, rhs, name, fallback) {
        return Math.abs(stateNumber(lhs, name, fallback) - stateNumber(rhs, name, fallback)) <= 0.001;
    }

    function fastScrollStateCompatible(lhs, rhs) {
        return sameStateNumber(lhs, rhs, "w", 0)
            && sameStateNumber(lhs, rhs, "h", 0)
            && sameStateNumber(lhs, rhs, "a", 255)
            && sameStateNumber(lhs, rhs, "r", 255)
            && sameStateNumber(lhs, rhs, "g", 255)
            && sameStateNumber(lhs, rhs, "b", 255)
            && sameStateNumber(lhs, rhs, "angle", 0)
            && sameStateNumber(lhs, rhs, "center", 0)
            && sameStateNumber(lhs, rhs, "blend", 0)
            && sameStateNumber(lhs, rhs, "filter", 0)
            && sameStateNumber(lhs, rhs, "op4", 0);
    }

    function fastBarScrollStepFor(states) {
        if (!states || states.length < 2) {
            return { valid: false, dx: 0, dy: 0 };
        }

        let dx = 0;
        let dy = 0;
        let haveStep = false;
        for (let row = 1; row < states.length; ++row) {
            let previous = states[row - 1];
            let current = states[row];
            if (!stateHasPosition(previous) || !stateHasPosition(current)) {
                continue;
            }

            let nextDx = stateNumber(current, "x", 0) - stateNumber(previous, "x", 0);
            let nextDy = stateNumber(current, "y", 0) - stateNumber(previous, "y", 0);
            if (!haveStep) {
                dx = nextDx;
                dy = nextDy;
                haveStep = true;
            } else if (Math.abs(dx - nextDx) > 0.001 || Math.abs(dy - nextDy) > 0.001) {
                return { valid: false, dx: 0, dy: 0 };
            }

            if (row !== selectedRow && row - 1 !== selectedRow
                    && !fastScrollStateCompatible(previous, current)) {
                return { valid: false, dx: 0, dy: 0 };
            }
        }

        return { valid: haveStep && (Math.abs(dx) > 0.001 || Math.abs(dy) > 0.001), dx: dx, dy: dy };
    }

    function selectedBarRow() {
        return geometry.selectedRow;
    }

    function barClickStart() {
        return geometry.clickStartRow;
    }

    function barClickEnd() {
        return geometry.clickEndRow;
    }

    function barRowCanClick(row) {
        if (skinModel.barAvailableEnd < skinModel.barAvailableStart) {
            return false;
        }
        return row >= geometry.clickStartRow && row <= geometry.clickEndRow;
    }

    function barRowScrollDelta(row) {
        let first = skinModel.barAvailableStart;
        let last = skinModel.barAvailableEnd;
        if (row < first) {
            return -2 * (first - row);
        }
        if (row > last) {
            return 2 * (row - last);
        }
        return 0;
    }

    function handleBarRowClick(row, mouse) {
        root.clearSelectSearchFocus();
        if (mouse.button === Qt.RightButton) {
            root.selectGoBack();
            return;
        }

        let delta = geometry.barRowScrollDelta(row);
        if (delta !== 0) {
            if (!selectContext.visualMoveActive) {
                selectContext.scrollBy(delta, selectContext.lr2ClickDuration);
            }
            return;
        }

        if (row === geometry.selectedRow) {
            root.selectGoForward();
            return;
        }

        selectContext.selectVisibleRow(row, skinModel.barCenter);
    }
}
