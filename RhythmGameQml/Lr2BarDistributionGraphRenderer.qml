import QtQuick
import RhythmGameQml 1.0

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property int sourceSkinTime: skinTime
    property var skinClock: null
    property int sourceSkinClockMode: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property var chart
    property string chartAssetSource: ""
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barPositionMap
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
    readonly property int sourceSpecialType: srcData ? (srcData.specialType || 0) : 0
    readonly property real sourceX: srcData ? (srcData.x || 0) : 0
    readonly property real sourceY: srcData ? (srcData.y || 0) : 0
    readonly property real sourceW: srcData ? (srcData.w || 0) : 0
    readonly property real sourceH: srcData ? (srcData.h || 0) : 0
    readonly property int sourceDivX: srcData ? Math.max(1, srcData.div_x || 1) : 1
    readonly property int sourceDivY: srcData ? Math.max(1, srcData.div_y || 1) : 1
    readonly property string sourcePath: srcData && srcData.source ? srcData.source : ""
    readonly property int segmentCount: graphType === 0 ? 11 : 28
    readonly property int frameCountValue: srcData
        ? Math.max(1, srcData.div_x || 1) * Math.max(1, srcData.div_y || 1)
        : 1
    readonly property int animationGroupCount: Math.max(1, Math.floor(frameCountValue / segmentCount))
    readonly property bool sourceAnimates: srcData && animationGroupCount > 1 && (srcData.cycle || 0) > 0
    readonly property var graphDsts: srcData ? dsts : []
    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.graphDsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }
    readonly property var graphTimelineState: staticTimelineState
        || timelineState.state

    readonly property bool hasGraphState: !!graphTimelineState
    readonly property real stateX: hasGraphState ? (graphTimelineState.x || 0) : 0
    readonly property real stateY: hasGraphState ? (graphTimelineState.y || 0) : 0
    readonly property real stateW: hasGraphState ? (graphTimelineState.w || 0) : 0
    readonly property real stateH: hasGraphState ? (graphTimelineState.h || 0) : 0
    readonly property real stateA: hasGraphState ? (graphTimelineState.a === undefined ? 255 : graphTimelineState.a) : 0
    readonly property real stateR: hasGraphState ? (graphTimelineState.r === undefined ? 255 : graphTimelineState.r) : 255
    readonly property real stateG: hasGraphState ? (graphTimelineState.g === undefined ? 255 : graphTimelineState.g) : 255
    readonly property real stateB: hasGraphState ? (graphTimelineState.b === undefined ? 255 : graphTimelineState.b) : 255
    readonly property real stateAngle: hasGraphState ? (graphTimelineState.angle || 0) : 0
    readonly property int stateCenter: hasGraphState ? (graphTimelineState.center || 0) : 0
    readonly property int stateBlend: hasGraphState ? (graphTimelineState.blend || 0) : 0
    readonly property int stateFilter: hasGraphState ? (graphTimelineState.filter || 0) : 0
    readonly property real baseX: stateX + (stateW < 0 ? stateW : 0)
    readonly property real baseY: stateY + (stateH < 0 ? stateH : 0)
    readonly property real fullW: Math.abs(stateW)
    readonly property real fullH: Math.abs(stateH)
    readonly property bool graphRenderable: !!barRows
        && !!selectContext
        && !!srcData
        && hasGraphState
        && stateA > 0
        && fullW > 0
        && fullH > 0

    property Lr2AnimationFrameState sourceAnimationState: Lr2AnimationFrameState {
        enabled: root.sourceAnimates
        skinClock: root.sourceAnimates ? root.skinClock : null
        clockMode: root.sourceAnimates ? root.sourceSkinClockMode : 0
        sourceData: root.srcData
        skinTime: root.sourceSkinClockMode === 0 ? root.sourceSkinTime : 0
        timers: root.timers
        timerFire: root.sourceTimerFire
        frameGroupSize: root.segmentCount
    }
    readonly property int frameOverrideBase: root.sourceAnimates
        ? sourceAnimationState.frameIndex * root.segmentCount
        : 0

    function graphValues(cell: var) : var {
        if (!cell) {
            return [];
        }
        return root.graphType === 0 ? cell.graphLamps : cell.graphRanks;
    }

    function graphAmount(values: var, segment: var) : var {
        if (!values || segment < 0 || segment >= values.length) {
            return 0;
        }
        let value = Number(values[segment]);
        return isFinite(value) ? Math.max(0, value) : 0;
    }

    function graphTotal(values: var) : var {
        let total = 0;
        for (let segment = 0; segment < root.segmentCount; ++segment) {
            total += graphAmount(values, segment);
        }
        return total;
    }

    function segmentStart(values: var, total: var, segment: var) : var {
        if (total <= 0) {
            return 0;
        }

        let accumulated = 0;
        for (let current = root.segmentCount - 1; current > segment; --current) {
            accumulated += graphAmount(values, current);
        }
        return accumulated * root.fullW / total;
    }

    function segmentWidth(values: var, total: var, segment: var) : var {
        if (total <= 0) {
            return 0;
        }
        return graphAmount(values, segment) * root.fullW / total;
    }

    function segmentState(start: var, widthValue: var) : var {
        return {
            x: root.baseX + start,
            y: root.baseY,
            w: widthValue,
            h: root.fullH,
            a: root.stateA,
            r: root.stateR,
            g: root.stateG,
            b: root.stateB,
            angle: root.stateAngle,
            center: root.stateCenter,
            blend: root.stateBlend,
            filter: root.stateFilter
        };
    }

    Repeater {
        id: rowRepeater

        model: root.graphRenderable ? (root.barCells || []) : []

        Lr2BarPositionedItem {
            id: rowDelegate

            readonly property var cell: modelData
            readonly property var values: root.graphValues(cell)
            readonly property real total: root.graphTotal(values)
            readonly property bool contentVisible: rowVisible
                && !!cell
                && cell.valid
                && total > 0
            positionMap: root.barPositionMap
            slot: index
            useSlotRow: true
            usePositionMap: true
            scaleOverride: root.scaleOverride
            width: root.width
            height: root.height
            visible: contentVisible

            Repeater {
                model: rowDelegate.contentVisible ? root.segmentCount : 0

                Loader {
                    id: segmentLoader

                    readonly property int segmentIndex: index
                    readonly property real segmentStartX: root.segmentStart(rowDelegate.values, rowDelegate.total, segmentIndex)
                    readonly property real segmentW: root.segmentWidth(rowDelegate.values, rowDelegate.total, segmentIndex)
                    anchors.fill: parent
                    active: segmentW > 0
                    sourceComponent: Component {
                        Lr2SpriteRenderer {
                            anchors.fill: parent
                            dsts: []
                            srcData: root.srcData
                            stateOverride: root.segmentState(segmentLoader.segmentStartX, segmentLoader.segmentW)
                            skinTime: 0
                            sourceSkinTime: 0
                            skinClock: null
                            sourceSkinClockMode: 0
                            activeOptions: root.activeOptions
                            timers: root.timers
                            timerFire: -2147483648
                            sourceTimerFire: -2147483648
                            frameOverride: Math.min(root.frameCountValue - 1, root.frameOverrideBase + segmentLoader.segmentIndex)
                            scaleOverride: root.scaleOverride
                            colorKeyEnabled: root.colorKeyEnabled
                            transColor: root.transColor
                            chartAssetSource: root.chartAssetSource
                        }
                    }
                }
            }
        }
    }
}
