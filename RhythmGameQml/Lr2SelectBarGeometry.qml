pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

QtObject {
    id: geometry

    required property var screenRoot
    required property var skinModel
    required property var selectContext

    readonly property var root: screenRoot

    function computeBarBaseState(row, selectedRow) {
        let rows = skinModel.barRows;
        if (!rows || row < 0 || row >= rows.length) {
            return null;
        }
        let data = rows[row];
        let useOn = row === selectedRow && data.onDsts && data.onDsts.length > 0;
        let dstList = useOn ? data.onDsts : data.offDsts;
        if (!dstList || dstList.length === 0) {
            dstList = data.onDsts || [];
        }
        return Lr2Timeline.getCurrentState(dstList, root.barSkinTime, root.barTimers, root.barActiveOptions);
    }

    function interpolateBarState(fromState, toState, progress) {
        if (!fromState || !toState) {
            return fromState;
        }
        let inv = 1.0 - progress;
        return {
            x: fromState.x * inv + toState.x * progress,
            y: fromState.y * inv + toState.y * progress,
            w: fromState.w * inv + toState.w * progress,
            h: fromState.h * inv + toState.h * progress,
            a: fromState.a * inv + toState.a * progress,
            r: fromState.r * inv + toState.r * progress,
            g: fromState.g * inv + toState.g * progress,
            b: fromState.b * inv + toState.b * progress,
            blend: fromState.blend,
            filter: fromState.filter,
            angle: fromState.angle * inv + toState.angle * progress,
            center: fromState.center,
            sortId: (fromState.sortId || 0) * inv + (toState.sortId || 0) * progress
        };
    }

    readonly property bool fastBarScrollActive: false
    readonly property real fastBarScrollX: 0
    readonly property real fastBarScrollY: 0
    readonly property real selectedFastBarDrawX: 0
    readonly property real selectedFastBarDrawY: 0

    readonly property var cachedBarBaseStates: {
        let rows = skinModel.barRows || [];
        let selected = geometry.selectedBarRow();
        let result = [];
        for (let row = 0; row < rows.length; ++row) {
            result.push(geometry.computeBarBaseState(row, selected));
        }
        return result;
    }

    readonly property var cachedBarDrawXs: geometry.barDrawAxisValues("x")
    readonly property var cachedBarDrawYs: geometry.barDrawAxisValues("y")

    function barDrawAxisValues(axis) {
        let baseStates = geometry.cachedBarBaseStates || [];
        let offset = selectContext.scrollOffset || 0;
        let inv = 1.0 - offset;
        let result = [];
        for (let row = 0; row < baseStates.length; ++row) {
            let fromState = baseStates[row];
            let toState = row > 0 ? baseStates[row - 1] : null;
            result.push(fromState && toState && offset > 0.001
                ? fromState[axis] * inv + toState[axis] * offset
                : (fromState ? fromState[axis] : 0));
        }
        return result;
    }

    readonly property var cachedBarRowCells: {
        let revision = selectContext.barTextCellsRevision;
        let cells = selectContext.visibleBarTextCells || [];
        let result = [];
        for (let i = 0; i < cells.length; ++i) {
            let cell = cells[i];
            if (cell && cell.row >= 0) {
                result[cell.row] = cell;
            }
        }
        return result;
    }

    function barBaseState(row) {
        return geometry.cachedBarBaseStates && row >= 0 && row < geometry.cachedBarBaseStates.length
            ? geometry.cachedBarBaseStates[row]
            : null;
    }

    function barDrawState(row) {
        let fromState = geometry.barBaseState(row);
        let offset = selectContext.scrollOffset;
        if (!fromState || offset <= 0.001) {
            return fromState;
        }

        let toState = row > 0 ? geometry.barBaseState(row - 1) : null;
        return fromState && toState
            ? geometry.interpolateBarState(fromState, toState, offset)
            : fromState;
    }

    function barSpriteScrollOffset(src) {
        return selectContext.scrollOffset || 0;
    }

    function selectedBarRow() {
        return skinModel.barCenter + selectContext.selectedOffset;
    }

    function barClickStart() {
        return Math.max(0, skinModel.barAvailableStart - 3);
    }

    function barClickEnd() {
        let rows = skinModel.barRows ? skinModel.barRows.length : 0;
        return Math.min(rows - 1, skinModel.barAvailableEnd + 3);
    }

    function barRowCanClick(row) {
        if (skinModel.barAvailableEnd < skinModel.barAvailableStart) {
            return false;
        }
        return row >= geometry.barClickStart() && row <= geometry.barClickEnd();
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

        if (row === geometry.selectedBarRow()) {
            root.selectGoForward();
            return;
        }

        selectContext.selectVisibleRow(row, skinModel.barCenter);
    }
}
