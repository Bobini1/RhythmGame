import QtQuick

import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barBaseStates: []
    property var barDrawXs: []
    property var barDrawYs: []
    property var barCells: []
    property real barScrollOffset: 0
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property int barCenter: 0
    property bool colorKeyEnabled: false
    property color transColor: "black"
    x: fastBarScrollActive ? fastBarScrollX * scaleOverride : 0
    y: fastBarScrollActive ? fastBarScrollY * scaleOverride : 0

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
    readonly property var numberTimelineState: staticTimelineState
        || Lr2Timeline.getCurrentState(numberDsts, skinTime, timelineTimers, timelineActiveOptions)

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

    function visibleForCell(cell) {
        if (!cell || !srcData || !selectContext) {
            return false;
        }
        if (!cell.ranking && !cell.chartLike && !cell.entryLike) {
            return false;
        }
        if (cell.ranking) {
            return srcData.variant === 6;
        }
        if ((cell.keymode || 0) <= 0 || (cell.playLevel || 0) <= 0) {
            return false;
        }
        let difficulty = cell.difficulty || 0;
        if (difficulty <= 0) {
            return srcData.variant === 0;
        }
        return srcData.variant === difficulty;
    }

    Repeater {
        model: root.numberSlotCount

        Item {
            readonly property int row: modelData
            readonly property var cell: root.cellData(row)
            readonly property var visibleBase: root.visibilityState(row)
            readonly property real drawX: root.barDrawXs && row >= 0 && row < root.barDrawXs.length
                ? root.barDrawXs[row]
                : (visibleBase ? visibleBase.x : 0)
            readonly property real drawY: root.barDrawYs && row >= 0 && row < root.barDrawYs.length
                ? root.barDrawYs[row]
                : (visibleBase ? visibleBase.y : 0)
            readonly property bool contentVisible: row > 0
                && !!visibleBase
                && root.visibleForCell(cell)
            x: drawX * root.scaleOverride
            y: drawY * root.scaleOverride
            width: root.width
            height: root.height
            visible: contentVisible

            Lr2NumberRenderer {
                anchors.fill: parent
                dsts: root.dsts
                srcData: root.srcData ? root.srcData.source : null
                skinTime: root.skinTime
                activeOptions: root.activeOptions
                timers: root.timers
                scaleOverride: root.scaleOverride
                colorKeyEnabled: root.colorKeyEnabled
                transColor: root.transColor
                stateOverride: root.numberTimelineState
                value: parent.cell ? (parent.cell.playLevel || 0) : 0
            }
        }
    }
}
