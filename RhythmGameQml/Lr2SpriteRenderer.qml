import QtQuick
import QtMultimedia
import RhythmGameQml 1.0

import "Lr2Timeline.js" as Lr2Timeline

// The root Item is sized by its parent (Loader / container) to the skin
// canvas. An inner `sprite` Item does the real animated positioning and
// sizing, so that the parent container can't override our bindings (which
// is exactly what QQuickLoader does when it has an explicit size).
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
    property real offsetX: 0
    property real offsetY: 0
    property bool colorKeyEnabled: false
    property color transColor: "black"
    property int frameOverride: -1
    property var stateOverride: null
    property bool forceHidden: false
    property bool mediaActive: true
    property real scratchAngle1: 0
    property real scratchAngle2: 0

    readonly property bool hasFrameAnimation: srcData
        && (srcData.cycle || 0) > 0
        && Math.max(1, srcData.div_x || 1) * Math.max(1, srcData.div_y || 1) > 1
    readonly property bool hasStaticTimelineState: !stateOverride
        && !forceHidden
        && Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.stateOverride && !root.forceHidden && !root.hasStaticTimelineState
        skinClock: root.skinClock
        clockMode: root.skinClockMode
        dsts: root.dsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptions: root.timelineActiveOptions
    }
    readonly property var objectState: forceHidden ? null : (stateOverride || staticTimelineState)
    readonly property bool hasCurrentState: !!objectState || (!forceHidden && timelineState.hasState)
    readonly property real stateX: objectState ? (objectState.x || 0) : (timelineState.hasState ? timelineState.stateX : 0)
    readonly property real stateY: objectState ? (objectState.y || 0) : (timelineState.hasState ? timelineState.stateY : 0)
    readonly property real stateW: objectState ? (objectState.w || 0) : (timelineState.hasState ? timelineState.stateW : 0)
    readonly property real stateH: objectState ? (objectState.h || 0) : (timelineState.hasState ? timelineState.stateH : 0)
    readonly property real stateA: objectState ? (objectState.a === undefined ? 255 : objectState.a) : (timelineState.hasState ? timelineState.stateA : 0)
    readonly property real stateR: objectState ? (objectState.r === undefined ? 255 : objectState.r) : (timelineState.hasState ? timelineState.stateR : 255)
    readonly property real stateG: objectState ? (objectState.g === undefined ? 255 : objectState.g) : (timelineState.hasState ? timelineState.stateG : 255)
    readonly property real stateB: objectState ? (objectState.b === undefined ? 255 : objectState.b) : (timelineState.hasState ? timelineState.stateB : 255)
    readonly property real stateAngle: objectState ? (objectState.angle || 0) : (timelineState.hasState ? timelineState.stateAngle : 0)
    readonly property int stateCenter: objectState ? (objectState.center || 0) : (timelineState.hasState ? timelineState.stateCenter : 0)
    readonly property int stateBlend: objectState ? (objectState.blend || 0) : (timelineState.hasState ? timelineState.stateBlend : 0)
    readonly property int stateFilter: objectState ? (objectState.filter || 0) : (timelineState.hasState ? timelineState.stateFilter : 0)
    readonly property int stateOp4: objectState ? (objectState.op4 || 0) : (timelineState.hasState ? timelineState.stateOp4 : 0)
    readonly property real drawX: root.stateX + (root.stateW < 0 ? root.stateW : 0) + root.offsetX
    readonly property real drawY: root.stateY + (root.stateH < 0 ? root.stateH : 0) + root.offsetY
    readonly property real drawW: Math.abs(root.stateW)
    readonly property real drawH: Math.abs(root.stateH)
    readonly property real effectiveAngle: {
        if (!hasCurrentState) {
            return 0;
        }
        if (root.stateOp4 === 1) {
            return scratchAngle1;
        }
        if (root.stateOp4 === 2) {
            return scratchAngle2;
        }
        return root.stateAngle;
    }
    // LR2 blend modes we can express via Qt Quick primitives:
    //   0 = TRANSCOLOR alpha (black -> transparent, then alpha)
    //   1 = plain alpha
    //   2 = ADD, implemented via Lr2AddBlend.frag + premult-alpha trick
    //   5 = ADD (undocumented LR2 alias seen in existing skins)
    //   6 = ADD(XOR) — visually identical to ADD on the opaque surfaces LR2
    //       skins actually composite onto, so we alias it to 2.
    //   10 = INVSRC / ANTI_COLOR, implemented as an inverted source alpha blend
    // Modes 3 (SUB), 4 (MULT), 9, 11 (MULT*ALPHA) all require
    // custom glBlendFunc state (e.g. GL_FUNC_REVERSE_SUBTRACT, GL_DST_COLOR)
    // which Qt Quick's default ShaderEffect pipeline can't set. Implementing
    // them properly needs a C++ QSGMaterial subclass that overrides
    // updatePipelineState(). Until that lands, fall through to plain alpha.
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
    readonly property bool hasColorTint: Math.abs(root.tintR - 1.0) > 0.001
        || Math.abs(root.tintG - 1.0) > 0.001
        || Math.abs(root.tintB - 1.0) > 0.001
    readonly property color tintColor: Qt.rgba(root.tintR, root.tintG, root.tintB, 1.0)
    readonly property bool usesScratchRotation: hasCurrentState
        && (root.stateOp4 === 1 || root.stateOp4 === 2)
    readonly property var anchor: root.usesScratchRotation
        ? ({ x: 0.5, y: 0.5 })
        : Lr2Timeline.centerAnchor(root.hasCurrentState ? root.stateCenter : 0)
    readonly property bool isSolidFill: srcData && srcData.specialType === 2
    readonly property bool hasWholeTextureSource: srcData && !root.isSolidFill
        && (srcData.x < 0 || srcData.y < 0 || srcData.w < 0 || srcData.h < 0)
    readonly property bool hasCroppedTextureSource: srcData && !root.isSolidFill && srcData.w > 0 && srcData.h > 0
    readonly property bool isVideoSource: /\.(mpg|mpeg|mp4|avi|wmv|mov|mkv)$/i.test(root.resolvedSource)
    readonly property bool hasDrawableVideo: root.isVideoSource && root.resolvedSource !== ""
        && (root.hasWholeTextureSource || root.hasCroppedTextureSource)
    readonly property bool hasDrawableTexture: !root.isVideoSource && root.resolvedSource !== ""
        && (root.hasWholeTextureSource || root.hasCroppedTextureSource)
    readonly property bool shouldPlayVideo: root.mediaActive
        && root.hasDrawableVideo
        && root.hasCurrentState
        && root.stateA > 0
        && root.drawW > 0
        && root.drawH > 0

    function syncVideoPlayback() {
        if (root.shouldPlayVideo) {
            if (videoPlayer.playbackState !== MediaPlayer.PlayingState) {
                videoPlayer.play();
            }
        } else {
            videoPlayer.stop();
        }
    }

    onShouldPlayVideoChanged: syncVideoPlayback()
    onResolvedSourceChanged: syncVideoPlayback()
    Component.onCompleted: syncVideoPlayback()
    Component.onDestruction: videoPlayer.stop()

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

    property Lr2AnimationFrameState animationFrameState: Lr2AnimationFrameState {
        enabled: root.hasFrameAnimation || root.frameOverride >= 0
        skinClock: root.skinClock
        clockMode: root.hasFrameAnimation ? root.sourceSkinClockMode : 0
        sourceData: root.srcData
        skinTime: root.sourceSkinTime
        timers: root.timers
        timerFire: root.sourceTimerFire
        frameOverride: root.frameOverride
        textureWidth: Math.max(0, atlasImage.implicitWidth)
        textureHeight: Math.max(0, atlasImage.implicitHeight)
    }

    Item {
        id: sprite
        x: root.hasCurrentState ? root.drawX * root.scaleOverride : 0
        y: root.hasCurrentState ? root.drawY * root.scaleOverride : 0
        width: root.hasCurrentState ? root.drawW * root.scaleOverride : 0
        height: root.hasCurrentState ? root.drawH * root.scaleOverride : 0
        visible: root.hasCurrentState && root.stateA > 0 && width > 0 && height > 0
        opacity: root.hasCurrentState ? root.stateA / 255.0 : 0

        transform: Rotation {
            origin.x: sprite.width * root.anchor.x
            origin.y: sprite.height * root.anchor.y
            angle: root.effectiveAngle
        }

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            visible: root.hasDrawableVideo
            fillMode: VideoOutput.Stretch
        }

        AudioOutput {
            id: videoAudio
            muted: true
            volume: 0
        }

        MediaPlayer {
            id: videoPlayer
            source: root.hasDrawableVideo ? root.resolvedSource : ""
            videoOutput: videoOutput
            audioOutput: videoAudio
            loops: MediaPlayer.Infinite
        }

        Image {
            id: atlasImage
            source: root.hasDrawableTexture ? root.resolvedSource : ""
            cache: true
            asynchronous: root.srcData
                && (root.srcData.specialType === 1
                    || root.srcData.specialType === 3
                    || root.srcData.specialType === 4)
            visible: false
        }

        ShaderEffect {
            anchors.fill: parent
            visible: root.hasDrawableTexture && atlasImage.status === Image.Ready
            blending: true
            property var source: atlasImage
            property color tint: root.tintColor
            property color transColor: root.transColor
            property real blendMode: root.blendMode
            property real colorKeyEnabled: root.blendMode === 0 ? 1.0 : 0.0
            property real tolerance: 0.03125
            property real nearestMode: root.hasCurrentState && root.stateFilter === 0 ? 1.0 : 0.0
            property vector2d sourceSize: Qt.vector2d(
                Math.max(1, atlasImage.implicitWidth),
                Math.max(1, atlasImage.implicitHeight))
            property vector4d sourceRect: root.animationFrameState.sourceRect
            fragmentShader: "qrc:/Lr2SpriteAtlas.frag.qsb"
        }

        Rectangle {
            visible: root.isSolidFill || (srcData && srcData.specialType === 5)
            anchors.fill: parent
            color: srcData && srcData.specialType === 5 ? "white" : "black"
        }
    }
}
