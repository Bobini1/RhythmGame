pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

QtObject {
    id: geometry

    required property var screenRoot
    required property var skinModel
    required property var selectContext

    readonly property var root: screenRoot

    readonly property bool fastBarScrollActive: barBaseStateResolver.fastScrollActive
    readonly property real fastBarScrollX: fastBarScrollActive && selectContext
        ? -barBaseStateResolver.fastScrollDx * selectContext.visualStateObject.offset
        : 0
    readonly property real fastBarScrollY: fastBarScrollActive && selectContext
        ? -barBaseStateResolver.fastScrollDy * selectContext.visualStateObject.offset
        : 0
    readonly property real selectedFastBarDrawX: 0
    readonly property real selectedFastBarDrawY: 0
    readonly property int selectedRow: skinModel.barCenter + selectContext.selectedOffset
    readonly property int clickStartRow: Math.max(0, skinModel.barAvailableStart - 3)
    readonly property int clickEndRow: Math.min(
        (skinModel.barRows ? skinModel.barRows.length : 0) - 1,
        skinModel.barAvailableEnd + 3)
    property Lr2BarBaseStateResolver barBaseStateResolver: Lr2BarBaseStateResolver {
        barRows: geometry.skinModel.barRows
        selectedRow: geometry.selectedRow
        skinTime: geometry.root.barSkinTime
        timers: geometry.root.barTimers
        activeOptions: geometry.root.barActiveOptions
    }

    property Lr2BarPositionMap barPositionMap: Lr2BarPositionMap {
        baseStateResolver: geometry.barBaseStateResolver
        slotOffset: geometry.selectContext ? geometry.selectContext.visibleBarSlotOffset : 0
        slotCount: geometry.skinModel.barRows ? geometry.skinModel.barRows.length : 0
        visualState: geometry.fastBarScrollActive || !geometry.selectContext
            ? null
            : geometry.selectContext.visualStateObject
    }

    function selectedBarRow() : var {
        return geometry.selectedRow;
    }

    function barClickStart() : var {
        return geometry.clickStartRow;
    }

    function barClickEnd() : var {
        return geometry.clickEndRow;
    }

    function barRowCanClick(row: var) : var {
        if (skinModel.barAvailableEnd < skinModel.barAvailableStart) {
            return false;
        }
        return row >= geometry.clickStartRow && row <= geometry.clickEndRow;
    }

    function barRowScrollDelta(row: var) : var {
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

    function handleBarRowClick(row: var, mouse: var) : var {
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
