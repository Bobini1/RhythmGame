import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

QtObject {
    id: frame

    property var dsts: []
    property var srcData: null
    property int skinTime: 0
    property var skinClock: null
    property int skinClockMode: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648

    property var stateOverride: null
    property bool forceHidden: false
    property var screenRoot: null
    property bool screenDstOffsetsEnabled: false

    property bool sliderTranslationEnabled: false
    property real sliderPosition: 0
    property int sliderRange: 0
    property int sliderDirection: 0

    property bool dstOffsetsEnabled: false
    property real dstOffsetLiftY: 0
    property real dstOffsetLaneCoverY: 0
    property real dstOffsetHiddenY: 0
    property real dstOffsetHiddenA: 0

    property bool colorKeyEnabled: false
    property bool supportsInvertedBlend: true

    readonly property bool canUseStaticState: !stateOverride
        && !sliderTranslationEnabled
        && !dstOffsetsEnabled
        && !forceHidden
        && timelineState.canUseStaticState
    readonly property var staticState: canUseStaticState ? timelineState.staticState : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null

    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !frame.stateOverride && !frame.forceHidden && !frame.canUseStaticState
        skinClock: frame.skinClock
        clockMode: frame.skinClockMode
        dsts: frame.dsts
        skinTime: frame.skinTime
        timers: frame.timelineTimers
        timerFire: frame.timerFire
        activeOptionsState: frame.activeOptionsState
        activeOptions: frame.activeOptions
        sliderTranslationEnabled: frame.sliderTranslationEnabled
        sliderPosition: frame.sliderPosition
        sliderRange: frame.sliderRange
        sliderDirection: frame.sliderDirection
        dstOffsetsEnabled: frame.dstOffsetsEnabled
        dstOffsetLiftY: frame.dstOffsetLiftY
        dstOffsetLaneCoverY: frame.dstOffsetLaneCoverY
        dstOffsetHiddenY: frame.dstOffsetHiddenY
        dstOffsetHiddenA: frame.dstOffsetHiddenA
    }

    readonly property var directState: forceHidden ? null : (stateOverride || staticState)
    readonly property bool hasDirectState: !!directState
    readonly property bool hasTimelineState: !forceHidden && timelineState.hasState
    readonly property bool hasScreenDstOffsets: screenDstOffsetsEnabled
        && !!screenRoot
        && !!screenRoot.applyLr2DstOffsets
        && dsts
        && dsts.length > 0
        && dsts[0]
        && dsts[0].offsets
        && dsts[0].offsets.length > 0
    readonly property var stateBeforeScreenOffsets: hasScreenDstOffsets
        ? (directState || (hasTimelineState ? timelineState.state : null))
        : null
    readonly property var state: hasScreenDstOffsets
        ? screenRoot.applyLr2DstOffsets(
            stateBeforeScreenOffsets,
            dsts,
            srcData ? srcData.side || 0 : 0)
        : directState
    readonly property bool hasState: hasScreenDstOffsets ? !!state : (hasDirectState || hasTimelineState)

    readonly property real x: state ? (state.x || 0) : (hasTimelineState ? timelineState.stateX : 0)
    readonly property real y: state ? (state.y || 0) : (hasTimelineState ? timelineState.stateY : 0)
    readonly property real w: state ? (state.w || 0) : (hasTimelineState ? timelineState.stateW : 0)
    readonly property real h: state ? (state.h || 0) : (hasTimelineState ? timelineState.stateH : 0)
    readonly property real a: state
        ? Lr2SkinUtils.stateValue(state, "a", 255)
        : (hasTimelineState ? timelineState.stateA : 0)
    readonly property real r: state
        ? Lr2SkinUtils.stateValue(state, "r", 255)
        : (hasTimelineState ? timelineState.stateR : 255)
    readonly property real g: state
        ? Lr2SkinUtils.stateValue(state, "g", 255)
        : (hasTimelineState ? timelineState.stateG : 255)
    readonly property real b: state
        ? Lr2SkinUtils.stateValue(state, "b", 255)
        : (hasTimelineState ? timelineState.stateB : 255)
    readonly property real angle: state ? (state.angle || 0) : (hasTimelineState ? timelineState.stateAngle : 0)
    readonly property int center: state ? (state.center || 0) : (hasTimelineState ? timelineState.stateCenter : 0)
    readonly property int blend: state ? (state.blend || 0) : (hasTimelineState ? timelineState.stateBlend : 0)
    readonly property int filter: state ? (state.filter || 0) : (hasTimelineState ? timelineState.stateFilter : 0)
    readonly property int op4: state ? (state.op4 || 0) : (hasTimelineState ? timelineState.stateOp4 : 0)

    readonly property int blendMode: Lr2SkinUtils.normalizedBlendMode(
        hasState ? blend : 1,
        colorKeyEnabled,
        supportsInvertedBlend)
    readonly property real tintR: hasState ? Lr2SkinUtils.colorComponent(r) : 1.0
    readonly property real tintG: hasState ? Lr2SkinUtils.colorComponent(g) : 1.0
    readonly property real tintB: hasState ? Lr2SkinUtils.colorComponent(b) : 1.0
    readonly property bool hasColorTint: Math.abs(tintR - 1.0) > 0.001
        || Math.abs(tintG - 1.0) > 0.001
        || Math.abs(tintB - 1.0) > 0.001
    readonly property color tintColor: Qt.rgba(tintR, tintG, tintB, 1.0)
    readonly property real opacity: hasState ? a / 255.0 : 0
}
