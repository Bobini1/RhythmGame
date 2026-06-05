import QtQuick
import RhythmGameQml

QtObject {
    id: frame

    property var dsts: []
    property int skinTime: 0
    property var skinClock: null
    property int skinClockMode: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648

    property var stateOverride: null
    property bool forceHidden: false

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
    readonly property var staticState: canUseStaticState && timelineState.staticState.valid
        ? timelineState.staticState
        : null
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
    readonly property bool hasTimelineState: !forceHidden && !hasDirectState && timelineState.hasState
    readonly property var state: directState
    readonly property bool hasState: hasDirectState || hasTimelineState

    readonly property real x: state ? (state.x || 0) : (hasTimelineState ? timelineState.stateX : 0)
    readonly property real y: state ? (state.y || 0) : (hasTimelineState ? timelineState.stateY : 0)
    readonly property real w: state ? (state.w || 0) : (hasTimelineState ? timelineState.stateW : 0)
    readonly property real h: state ? (state.h || 0) : (hasTimelineState ? timelineState.stateH : 0)
    readonly property real a: state
        ? (state.a === undefined || state.a === null ? 255 : state.a)
        : (hasTimelineState ? timelineState.stateA : 0)
    readonly property real r: state
        ? (state.r === undefined || state.r === null ? 255 : state.r)
        : (hasTimelineState ? timelineState.stateR : 255)
    readonly property real g: state
        ? (state.g === undefined || state.g === null ? 255 : state.g)
        : (hasTimelineState ? timelineState.stateG : 255)
    readonly property real b: state
        ? (state.b === undefined || state.b === null ? 255 : state.b)
        : (hasTimelineState ? timelineState.stateB : 255)
    readonly property real angle: state ? (state.angle || 0) : (hasTimelineState ? timelineState.stateAngle : 0)
    readonly property int center: state ? (state.center || 0) : (hasTimelineState ? timelineState.stateCenter : 0)
    readonly property int blend: state ? (state.blend || 0) : (hasTimelineState ? timelineState.stateBlend : 0)
    readonly property int filter: state ? (state.filter || 0) : (hasTimelineState ? timelineState.stateFilter : 0)
    readonly property int op4: state ? (state.op4 || 0) : (hasTimelineState ? timelineState.stateOp4 : 0)

    readonly property int rawBlendMode: hasState ? blend : 1
    readonly property int blendMode: {
        if (rawBlendMode === 0 && !colorKeyEnabled) {
            return 1;
        }
        if (rawBlendMode === 5 || rawBlendMode === 6) {
            return 2;
        }
        if (rawBlendMode === 10 && !supportsInvertedBlend) {
            return 1;
        }
        if (rawBlendMode === 3
                || rawBlendMode === 4
                || rawBlendMode === 9
                || rawBlendMode === 11) {
            return 1;
        }
        return rawBlendMode;
    }
    readonly property real tintR: hasState ? Math.max(0, Math.min(255, r)) / 255.0 : 1.0
    readonly property real tintG: hasState ? Math.max(0, Math.min(255, g)) / 255.0 : 1.0
    readonly property real tintB: hasState ? Math.max(0, Math.min(255, b)) / 255.0 : 1.0
    readonly property bool hasColorTint: Math.abs(tintR - 1.0) > 0.001
        || Math.abs(tintG - 1.0) > 0.001
        || Math.abs(tintB - 1.0) > 0.001
    readonly property color tintColor: Qt.rgba(tintR, tintG, tintB, 1.0)
    readonly property real opacity: hasState ? a / 255.0 : 0
}
