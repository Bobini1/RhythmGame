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
    x: fastBarScrollActive ? fastBarScrollX * scaleOverride : 0
    y: fastBarScrollActive ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property int barCellCount: barCells ? barCells.length : 0
    readonly property var timelineDsts: {
        if (!dsts || dsts.length === 0 || !dsts[0]) {
            return dsts;
        }

        // OpenLR2's select bar title path applies the type-0/type-1 source
        // choice itself and does not gate #DST_BAR_TITLE by opt1/opt2/opt3.
        let result = dsts.slice();
        let first = {};
        for (let key in dsts[0]) {
            first[key] = dsts[0][key];
        }
        first.op1 = 0;
        first.op2 = 0;
        first.op3 = 0;
        result[0] = first;
        return result;
    }

    readonly property int textSlotCount: barRows && selectContext && srcData
        ? Math.max(0, barRows.length)
        : 0
    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(timelineDsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(timelineDsts[0], timelineDsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(timelineDsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(timelineDsts) ? activeOptions : []
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.timelineDsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptions: root.timelineActiveOptions
    }
    readonly property var textTimelineState: staticTimelineState
        || timelineState.state

    function visibleFor(cellValid, cellTitleType) {
        return cellValid && !!srcData && !!selectContext
            && cellTitleType === srcData.titleType;
    }

    function baseState(row) {
        return barBaseStates && row >= 0 && row < barBaseStates.length
            ? barBaseStates[row]
            : null;
    }

    function visibilityState(row) {
        return baseState(row);
    }

    Repeater {
        id: textRepeater

        model: root.textSlotCount

        Lr2BarPositionedItem {
            id: barTextDelegate

            readonly property var cell: root.barCells && slot >= 0 && slot < root.barCells.length
                ? root.barCells[slot]
                : null
            readonly property bool cellValid: cell ? !!cell.valid : false
            readonly property int cellTitleType: cell ? cell.titleType : -1
            readonly property string cellText: cell ? (cell.text || "") : ""
            readonly property bool contentVisible: rowVisible
                && root.visibleFor(cellValid, cellTitleType)
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

            Lr2TextRenderer {
                anchors.fill: parent
                dsts: root.timelineDsts
                srcData: root.srcData
                skinTime: 0
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                scaleOverride: root.scaleOverride
                stateOverride: root.textTimelineState
                resolvedText: barTextDelegate.cellText
            }
        }
    }
}
