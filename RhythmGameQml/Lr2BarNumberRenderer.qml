import QtQuick
import RhythmGameQml 1.0

import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
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
    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(numberDsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(numberDsts[0], numberDsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(numberDsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(numberDsts) ? activeOptions : []
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.numberDsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptions: root.timelineActiveOptions
    }
    readonly property var numberTimelineState: staticTimelineState
        || timelineState.state
    readonly property bool numberSourceAnimates: srcData && srcData.source
        && (srcData.source.cycle || 0) > 0

    function baseState(row) {
        return barBaseStates && row >= 0 && row < barBaseStates.length
            ? barBaseStates[row]
            : null;
    }

    function visibilityState(row) {
        return baseState(row);
    }

    function cellData(row) {
        return barCells && row >= 0 && row < barCells.length ? barCells[row] : null;
    }

    function visibleForValues(cellValid, ranking, chartLike, entryLike, keymode, playLevel, difficulty) {
        if (!cellValid || !srcData || !selectContext) {
            return false;
        }
        if (!ranking && !chartLike && !entryLike) {
            return false;
        }
        if (ranking) {
            return srcData.variant === 6;
        }
        if ((keymode || 0) <= 0 || (playLevel || 0) <= 0) {
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
            readonly property bool cellValid: !!cell
            readonly property bool cellRanking: cell ? !!cell.ranking : false
            readonly property bool cellChartLike: cell ? !!cell.chartLike : false
            readonly property bool cellEntryLike: cell ? !!cell.entryLike : false
            readonly property int cellKeymode: cell ? (cell.keymode || 0) : 0
            readonly property int cellPlayLevel: cell ? (cell.playLevel || 0) : 0
            readonly property int cellDifficulty: cell ? (cell.difficulty || 0) : 0
            readonly property bool contentVisible: rowVisible
                && root.visibleForValues(cellValid, cellRanking, cellChartLike, cellEntryLike,
                    cellKeymode, cellPlayLevel, cellDifficulty)
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
                value: parent.cellPlayLevel
            }
        }
    }
}
