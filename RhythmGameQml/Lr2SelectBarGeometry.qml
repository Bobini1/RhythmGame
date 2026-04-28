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
    readonly property int selectedRow: skinModel.barCenter + selectContext.selectedOffset
    readonly property int clickStartRow: Math.max(0, skinModel.barAvailableStart - 3)
    readonly property int clickEndRow: Math.min(
        (skinModel.barRows ? skinModel.barRows.length : 0) - 1,
        skinModel.barAvailableEnd + 3)
    readonly property real scrollOffset: selectContext.scrollOffset || 0

    readonly property var cachedBarBaseStates: {
        let rows = skinModel.barRows || [];
        let result = [];
        for (let row = 0; row < rows.length; ++row) {
            result.push(geometry.computeBarBaseState(row, geometry.selectedRow));
        }
        return result;
    }

    readonly property var cachedBarDrawCoordinates: {
        let baseStates = geometry.cachedBarBaseStates || [];
        let xs = [];
        let ys = [];
        let progress = geometry.scrollOffset || 0;
        let shouldInterpolate = progress > 0.001;
        let inv = 1.0 - progress;
        for (let row = 0; row < baseStates.length; ++row) {
            let fromState = baseStates[row];
            let toState = row > 0 ? baseStates[row - 1] : null;
            if (fromState && toState && shouldInterpolate) {
                xs.push(fromState.x * inv + toState.x * progress);
                ys.push(fromState.y * inv + toState.y * progress);
            } else {
                xs.push(fromState ? fromState.x : 0);
                ys.push(fromState ? fromState.y : 0);
            }
        }
        return { xs: xs, ys: ys };
    }

    readonly property var cachedBarDrawXs: geometry.cachedBarDrawCoordinates.xs
    readonly property var cachedBarDrawYs: geometry.cachedBarDrawCoordinates.ys

    readonly property var cachedBarRowCells: selectContext.visibleBarRowCells || []

    function barBaseState(row) {
        return geometry.cachedBarBaseStates && row >= 0 && row < geometry.cachedBarBaseStates.length
            ? geometry.cachedBarBaseStates[row]
            : null;
    }

    function barDrawState(row) {
        let fromState = geometry.barBaseState(row);
        let toState = row > 0 ? geometry.barBaseState(row - 1) : null;
        return fromState && toState && geometry.scrollOffset > 0.001
            ? geometry.interpolateBarState(fromState, toState, geometry.scrollOffset)
            : fromState;
    }

    function barSpriteScrollOffset(src) {
        return geometry.scrollOffset;
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
