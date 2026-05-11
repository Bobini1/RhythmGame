import QtQuick
import RhythmGameQml 1.0

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
    readonly property int frameCountValue: srcData
        ? Math.max(1, srcData.div_x || 1) * Math.max(1, srcData.div_y || 1)
        : 1
    readonly property int animationGroupCount: Math.max(1, Math.floor(frameCountValue / segmentCount))
    readonly property bool sourceAnimates: srcData && animationGroupCount > 1 && (srcData.cycle || 0) > 0
    property Lr2AnimationFrameState sourceAnimationState: Lr2AnimationFrameState {
        enabled: root.sourceAnimates
        sourceData: root.srcData
        skinTime: root.sourceSkinTime
        timerFire: root.sourceTimerFire
        frameGroupSize: root.segmentCount
    }
    readonly property int animationGroupFrameValue: sourceAnimationState.frameIndex
    readonly property int frameOverrideBase: animationGroupFrameValue * segmentCount
    readonly property var graphDsts: srcData ? dsts : []
    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    readonly property var timelineActiveOptions: timelineState.usesActiveOptions ? activeOptions : []
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

    Lr2BarDistributionGraphItem {
        anchors.fill: parent
        visible: root.barRows && root.selectContext && root.srcData
        srcData: root.srcData
        stateData: root.graphTimelineState
        barPositionCache: root.barPositionCache
        barCells: root.barCells || []
        scaleOverride: root.scaleOverride
        frameOverrideBase: root.frameOverrideBase
        transColor: root.transColor
        colorKeyEnabled: root.colorKeyEnabled
    }
}
