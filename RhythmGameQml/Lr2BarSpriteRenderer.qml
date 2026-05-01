import QtQuick
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property int sourceSkinTime: skinTime
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property var chart
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barBaseStates: []
    property var barPositionCache
    property var barCells: []
    property var barTextCells: []
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property real selectedFastBarDrawX: 0
    property real selectedFastBarDrawY: 0
    property int barCenter: 0
    property var barLampVariants: []
    property color transColor: "black"
    property bool colorKeyEnabled: false
    readonly property bool selectedBodySource: !!srcData
        && srcData.kind === 0
        && srcData.row === selectedRow
    readonly property bool applyFastBarScroll: fastBarScrollActive
        && !selectedBodySource
        && (!srcData || srcData.kind !== 2)
    x: applyFastBarScroll ? fastBarScrollX * scaleOverride : 0
    y: applyFastBarScroll ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property int barCellCount: barTextCells && barTextCells.length > 0
        ? barTextCells.length
        : (barCells ? barCells.length : 0)
    readonly property bool hasOverlayTimelineState: srcData && srcData.kind >= 2
    readonly property var overlayDsts: hasOverlayTimelineState ? dsts : []
    readonly property bool overlayHasStaticTimelineState: Lr2Timeline.canUseStaticState(overlayDsts)
    readonly property var overlayStaticTimelineState: overlayHasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(overlayDsts[0], overlayDsts[0])
        : null
    readonly property var overlayTimelineTimers: Lr2Timeline.dstsUseDynamicTimer(overlayDsts) ? timers : null
    readonly property var overlayTimelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(overlayDsts) ? activeOptions : []
    property Lr2TimelineState overlayTimelineCache: Lr2TimelineState {
        enabled: root.hasOverlayTimelineState && !root.overlayHasStaticTimelineState
        dsts: root.overlayDsts
        skinTime: root.skinTime
        timers: root.overlayTimelineTimers
        timerFire: root.timerFire
        activeOptions: root.overlayTimelineActiveOptions
    }
    readonly property var overlayTimelineState: hasOverlayTimelineState
        ? (overlayStaticTimelineState
           || overlayTimelineCache.state)
        : null

    readonly property var bodyRows: {
        if (!srcData || !selectContext || srcData.kind !== 0) {
            return [];
        }
        let total = barRows ? barRows.length : 0;
        let row = srcData.row;
        if (row <= 0 || row >= total || !rowData(row) || !rowData(row - 1)) {
            return [];
        }
        return [row];
    }
    readonly property int overlaySlotCount: {
        if (!srcData || !selectContext || srcData.kind < 2) {
            return 0;
        }

        if (srcData.kind === 4 || srcData.kind === 5 || srcData.kind === 7) {
            return 0;
        }

        // OpenLR2 draws BAR_RANK only while the IR ranking list is active.
        if (srcData.kind === 6 && !selectContext.rankingMode) {
            return 0;
        }

        let total = barRows ? barRows.length : 0;
        if (srcData.kind === 2) {
            return selectedRow > 0 && selectedRow < total ? 1 : 0;
        }
        return total;
    }

    function rowData(row) {
        return barRows && row >= 0 && row < barRows.length ? barRows[row] : null;
    }

    function visibilityState(row) {
        return cachedBaseState(row);
    }

    function stateField(state, name, fallback) {
        if (!state) {
            return fallback;
        }
        let value = state[name];
        return value === undefined || value === null ? fallback : value;
    }

    function sameStateField(a, b, name, fallback) {
        return Math.abs(stateField(a, name, fallback) - stateField(b, name, fallback)) <= 0.001;
    }

    function bodyNeedsStateInterpolation(row) {
        if (applyFastBarScroll) {
            return false;
        }

        let fromState = cachedBaseState(row);
        let toState = row > 0 ? cachedBaseState(row - 1) : null;
        if (!fromState || !toState) {
            return false;
        }

        return !sameStateField(fromState, toState, "w", 0)
            || !sameStateField(fromState, toState, "h", 0)
            || !sameStateField(fromState, toState, "a", 255)
            || !sameStateField(fromState, toState, "r", 255)
            || !sameStateField(fromState, toState, "g", 255)
            || !sameStateField(fromState, toState, "b", 255)
            || !sameStateField(fromState, toState, "angle", 0)
            || !sameStateField(fromState, toState, "center", 0)
            || !sameStateField(fromState, toState, "blend", 0)
            || !sameStateField(fromState, toState, "filter", 0)
            || !sameStateField(fromState, toState, "op4", 0);
    }

    function positionlessState(state) {
        if (!state) {
            return null;
        }
        return {
            x: 0,
            y: 0,
            w: state.w,
            h: state.h,
            a: state.a,
            r: state.r,
            g: state.g,
            b: state.b,
            blend: state.blend,
            filter: state.filter,
            angle: state.angle,
            center: state.center,
            sortId: state.sortId,
            op1: state.op1,
            op2: state.op2,
            op3: state.op3,
            op4: state.op4
        };
    }

    function cachedBaseState(row) {
        return barBaseStates && row >= 0 && row < barBaseStates.length
            ? barBaseStates[row]
            : null;
    }

    function slotForDisplayRow(row) {
        let count = root.barCellCount;
        let offset = root.selectContext ? root.selectContext.visibleBarSlotOffset : 0;
        return count > 0 && row >= 0 && row < count
            ? (row + offset) % count
            : -1;
    }

    function cellData(row) {
        let cells = barTextCells && barTextCells.length > 0 ? barTextCells : barCells;
        let slot = slotForDisplayRow(row);
        return cells && slot >= 0 && slot < cells.length ? cells[slot] : null;
    }

    function textCellData(slot) {
        return barTextCells && slot >= 0 && slot < barTextCells.length ? barTextCells[slot] : null;
    }

    function bodyDsts(row) {
        let data = rowData(row);
        if (!data) {
            return [];
        }
        return data.offDsts && data.offDsts.length > 0 ? data.offDsts : (data.onDsts || []);
    }

    function sourceForBody(row) {
        if (!selectContext || !srcData || !srcData.sources) {
            return srcData && srcData.source ? srcData.source : null;
        }
        let cell = cellData(row);
        let bodyType = cell ? (cell.bodyType || 0) : 0;
        return srcData.sources[bodyType] || srcData.sources[0] || srcData.source || null;
    }

    function overlayVisibleForValues(cellValid, lamp, ranking, rank) {
        if (!selectContext || !srcData) {
            return false;
        }
        if (srcData.kind === 2) {
            return true;
        }
        if (!cellValid) {
            return false;
        }
        switch (srcData.kind) {
        case 3:
            return (lamp || 0) === srcData.variant;
        case 6:
            return !!ranking && (rank || 0) === srcData.variant;
        default:
            return false;
        }
    }

    function spriteSourceSkinTime(source) {
        return Lr2Timeline.srcCyclesContinuously(source)
            ? root.sourceSkinTime
            : 0;
    }

    function spriteSourceTimerFire(source) {
        return Lr2Timeline.srcCyclesContinuously(source)
            ? root.sourceTimerFire
            : -2147483648;
    }

    Repeater {
        id: bodyRepeater

        model: root.bodyRows

        Lr2BarPositionedItem {
            id: bodyDelegate

            readonly property int bodyRow: modelData
            readonly property var baseState: root.cachedBaseState(bodyRow)
            readonly property bool needsStateInterpolation: root.bodyNeedsStateInterpolation(bodyRow)
            readonly property var bodySource: root.sourceForBody(bodyRow)
            readonly property var staticBodyState: root.positionlessState(baseState)
            readonly property var effectiveBodyState: needsStateInterpolation ? bodyInterpolatedState : staticBodyState
            positionCache: root.barPositionCache
            row: bodyDelegate.bodyRow
            scaleOverride: root.scaleOverride
            usePositionCache: !needsStateInterpolation && !root.applyFastBarScroll
            fallbackX: !needsStateInterpolation && baseState ? baseState.x : 0
            fallbackY: !needsStateInterpolation && baseState ? baseState.y : 0
            visible: !!effectiveBodyState
            width: root.width
            height: root.height

            Lr2BarInterpolatedState {
                id: bodyInterpolatedState
                enabled: bodyDelegate.needsStateInterpolation
                positionCache: root.barPositionCache
                baseStates: root.barBaseStates
                row: bodyDelegate.bodyRow
            }

            Lr2SpriteRenderer {
                anchors.fill: parent
                dsts: root.bodyDsts(bodyDelegate.bodyRow)
                srcData: bodyDelegate.bodySource
                stateOverride: bodyDelegate.effectiveBodyState
                skinTime: 0
                sourceSkinTime: root.spriteSourceSkinTime(bodyDelegate.bodySource)
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                sourceTimerFire: root.spriteSourceTimerFire(bodyDelegate.bodySource)
                chart: root.chart
                scaleOverride: root.scaleOverride
                transColor: root.transColor
                colorKeyEnabled: false
            }
        }
    }

    Repeater {
        id: overlayRepeater

        model: root.overlaySlotCount

        Lr2BarPositionedItem {
            id: overlayDelegate

            readonly property bool selectedOverlaySource: root.srcData && root.srcData.kind === 2
            readonly property bool usesRowPositionCache: !selectedOverlaySource
            readonly property int displayRow: selectedOverlaySource ? slot : effectiveRow
            readonly property var visibleBase: selectedOverlaySource ? root.visibilityState(displayRow) : null
            readonly property var cell: selectedOverlaySource
                ? null
                : root.cellData(displayRow)
            readonly property bool cellValid: selectedOverlaySource ? true : !!cell
            readonly property int cellLamp: cell ? (cell.lamp || 0) : 0
            readonly property bool cellRanking: cell ? !!cell.ranking : false
            readonly property int cellRank: cell ? (cell.rank || 0) : 0
            readonly property bool contentVisible: (selectedOverlaySource
                    ? displayRow > 0 && !!visibleBase
                    : rowVisible)
                && root.overlayVisibleForValues(cellValid, cellLamp, cellRanking, cellRank)
            positionCache: root.barPositionCache
            row: selectedOverlaySource ? displayRow : -1
            slot: selectedOverlaySource ? root.selectedRow : modelData
            scaleOverride: root.scaleOverride
            useSlotRow: usesRowPositionCache
            usePositionCache: usesRowPositionCache
            hasOverride: false
            fallbackX: selectedOverlaySource && visibleBase ? visibleBase.x : 0
            fallbackY: selectedOverlaySource && visibleBase ? visibleBase.y : 0
            width: root.width
            height: root.height
            visible: contentVisible

            Lr2SpriteRenderer {
                anchors.fill: parent
                dsts: root.dsts
                srcData: root.srcData ? root.srcData.source : null
                skinTime: 0
                sourceSkinTime: root.spriteSourceSkinTime(srcData)
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                sourceTimerFire: root.spriteSourceTimerFire(srcData)
                chart: root.chart
                scaleOverride: root.scaleOverride
                transColor: root.transColor
                colorKeyEnabled: root.colorKeyEnabled
                stateOverride: root.overlayTimelineState
            }
        }
    }
}
