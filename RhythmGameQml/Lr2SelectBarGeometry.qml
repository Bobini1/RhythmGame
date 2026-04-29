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
    readonly property var cachedBarBaseStates: {
        let rows = skinModel.barRows || [];
        let result = [];
        for (let row = 0; row < rows.length; ++row) {
            result.push(geometry.computeBarBaseState(row, geometry.selectedRow));
        }
        return result;
    }

    property Lr2BarPositionCache barPositionCache: Lr2BarPositionCache {
        baseStates: geometry.cachedBarBaseStates
        visualState: geometry.selectContext ? geometry.selectContext.visualStateObject : null
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
