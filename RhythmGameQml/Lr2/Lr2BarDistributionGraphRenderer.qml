import QtQuick
import RhythmGameQml

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

    readonly property int graphType: srcData ? (srcData.graphType || 0) : 0
    readonly property int segmentCount: graphType === 0 ? 11 : 28
    readonly property int frameCountValue: srcData
        ? Math.max(1, srcData.div_x || 1) * Math.max(1, srcData.div_y || 1)
        : 1
    readonly property int animationGroupCount: Math.max(1, Math.floor(frameCountValue / segmentCount))
    readonly property bool sourceAnimates: srcData && animationGroupCount > 1 && (srcData.cycle || 0) > 0
    readonly property var graphDsts: srcData ? dsts : []
    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState && timelineState.staticState.valid
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    readonly property var graphTimelineState: staticTimelineState
        || (timelineState.hasState ? timelineState.state : null)

    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.graphDsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }

    Lr2BarDistributionGraphItem {
        anchors.fill: parent
        visible: !!root.srcData && !!root.graphTimelineState && !!root.barCells
        srcData: root.srcData
        skinClock: root.sourceAnimates ? root.skinClock : null
        sourceSkinClockMode: root.sourceAnimates ? root.sourceSkinClockMode : 0
        sourceSkinTime: root.sourceSkinClockMode === 0 ? root.sourceSkinTime : 0
        sourceTimerFire: root.sourceTimerFire
        stateData: root.graphTimelineState
        barCells: root.barCells || []
        barPositionMap: root.barPositionMap
        scaleOverride: root.scaleOverride
        colorKeyEnabled: root.colorKeyEnabled
        transColor: root.transColor
        chartAssetSource: root.chartAssetSource
    }
}
