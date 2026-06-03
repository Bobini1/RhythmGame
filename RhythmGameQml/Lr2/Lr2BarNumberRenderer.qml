import QtQuick
import RhythmGameQml

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barBaseStateResolver
    property var barPositionMap
    property var barCells: []
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property real selectedFastBarDrawX: 0
    property real selectedFastBarDrawY: 0
    property int barCenter: 0
    property bool colorKeyEnabled: false
    property color transColor: "black"
    readonly property int barBaseStateRevision: barBaseStateResolver
        ? barBaseStateResolver.baseStatesRevision
        : 0
    x: fastBarScrollActive ? fastBarScrollX * scaleOverride : 0
    y: fastBarScrollActive ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property int barCellCount: barCells ? barCells.length : 0

    readonly property int numberSlotCount: barRows && selectContext && srcData
        ? Math.max(0, barRows.length)
        : 0
    readonly property var numberDsts: srcData && srcData.source ? dsts : []
    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState && timelineState.staticState.valid
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.numberDsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }
    readonly property var numberTimelineState: staticTimelineState
        || (timelineState.hasState ? timelineState.state : null)
    readonly property bool numberSourceAnimates: srcData && srcData.source
        && (srcData.source.cycle || 0) > 0

    function baseState(row: var) : var {
        root.barBaseStateRevision;
        if (!barBaseStateResolver || row < 0 || row >= barBaseStateResolver.stateCount()) {
            return null;
        }
        let state = barBaseStateResolver.stateAt(row);
        return state && state.valid ? state : null;
    }

    function visibilityState(row: var) : var {
        return baseState(row);
    }

    function cellData(row: var) : var {
        return barCells && row >= 0 && row < barCells.length ? barCells[row] : null;
    }

    function numberVisibleForCell(cell: var, sourceVariant: var) : var {
        if (!cell || !cell.valid || !srcData || !selectContext) {
            return false;
        }
        if (!cell.ranking && !cell.chartLike && !cell.entryLike) {
            return false;
        }
        if (cell.ranking) {
            return sourceVariant === 0 || sourceVariant === 6;
        }
        if ((cell.keymode || 0) <= 0 || (cell.playLevel || 0) < 0) {
            return false;
        }
        if (cell.difficulty <= 0) {
            return sourceVariant === 0;
        }
        return sourceVariant === cell.difficulty;
    }

    Repeater {
        id: numberRepeater

        model: root.numberSlotCount

        Lr2BarPositionedItem {
            id: barNumberDelegate

            readonly property var cell: root.barCells && slot >= 0 && slot < root.barCells.length
                ? root.barCells[slot]
                : null
            readonly property int sourceVariant: root.srcData ? (root.srcData.variant || 0) : 0
            readonly property bool cellNumberVisible: root.numberVisibleForCell(cell, sourceVariant)
            readonly property int cellPlayLevel: cellNumberVisible
                ? cell.playLevel
                : -2147483648
            readonly property bool contentVisible: rowVisible && cellPlayLevel !== -2147483648
            positionMap: root.barPositionMap
            slot: index
            scaleOverride: root.scaleOverride
            useSlotRow: true
            usePositionMap: true
            hasOverride: false
            adjustX: 0
            adjustY: 0
            fallbackX: 0
            fallbackY: 0
            width: root.width
            height: root.height
            visible: contentVisible

            Lr2NumberRenderer {
                anchors.fill: parent
                dsts: root.dsts
                srcData: root.srcData ? root.srcData.source : null
                skinTime: root.numberSourceAnimates ? root.skinTime : 0
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                sourceTimerFire: root.numberSourceAnimates ? root.sourceTimerFire : -2147483648
                scaleOverride: root.scaleOverride
                colorKeyEnabled: root.colorKeyEnabled
                transColor: root.transColor
                stateOverride: root.numberTimelineState
                value: parent.contentVisible ? parent.cellPlayLevel : 0
            }
        }
    }
}
