import QtQuick
import QtMultimedia
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

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
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property var chart
    property string chartAssetSource: ""
    property real scaleOverride: 1.0
    property real offsetX: 0
    property real offsetY: 0
    property bool colorKeyEnabled: false
    property color transColor: "black"
    property int frameOverride: -1
    property var stateOverride: null
    property Lr2TimelineState stateOverrideSource: null
    property bool sliderTranslationEnabled: false
    property real sliderPosition: 0
    property int sliderRange: 0
    property int sliderDirection: 0
    property bool dstOffsetsEnabled: false
    property real dstOffsetLiftY: 0
    property real dstOffsetLaneCoverY: 0
    property real dstOffsetHiddenY: 0
    property real dstOffsetHiddenA: 0
    property bool forceHidden: false
    property bool mediaActive: true
    property real scratchAngle1: 0
    property real scratchAngle2: 0
    property bool preferAtlasImagePath: false
    property bool sourceHasFrameAnimation: Lr2SkinUtils.sourceCyclesContinuously(srcData)

    readonly property bool hasFrameAnimation: sourceHasFrameAnimation
    readonly property int sourceTimeOffset: {
        if (!srcData || !dsts || dsts.length === 0 || !dsts[0]) {
            return 0;
        }
        return (srcData.timer || 0) === (dsts[0].timer || 0)
            ? Math.max(0, dsts[0].time || 0)
            : 0;
    }

    Lr2TimelineFrame {
        id: drawState
        dsts: root.dsts
        skinTime: root.skinTime
        skinClock: root.skinClock
        skinClockMode: root.skinClockMode
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
        timers: root.timers
        timerFire: root.timerFire
        stateOverride: root.stateOverride
        stateOverrideSource: root.stateOverrideSource
        forceHidden: root.forceHidden
        sliderTranslationEnabled: root.sliderTranslationEnabled
        sliderPosition: root.sliderPosition
        sliderRange: root.sliderRange
        sliderDirection: root.sliderDirection
        dstOffsetsEnabled: root.dstOffsetsEnabled
        dstOffsetLiftY: root.dstOffsetLiftY
        dstOffsetLaneCoverY: root.dstOffsetLaneCoverY
        dstOffsetHiddenY: root.dstOffsetHiddenY
        dstOffsetHiddenA: root.dstOffsetHiddenA
        colorKeyEnabled: root.colorKeyEnabled
        supportsInvertedBlend: true
    }

    readonly property real effectiveAngle: {
        if (!drawState.hasState) {
            return 0;
        }
        if (drawState.op4 === 1) {
            return root.scratchAngle1;
        }
        if (drawState.op4 === 2) {
            return root.scratchAngle2;
        }
        return drawState.angle;
    }
    readonly property int blendMode: drawState.blendMode
    readonly property bool hasColorTint: drawState.hasColorTint
    // Animated sheets and atlas-heavy screens can crop in the shader so rect
    // changes stay in uniforms. Ordinary static crops use Image.sourceClipRect
    // because Qt also handles edge clipping there.
    readonly property bool shouldAnimateInAtlasShader: root.hasFrameAnimation
    readonly property color tintColor: drawState.tintColor
    readonly property bool useFastImagePath: root.hasDrawableTexture
        && !root.preferAtlasImagePath
        && root.hasCroppedTextureSource
        && !root.shouldAnimateInAtlasShader
        && root.blendMode === 1
        && !root.colorKeyEnabled
        && !root.hasColorTint
    readonly property bool usesScratchRotation: drawState.hasState
        && (drawState.op4 === 1 || drawState.op4 === 2)
    readonly property int effectiveCenter: drawState.hasState ? drawState.center : 0
    readonly property real anchorX: {
        if (root.usesScratchRotation) {
            return 0.5;
        }
        switch (root.effectiveCenter) {
        case 1:
        case 4:
        case 7:
            return 0.0;
        case 3:
        case 6:
        case 9:
            return 1.0;
        default:
            return 0.5;
        }
    }
    readonly property real anchorY: {
        if (root.usesScratchRotation) {
            return 0.5;
        }
        switch (root.effectiveCenter) {
        case 1:
        case 2:
        case 3:
            return 1.0;
        case 7:
        case 8:
        case 9:
            return 0.0;
        default:
            return 0.5;
        }
    }
    readonly property bool isSolidFill: !!root.srcData && root.srcData.specialType === 2
    readonly property bool sourceIsChartAsset: Lr2SkinUtils.isChartAssetSource(root.srcData)
    readonly property bool hasWholeTextureSource: !!root.srcData && !root.isSolidFill
        && (root.srcData.x < 0 || root.srcData.y < 0 || root.srcData.w < 0 || root.srcData.h < 0)
    readonly property bool hasCroppedTextureSource: !!root.srcData && !root.isSolidFill
        && root.srcData.w > 0 && root.srcData.h > 0
    readonly property bool isVideoSource: /\.(mpg|mpeg|mp4|avi|wmv|mov|mkv)$/i.test(root.resolvedSource)
    readonly property bool hasDrawableVideo: root.isVideoSource && root.resolvedSource !== ""
        && (root.hasWholeTextureSource || root.hasCroppedTextureSource)
    readonly property bool hasDrawableTexture: !root.isVideoSource && root.resolvedSource !== ""
        && (root.hasWholeTextureSource || root.hasCroppedTextureSource)
    readonly property bool hasRenderableState: drawState.hasState
        && (drawState.blend === 0 ? 255 : drawState.a) > 0
        && drawState.w !== 0
        && drawState.h !== 0
    readonly property bool shouldPlayVideo: root.mediaActive
        && root.visible
        && root.opacity > 0
        && root.width > 0
        && root.height > 0
        && root.hasDrawableVideo
        && root.hasRenderableState
    property bool videoReloadPending: false

    function syncVideoPlayback() : void {
        if (videoLoader.item && videoLoader.item.syncVideoPlayback) {
            videoLoader.item.syncVideoPlayback();
        }
    }

    function reloadVideoPlayback() : void {
        if (videoReloadPending || !shouldPlayVideo) {
            return;
        }

        videoReloadPending = true;
        Qt.callLater(function() {
            videoReloadPending = false;
            syncVideoPlayback();
        });
    }

    onShouldPlayVideoChanged: syncVideoPlayback()
    onResolvedSourceChanged: {
        videoReloadPending = false;
        syncVideoPlayback();
    }
    Component.onCompleted: syncVideoPlayback()
    Component.onDestruction: {
        if (videoLoader.item && videoLoader.item.stopVideo) {
            videoLoader.item.stopVideo();
        }
    }

    readonly property string resolvedSource: Lr2SkinUtils.resolvedSource(
        srcData,
        chart,
        chartAssetSource)

    property Lr2AnimationFrameState animationFrameState: Lr2AnimationFrameState {
        enabled: (root.hasFrameAnimation || root.frameOverride >= 0)
            && (root.hasDrawableTexture || root.hasDrawableVideo)
            && root.hasRenderableState
        skinClock: root.skinClock
        clockMode: root.hasFrameAnimation ? root.sourceSkinClockMode : 0
        sourceData: root.srcData
        skinTime: root.sourceSkinTime
        timers: root.timers
        timerFire: root.sourceTimerFire
        sourceTimeOffset: root.sourceTimeOffset
        frameOverride: root.frameOverride
        textureWidth: Math.max(0, root.useFastImagePath ? 0 : atlasImage.implicitWidth)
        textureHeight: Math.max(0, root.useFastImagePath ? 0 : atlasImage.implicitHeight)
    }

    Item {
        id: sprite
        x: drawState.hasState
            ? (drawState.x + (drawState.w < 0 ? drawState.w : 0) + root.offsetX) * root.scaleOverride
            : 0
        y: drawState.hasState
            ? (drawState.y + (drawState.h < 0 ? drawState.h : 0) + root.offsetY) * root.scaleOverride
            : 0
        width: drawState.hasState
            ? (drawState.w < 0 ? -drawState.w : drawState.w) * root.scaleOverride
            : 0
        height: drawState.hasState
            ? (drawState.h < 0 ? -drawState.h : drawState.h) * root.scaleOverride
            : 0
        visible: root.hasRenderableState
        opacity: drawState.hasState ? (drawState.blend === 0 ? 1.0 : drawState.a / 255.0) : 0

        transform: Rotation {
            origin.x: sprite.width * root.anchorX
            origin.y: sprite.height * root.anchorY
            angle: root.effectiveAngle
        }

        Loader {
            id: videoLoader
            anchors.fill: parent
            active: root.shouldPlayVideo && !root.videoReloadPending
            sourceComponent: videoComponent
            onLoaded: root.syncVideoPlayback()
        }

        Component {
            id: videoComponent

            Item {
                anchors.fill: parent
                property real playbackStartWallMs: 0
                property int lastVideoPosition: -1
                property bool sawPositionAdvance: false
                readonly property int manualLoopLeadMs: 80
                readonly property int manualLoopGraceMs: 120

                function nowMs() : var {
                    return Date.now();
                }

                function notePlaybackStarted() : void {
                    playbackStartWallMs = nowMs();
                    lastVideoPosition = -1;
                    sawPositionAdvance = false;
                }

                function syncVideoPlayback() : void {
                    if (root.shouldPlayVideo) {
                        if (videoPlayer.playbackState !== MediaPlayer.PlayingState) {
                            videoPlayer.play();
                        } else if (playbackStartWallMs <= 0) {
                            notePlaybackStarted();
                        }
                    } else {
                        playbackStartWallMs = 0;
                        videoPlayer.stop();
                    }
                }

                function loopVideoFromEnd() : void {
                    if (!root.shouldPlayVideo || root.resolvedSource === "") {
                        return;
                    }

                    root.reloadVideoPlayback();
                }

                function checkManualLoop() : void {
                    if (!root.shouldPlayVideo || playbackStartWallMs <= 0 || videoPlayer.duration <= 0) {
                        return;
                    }

                    const elapsed = nowMs() - playbackStartWallMs;
                    const restartAt = sawPositionAdvance
                        ? videoPlayer.duration + manualLoopGraceMs
                        : Math.max(1, videoPlayer.duration - manualLoopLeadMs);
                    if (elapsed >= restartAt) {
                        root.reloadVideoPlayback();
                    }
                }

                function stopVideo() : void {
                    playbackStartWallMs = 0;
                    videoPlayer.stop();
                    videoPlayer.videoOutput = null;
                    videoPlayer.source = "";
                }

                VideoOutput {
                    id: videoOutput
                    anchors.fill: parent
                    fillMode: VideoOutput.Stretch
                }

                MediaPlayer {
                    id: videoPlayer
                    source: root.shouldPlayVideo ? root.resolvedSource : ""
                    videoOutput: videoOutput
                    loops: MediaPlayer.Infinite
                    onMediaStatusChanged: {
                        if (mediaStatus === MediaPlayer.EndOfMedia) {
                            loopVideoFromEnd();
                        }
                    }
                    onPlaybackStateChanged: {
                        if (playbackState === MediaPlayer.StoppedState
                                && mediaStatus === MediaPlayer.EndOfMedia) {
                            loopVideoFromEnd();
                        } else if (playbackState === MediaPlayer.PlayingState) {
                            notePlaybackStarted();
                        } else if (playbackState === MediaPlayer.StoppedState) {
                            playbackStartWallMs = 0;
                            if (root.shouldPlayVideo
                                    && mediaStatus !== MediaPlayer.NoMedia
                                    && mediaStatus !== MediaPlayer.InvalidMedia) {
                                root.reloadVideoPlayback();
                            }
                        }
                    }
                    onPositionChanged: (position) => {
                        if (lastVideoPosition >= 1000 && position < 500) {
                            notePlaybackStarted();
                        } else if (lastVideoPosition >= 0 && position > lastVideoPosition + 50) {
                            sawPositionAdvance = true;
                        }
                        lastVideoPosition = position;
                    }
                }

                Timer {
                    interval: 100
                    repeat: true
                    running: root.shouldPlayVideo
                        && videoPlayer.playbackState === MediaPlayer.PlayingState
                        && videoPlayer.duration > 0
                    onTriggered: checkManualLoop()
                }

                Component.onCompleted: syncVideoPlayback()
                Component.onDestruction: stopVideo()
            }
        }

        Image {
            id: fastImage
            anchors.fill: parent
            source: root.hasDrawableTexture && root.useFastImagePath ? root.resolvedSource : ""
            sourceClipRect: root.animationFrameState.sourceClipRect
            fillMode: Image.Stretch
            cache: true
            asynchronous: root.sourceIsChartAsset
            retainWhileLoading: true
            smooth: drawState.hasState && drawState.filter !== 0
            mipmap: false
            visible: root.useFastImagePath && status === Image.Ready
        }

        Image {
            id: atlasImage
            source: root.hasDrawableTexture && !root.useFastImagePath ? root.resolvedSource : ""
            cache: true
            asynchronous: root.sourceIsChartAsset
            retainWhileLoading: true
            smooth: drawState.hasState && drawState.filter !== 0
            mipmap: false
            visible: false
        }

        ShaderEffect {
            anchors.fill: parent
            visible: root.hasDrawableTexture && !root.useFastImagePath && atlasImage.status === Image.Ready
            blending: true
            supportsAtlasTextures: true
            property var source: atlasImage
            property color tint: root.tintColor
            property color transColor: root.transColor
            property real blendMode: root.blendMode
            property real colorKeyEnabled: root.colorKeyEnabled ? 1.0 : 0.0
            property real tolerance: 0.001
            property real nearestMode: drawState.hasState && drawState.filter === 0 ? 1.0 : 0.0
            property vector2d sourceSize: Qt.vector2d(
                Math.max(1, atlasImage.implicitWidth),
                Math.max(1, atlasImage.implicitHeight))
            property vector4d sourceRect: root.animationFrameState.sourceRect
            fragmentShader: "qrc:/Lr2/Lr2SpriteAtlas.frag.qsb"
        }

        Rectangle {
            visible: root.isSolidFill || (!!root.srcData && root.srcData.specialType === 5)
            anchors.fill: parent
            color: root.srcData && root.srcData.specialType === 5 ? "white" : "black"
        }
    }
}
