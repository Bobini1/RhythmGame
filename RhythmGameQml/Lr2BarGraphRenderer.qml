import QtQuick
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property int sourceSkinTime: skinTime
    property var skinClock: null
    property int skinClockMode: 0
    property int sourceSkinClockMode: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property var chart
    property real scaleOverride: 1.0
    property real value: 0
    property bool animateValue: false
    property int valueAnimationDuration: 100
    property bool colorKeyEnabled: false
    property color transColor: "black"

    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        skinClock: root.skinClock
        clockMode: root.skinClockMode
        dsts: root.dsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptions: root.timelineActiveOptions
    }
    readonly property var objectState: root.staticTimelineState
    readonly property bool hasCurrentState: !!objectState || root.timelineState.hasState
    readonly property real stateX: objectState ? (objectState.x || 0) : (timelineState.hasState ? timelineState.stateX : 0)
    readonly property real stateY: objectState ? (objectState.y || 0) : (timelineState.hasState ? timelineState.stateY : 0)
    readonly property real stateW: objectState ? (objectState.w || 0) : (timelineState.hasState ? timelineState.stateW : 0)
    readonly property real stateH: objectState ? (objectState.h || 0) : (timelineState.hasState ? timelineState.stateH : 0)
    readonly property real stateA: objectState ? (objectState.a === undefined ? 255 : objectState.a) : (timelineState.hasState ? timelineState.stateA : 0)
    readonly property real stateR: objectState ? (objectState.r === undefined ? 255 : objectState.r) : (timelineState.hasState ? timelineState.stateR : 255)
    readonly property real stateG: objectState ? (objectState.g === undefined ? 255 : objectState.g) : (timelineState.hasState ? timelineState.stateG : 255)
    readonly property real stateB: objectState ? (objectState.b === undefined ? 255 : objectState.b) : (timelineState.hasState ? timelineState.stateB : 255)
    readonly property int stateBlend: objectState ? (objectState.blend || 0) : (timelineState.hasState ? timelineState.stateBlend : 0)
    readonly property int stateFilter: objectState ? (objectState.filter || 0) : (timelineState.hasState ? timelineState.stateFilter : 0)
    readonly property real clampedValue: Math.max(0, Math.min(1, value))
    property real displayedValue: clampedValue
    readonly property int blendMode: {
        let raw = hasCurrentState ? stateBlend : 1;
        if (raw === 0 && !root.colorKeyEnabled) return 1;
        if (raw === 5 || raw === 6) return 2;
        if (raw === 3 || raw === 4 || raw === 9 || raw === 11) return 1;
        return raw;
    }
    function colorComponent(value) {
        if (value === undefined || value === null) return 1.0;
        return Math.max(0, Math.min(255, value)) / 255.0;
    }
    readonly property real tintR: root.hasCurrentState ? root.colorComponent(root.stateR) : 1.0
    readonly property real tintG: root.hasCurrentState ? root.colorComponent(root.stateG) : 1.0
    readonly property real tintB: root.hasCurrentState ? root.colorComponent(root.stateB) : 1.0
    readonly property color tintColor: Qt.rgba(root.tintR, root.tintG, root.tintB, 1.0)
    readonly property bool hasFrameAnimation: srcData
        && (srcData.cycle || 0) > 0
        && Math.max(1, srcData.div_x || 1) * Math.max(1, srcData.div_y || 1) > 1
    property Lr2AnimationFrameState animationFrameState: Lr2AnimationFrameState {
        enabled: root.hasFrameAnimation
        skinClock: root.skinClock
        clockMode: root.hasFrameAnimation ? root.sourceSkinClockMode : 0
        sourceData: root.srcData
        skinTime: root.sourceSkinTime
        timers: root.timers
        timerFire: root.sourceTimerFire
        textureWidth: Math.max(0, graphAtlas.implicitWidth)
        textureHeight: Math.max(0, graphAtlas.implicitHeight)
    }
    readonly property string resolvedSource: {
        if (!srcData) return "";
        if (srcData.specialType === 1 || srcData.specialType === 3 || srcData.specialType === 4) {
            let chartData = root.chart ? (root.chart.chartData || root.chart) : null;
            let fileName = "";
            if (srcData.specialType === 1) {
                fileName = chartData ? chartData.stageFile : "";
            } else if (srcData.specialType === 3) {
                fileName = chartData ? chartData.backBmp : "";
            } else {
                fileName = chartData ? chartData.banner : "";
            }
            if (!chartData || !fileName || !chartData.chartDirectory) {
                return "";
            }
            let dir = chartData.chartDirectory;
            if (dir[0] !== "/") {
                dir = "/" + dir;
            }
            return "file://" + dir + fileName.replace(/\.[^/.]+$/, "");
        }
        if (!srcData.source) return "";
        let absPath = srcData.source.replace(/\\/g, "/");
        if (/^[A-Za-z]:\//.test(absPath)) {
            return "file:///" + absPath;
        }
        if (absPath.startsWith("/")) {
            return "file://" + absPath;
        }
        return absPath;
    }

    onClampedValueChanged: displayedValue = clampedValue

    Behavior on displayedValue {
        enabled: root.animateValue
        NumberAnimation {
            duration: root.valueAnimationDuration
            easing.type: Easing.Linear
        }
    }

    Item {
        id: graph
        readonly property real signedW: root.hasCurrentState ? root.stateW * root.scaleOverride : 0
        readonly property real signedH: root.hasCurrentState ? root.stateH * root.scaleOverride : 0
        readonly property real fullW: Math.abs(signedW)
        readonly property real fullH: Math.abs(signedH)
        readonly property bool horizontal: root.srcData ? root.srcData.direction === 0 : true
        readonly property real drawnW: horizontal ? fullW * root.displayedValue : fullW
        readonly property real drawnH: horizontal ? fullH : fullH * root.displayedValue

        x: root.hasCurrentState
            ? root.stateX * root.scaleOverride
                + (horizontal && signedW < 0 ? signedW * root.displayedValue : 0)
            : 0
        y: root.hasCurrentState
            ? root.stateY * root.scaleOverride
                + (!horizontal && signedH < 0 ? signedH * root.displayedValue : 0)
            : 0
        width: drawnW
        height: drawnH
        visible: root.hasCurrentState && root.stateA > 0 && width > 0 && height > 0 && root.resolvedSource !== ""
        opacity: root.hasCurrentState ? root.stateA / 255.0 : 0

        Image {
            id: graphAtlas
            source: root.resolvedSource
            asynchronous: root.srcData
                && (root.srcData.specialType === 1
                    || root.srcData.specialType === 3
                    || root.srcData.specialType === 4)
            cache: true
            smooth: false
            mipmap: false
            visible: false
        }

        ShaderEffect {
            anchors.fill: parent
            visible: graphAtlas.status === Image.Ready
            blending: true
            property var source: graphAtlas
            property color tint: root.tintColor
            property color transColor: root.transColor
            property real blendMode: root.blendMode
            property real colorKeyEnabled: root.blendMode === 0 ? 1.0 : 0.0
            property real tolerance: 0.03125
            property real nearestMode: root.hasCurrentState && root.stateFilter === 0 ? 1.0 : 0.0
            property vector2d sourceSize: Qt.vector2d(
                Math.max(1, graphAtlas.implicitWidth),
                Math.max(1, graphAtlas.implicitHeight))
            property vector4d sourceRect: root.animationFrameState.sourceRect
            fragmentShader: "qrc:/Lr2SpriteAtlas.frag.qsb"
        }
    }
}
