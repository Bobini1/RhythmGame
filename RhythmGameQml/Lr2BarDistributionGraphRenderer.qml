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
    property var barPositionCache
    property var barCells: []
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property color transColor: "black"
    property bool colorKeyEnabled: false

    x: fastBarScrollActive ? fastBarScrollX * scaleOverride : 0
    y: fastBarScrollActive ? fastBarScrollY * scaleOverride : 0

    readonly property int barCellCount: barCells ? barCells.length : 0
    readonly property int graphType: srcData ? (srcData.graphType || 0) : 0
    readonly property int segmentCount: graphType === 0 ? 11 : 28
    readonly property var graphDsts: srcData ? dsts : []
    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(graphDsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(graphDsts[0], graphDsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(graphDsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(graphDsts) ? activeOptions : []
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.graphDsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptions: root.timelineActiveOptions
    }
    readonly property var graphTimelineState: staticTimelineState
        || timelineState.state

    function graphValues(cell) {
        if (!cell) {
            return [];
        }
        return root.graphType === 0 ? (cell.graphLamps || []) : (cell.graphRanks || []);
    }

    function segmentTotal(values) {
        let total = 0;
        for (let i = 0; i < values.length; ++i) {
            total += Math.max(0, Number(values[i]) || 0);
        }
        return total;
    }

    function frameCount() {
        if (!srcData) {
            return 1;
        }
        return Math.max(1, srcData.div_x || 1) * Math.max(1, srcData.div_y || 1);
    }

    function animationGroupFrame() {
        if (!srcData) {
            return 0;
        }

        let groups = Math.max(1, Math.floor(frameCount() / segmentCount));
        let cycle = srcData.cycle || 0;
        if (groups <= 1 || cycle <= 0) {
            return 0;
        }

        let fire = 0;
        if ((srcData.timer || 0) > 0) {
            fire = sourceTimerFire;
            if (fire < -2147480000) {
                return 0;
            }
        }

        let animTime = sourceSkinTime - fire;
        if (animTime < 0) {
            return 0;
        }

        let phase = animTime % cycle;
        let msPerFrame = cycle / groups;
        return Math.max(0, Math.min(groups - 1, Math.floor(phase / msPerFrame)));
    }

    function frameForSegment(segment) {
        return Math.min(frameCount() - 1, animationGroupFrame() * segmentCount + segment);
    }

    function segmentState(base, start, width) {
        let fullH = Math.abs(base.h || 0);
        return {
            x: (base.x || 0) + Math.min(0, base.w || 0) + start,
            y: (base.y || 0) + Math.min(0, base.h || 0),
            w: width,
            h: fullH,
            a: base.a,
            r: base.r,
            g: base.g,
            b: base.b,
            blend: base.blend,
            filter: base.filter,
            angle: base.angle,
            center: base.center,
            sortId: base.sortId,
            op1: base.op1,
            op2: base.op2,
            op3: base.op3,
            op4: base.op4
        };
    }

    function segmentsForCell(cell) {
        let state = graphTimelineState;
        if (!cell || !cell.folderLike || !state) {
            return [];
        }

        let values = graphValues(cell);
        let total = segmentTotal(values);
        if (total <= 0) {
            return [];
        }

        let fullW = Math.abs(state.w || 0);
        let fullH = Math.abs(state.h || 0);
        if (fullW <= 0 || fullH <= 0) {
            return [];
        }

        let result = [];
        let accumulated = 0;
        for (let i = segmentCount - 1; i >= 0; --i) {
            let amount = Math.max(0, Number(values[i]) || 0);
            if (amount <= 0) {
                continue;
            }

            let start = accumulated * fullW / total;
            let width = amount * fullW / total;
            result.push({
                segment: i,
                state: segmentState(state, start, width)
            });
            accumulated += amount;
        }
        return result;
    }

    Repeater {
        model: root.barRows && root.selectContext && root.srcData
            ? Math.max(0, root.barRows.length)
            : 0

        Lr2BarPositionedItem {
            id: graphDelegate

            readonly property var cell: root.barCells && slot >= 0 && slot < root.barCells.length
                ? root.barCells[slot]
                : null
            readonly property int cellRevision: cell ? cell.revision : -1
            readonly property var segmentModel: {
                let revision = cellRevision;
                return root.segmentsForCell(cell);
            }
            readonly property bool contentVisible: rowVisible && segmentModel.length > 0

            positionCache: root.barPositionCache
            slot: index
            scaleOverride: root.scaleOverride
            useSlotRow: true
            usePositionCache: true
            hasOverride: false
            width: root.width
            height: root.height
            visible: contentVisible

            Repeater {
                model: graphDelegate.segmentModel

                Lr2SpriteRenderer {
                    anchors.fill: parent
                    dsts: []
                    srcData: root.srcData
                    skinTime: 0
                    sourceSkinTime: 0
                    activeOptions: root.activeOptions
                    timers: root.timers
                    timerFire: -2147483648
                    sourceTimerFire: -2147483648
                    chart: root.chart
                    scaleOverride: root.scaleOverride
                    transColor: root.transColor
                    colorKeyEnabled: root.colorKeyEnabled
                    frameOverride: root.frameForSegment(modelData.segment)
                    stateOverride: modelData.state
                }
            }
        }
    }
}
