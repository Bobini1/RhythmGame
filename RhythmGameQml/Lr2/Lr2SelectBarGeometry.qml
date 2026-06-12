pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

QtObject {
    id: geometry

    required property var screenRoot
    required property var skinModel
    required property var selectContext
    required property int selectedOffset
    required property Lr2SelectBarModel visibleBarModel
    required property Lr2SelectVisualState selectVisualState

    readonly property var root: screenRoot

    readonly property bool fastBarScrollActive: barBaseStateResolver.fastScrollActive
    readonly property real fastBarScrollX: fastBarScrollActive && selectVisualState
        ? -barBaseStateResolver.fastScrollDx * selectVisualState.offset
        : 0
    readonly property real fastBarScrollY: fastBarScrollActive && selectVisualState
        ? -barBaseStateResolver.fastScrollDy * selectVisualState.offset
        : 0
    readonly property real selectedFastBarDrawX: 0
    readonly property real selectedFastBarDrawY: 0
    readonly property int selectedRow: skinModel.barCenter + selectedOffset
    readonly property int barRenderSkinTime: root.barSkinTime
    readonly property int barLayoutSkinTime: Math.max(barRenderSkinTime, root.barAnimationLimit)
    readonly property var barBaseStates: barLayoutBaseStateResolver.baseStates
    readonly property int clickStartRow: Math.max(0, skinModel.barAvailableStart - 3)
    readonly property int clickEndRow: Math.min(
        (skinModel.barRows ? skinModel.barRows.length : 0) - 1,
        skinModel.barAvailableEnd + 3)
    property Lr2BarBaseStateResolver barBaseStateResolver: Lr2BarBaseStateResolver {
        barRows: geometry.skinModel.barRows
        selectedRow: geometry.selectedRow
        skinTime: geometry.barRenderSkinTime
        timers: geometry.root.barTimers
        activeOptions: geometry.root.barActiveOptions
    }

    property Lr2BarBaseStateResolver barLayoutBaseStateResolver: Lr2BarBaseStateResolver {
        barRows: geometry.skinModel.barRows
        selectedRow: geometry.selectedRow
        skinTime: geometry.barLayoutSkinTime
        timers: geometry.root.barTimers
        activeOptions: geometry.root.barActiveOptions
    }

    property Lr2BarPositionMap barPositionMap: Lr2BarPositionMap {
        baseStateResolver: geometry.barBaseStateResolver
        slotOffsetModel: geometry.visibleBarModel
        slotCount: geometry.skinModel.barRows ? geometry.skinModel.barRows.length : 0
        visualState: geometry.fastBarScrollActive || !geometry.selectVisualState
            ? null
            : geometry.selectVisualState
    }

    function handleBarRowClick(row: var, mouse: var) : var {
        root.clearSelectSearchFocus();

        let first = skinModel.barAvailableStart;
        let last = skinModel.barAvailableEnd;
        let delta = row < first
            ? -2 * (first - row)
            : (row > last ? 2 * (row - last) : 0);
        if (delta !== 0) {
            if (!selectContext.visualMoveActive) {
                selectContext.scrollBy(delta, selectContext.lr2ClickDuration);
            }
            return;
        }

        if (row === geometry.selectedRow) {
            if (root.launchLr2RankingScoreAction(mouse.button)) {
                return;
            }
            if (mouse.button === Qt.RightButton) {
                root.selectGoBack();
                return;
            }
            if (mouse.button === Qt.LeftButton) {
                root.selectGoForward();
            }
            return;
        }

        if (mouse.button === Qt.RightButton) {
            root.selectGoBack();
            return;
        }

        selectContext.selectVisibleRow(row, skinModel.barCenter);
    }
}
