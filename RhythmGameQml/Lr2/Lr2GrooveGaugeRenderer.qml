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
    property var screenRoot
    property real scaleOverride: 1.0
    property bool mediaActive: true
    property color transColor: "black"
    property int trembleRoll: 0

    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState && timelineState.staticState.valid
        ? timelineState.staticState
        : null
    readonly property bool sourceAnimates: !!srcData
        && (srcData.cycle || 0) > 0
        && Math.max(1, srcData.div_x || 1) * Math.max(1, srcData.div_y || 1) > 4
    readonly property int spriteSkinTime: sourceAnimates ? skinTime : 0
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.dsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }
    readonly property var currentState: staticTimelineState
        || (timelineState.hasState ? timelineState.state : null)
    readonly property real segmentBaseX: currentState ? currentState.x || 0 : 0
    readonly property real segmentBaseY: currentState ? currentState.y || 0 : 0
    readonly property real segmentStepX: srcData ? srcData.op1 || 0 : 0
    readonly property real segmentStepY: srcData ? srcData.op2 || 0 : 0
    readonly property int side: srcData && srcData.side > 0 ? srcData.side : 1
    readonly property var player: screenRoot
        ? (screenRoot.gameplayPlayer(side)
            || (screenRoot.gameplayLanePlayer ? screenRoot.gameplayLanePlayer(side) : null))
        : null
    readonly property var score: player ? player.score : null
    readonly property real gaugeValue: screenRoot && score ? screenRoot.gameplayGaugeValue(score) : 0
    readonly property int hpSegments: Math.max(0, Math.min(50, Math.floor(gaugeValue / 2)))
    readonly property string activeGaugeName: screenRoot
        ? (side === 2 ? screenRoot.activeGaugeName2 : screenRoot.activeGaugeName1)
        : ""
    readonly property bool survivalGauge: screenRoot
        ? (side === 2 ? screenRoot.activeGaugeSurvival2 : screenRoot.activeGaugeSurvival1)
        : false
    readonly property int exGaugeFrameOffset: srcData
        && srcData.grooveGaugeEx
        && screenRoot
        && (side === 2 ? screenRoot.activeGaugeUsesExSprites2 : screenRoot.activeGaugeUsesExSprites1)
        ? 4
        : 0

    function segmentFrame(segmentIndex: var, filledSegments: var, survivalGauge: var) : var {
        let segmentNumber = segmentIndex + 1;
        let trembles = segmentNumber !== 1
            && segmentNumber < filledSegments
            && segmentNumber >= filledSegments - 2;
        if (!trembles) {
            if (survivalGauge || segmentNumber >= 40) {
                return segmentNumber <= filledSegments ? 0 : 2;
            }
            return segmentNumber <= filledSegments ? 1 : 3;
        }

        // Only the two segments immediately below the fill boundary can
        // tremble. Keep every other delegate independent of trembleRoll so a
        // 16 ms timer tick does not dirty all 50 sprite materials.
        let trembleBoundary = filledSegments - root.trembleRoll;
        if (!survivalGauge) {
            if (segmentNumber < 40) {
                return segmentNumber < trembleBoundary ? 1 : 3;
            }
            return segmentNumber < trembleBoundary ? 0 : 2;
        }
        return segmentNumber < trembleBoundary ? 0 : 2;
    }

    Timer {
        interval: 16
        repeat: true
        triggeredOnStart: true
        running: root.mediaActive && !!root.currentState && !!root.srcData
        onTriggered: {
            // DXLib GetRand(2) returns an integer in the inclusive 0..2 range.
            root.trembleRoll = Math.floor(Math.random() * 3);
        }
    }

    Repeater {
        model: root.srcData ? 50 : 0

        Lr2FastSprite {
            srcData: root.srcData
            stateData: root.currentState
            stateXOverride: root.segmentBaseX + index * root.segmentStepX
            stateYOverride: root.segmentBaseY + index * root.segmentStepY
            asynchronousLoading: !!root.screenRoot
                && root.screenRoot.customizeMode === true
            skinTime: root.spriteSkinTime
            timers: root.timers
            scaleOverride: root.scaleOverride
            preloadTexture: true
            useAtlasShader: true
            frameOverride: root.segmentFrame(
                index, root.hpSegments, root.survivalGauge)
                + root.exGaugeFrameOffset
        }
    }
}
