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
    property Lr2AnimationFrameState sourceAnimationState: Lr2AnimationFrameState {
        enabled: root.sourceAnimates
        skinClock: root.skinClock
        clockMode: root.sourceSkinClockMode
        sourceData: root.srcData
        skinTime: root.sourceSkinClockMode === 0 ? root.sourceSkinTime : 0
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

    Lr2BarDistributionGraphItem {
        anchors.fill: parent
        visible: !!root.barRows && !!root.selectContext && !!root.srcData && !!root.graphTimelineState
        stateData: root.graphTimelineState
        sourceGraphType: root.graphType
        sourceSpecialType: root.sourceSpecialType
        sourceX: root.sourceX
        sourceY: root.sourceY
        sourceW: root.sourceW
        sourceH: root.sourceH
        sourceDivX: root.sourceDivX
        sourceDivY: root.sourceDivY
        sourcePath: root.sourcePath
        barCells: root.barCells || []
        barPositionMap: root.barPositionMap
        scaleOverride: root.scaleOverride
        frameOverrideBase: root.frameOverrideBase
        transColor: root.transColor
        colorKeyEnabled: root.colorKeyEnabled
        chartAssetSource: root.chartAssetSource
    }
}
