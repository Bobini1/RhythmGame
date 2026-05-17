import QtQuick
import RhythmGameQml 1.0

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
    property var barBaseStates: []
    property var barPositionCache
    property var barCells: []
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property real selectedFastBarDrawX: 0
    property real selectedFastBarDrawY: 0
    property int barCenter: 0
    property bool colorKeyEnabled: false
    property color transColor: "black"
    x: fastBarScrollActive ? fastBarScrollX * scaleOverride : 0
    y: fastBarScrollActive ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property int barCellCount: barCells ? barCells.length : 0

    readonly property int numberSlotCount: barRows && selectContext && srcData
        ? Math.max(0, barRows.length)
        : 0
    readonly property var numberDsts: srcData && srcData.source ? dsts : []
    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState
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
        || timelineState.state
    readonly property bool numberSourceAnimates: srcData && srcData.source
        && (srcData.source.cycle || 0) > 0

    function baseState(row: var) : var {
        return barBaseStates && row >= 0 && row < barBaseStates.length
            ? barBaseStates[row]
            : null;
    }

    function visibilityState(row: var) : var {
        return baseState(row);
    }

    function cellData(row: var) : var {
        return barCells && row >= 0 && row < barCells.length ? barCells[row] : null;
    }

    function visibleForValues(cellValid: var, ranking: var, chartLike: var, entryLike: var, keymode: var, playLevel: var, difficulty: var) : var {
        if (!cellValid || !srcData || !selectContext) {
            return false;
        }
        if (!ranking && !chartLike && !entryLike) {
            return false;
        }
        if (ranking) {
            return srcData.variant === 0 || srcData.variant === 6;
        }
        if ((keymode || 0) <= 0 || (playLevel || 0) < 0) {
            return false;
        }
        if (difficulty <= 0) {
            return srcData.variant === 0;
        }
        return srcData.variant === difficulty;
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
            readonly property int cellRevision: cell ? cell.revision : -1
            readonly property int cellPlayLevel: cell && cellRevision >= 0
                ? cell.numberValueOrInvisibleForVariant(sourceVariant)
                : -2147483648
            readonly property bool contentVisible: rowVisible && cellPlayLevel !== -2147483648
            positionCache: root.barPositionCache
            slot: index
            scaleOverride: root.scaleOverride
            useSlotRow: true
            usePositionCache: true
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
