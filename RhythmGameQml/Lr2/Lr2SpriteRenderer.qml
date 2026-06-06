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

    readonly property bool hasCurrentState: drawState.hasState
    readonly property real stateX: drawState.x
    readonly property real stateY: drawState.y
    readonly property real stateW: drawState.w
    readonly property real stateH: drawState.h
    readonly property real stateA: drawState.a
    readonly property real stateR: drawState.r
    readonly property real stateG: drawState.g
    readonly property real stateB: drawState.b
    readonly property real stateAngle: drawState.angle
    readonly property int stateCenter: drawState.center
    readonly property int stateBlend: drawState.blend
    readonly property int stateFilter: drawState.filter
    readonly property int stateOp4: drawState.op4
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
    readonly property int blendMode: drawState.blendMode
    readonly property bool hasColorTint: drawState.hasColorTint
    readonly property real effectiveAlpha: root.hasCurrentState && root.stateBlend === 0
        ? 255
        : root.stateA
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
        && !root.hasColorTint
    readonly property bool usesScratchRotation: hasCurrentState
        && (root.stateOp4 === 1 || root.stateOp4 === 2)
    readonly property var anchor: root.usesScratchRotation
        ? ({ x: 0.5, y: 0.5 })
        : Lr2SkinUtils.centerAnchor(root.hasCurrentState ? root.stateCenter : 0)
    readonly property bool isSolidFill: !!srcData && srcData.specialType === 2
    readonly property bool sourceIsChartAsset: Lr2SkinUtils.isChartAssetSource(srcData)
    readonly property bool hasWholeTextureSource: !!srcData && !root.isSolidFill
        && (srcData.x < 0 || srcData.y < 0 || srcData.w < 0 || srcData.h < 0)
    readonly property bool hasCroppedTextureSource: !!srcData && !root.isSolidFill && srcData.w > 0 && srcData.h > 0
    readonly property bool isVideoSource: /\.(mpg|mpeg|mp4|avi|wmv|mov|mkv)$/i.test(root.resolvedSource)
    readonly property bool hasDrawableVideo: root.isVideoSource && root.resolvedSource !== ""
        && (root.hasWholeTextureSource || root.hasCroppedTextureSource)
    readonly property bool hasDrawableTexture: !root.isVideoSource && root.resolvedSource !== ""
        && (root.hasWholeTextureSource || root.hasCroppedTextureSource)
    readonly property bool hasRenderableState: root.hasCurrentState
        && root.effectiveAlpha > 0
        && root.drawW > 0
        && root.drawH > 0
    readonly property bool shouldPlayVideo: root.mediaActive
        && root.visible
        && root.opacity > 0
        && root.width > 0
        && root.height > 0
        && root.hasDrawableVideo
        && root.hasRenderableState

    function syncVideoPlayback() : void {
        if (videoLoader.item && videoLoader.item.syncVideoPlayback) {
            videoLoader.item.syncVideoPlayback();
        }
    }

    onShouldPlayVideoChanged: syncVideoPlayback()
    onResolvedSourceChanged: syncVideoPlayback()
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
        frameOverride: root.frameOverride
        textureWidth: Math.max(0, root.useFastImagePath ? 0 : atlasImage.implicitWidth)
        textureHeight: Math.max(0, root.useFastImagePath ? 0 : atlasImage.implicitHeight)
    }

    Item {
        id: sprite
        x: root.hasCurrentState ? root.drawX * root.scaleOverride : 0
        y: root.hasCurrentState ? root.drawY * root.scaleOverride : 0
        width: root.hasCurrentState ? root.drawW * root.scaleOverride : 0
        height: root.hasCurrentState ? root.drawH * root.scaleOverride : 0
        visible: root.hasRenderableState
        opacity: root.hasCurrentState ? root.effectiveAlpha / 255.0 : 0

        transform: Rotation {
            origin.x: sprite.width * root.anchor.x
            origin.y: sprite.height * root.anchor.y
            angle: root.effectiveAngle
        }

        Loader {
            id: videoLoader
            anchors.fill: parent
            active: root.shouldPlayVideo
            sourceComponent: videoComponent
            onLoaded: root.syncVideoPlayback()
        }

        Component {
            id: videoComponent

            Item {
                anchors.fill: parent

                function syncVideoPlayback() : void {
                    if (root.shouldPlayVideo) {
                        if (videoPlayer.playbackState !== MediaPlayer.PlayingState) {
                            videoPlayer.play();
                        }
                    } else {
                        videoPlayer.stop();
                    }
                }

                function stopVideo() : void {
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
            smooth: root.hasCurrentState && root.stateFilter !== 0
            mipmap: false
            visible: root.useFastImagePath && status === Image.Ready
        }

        Image {
            id: atlasImage
            source: root.hasDrawableTexture && !root.useFastImagePath ? root.resolvedSource : ""
            cache: true
            asynchronous: root.sourceIsChartAsset
            retainWhileLoading: true
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
            property real colorKeyEnabled: root.blendMode === 0 ? 1.0 : 0.0
            property real tolerance: 0.001
            property real nearestMode: root.hasCurrentState && root.stateFilter === 0 ? 1.0 : 0.0
            property vector2d sourceSize: Qt.vector2d(
                Math.max(1, atlasImage.implicitWidth),
                Math.max(1, atlasImage.implicitHeight))
            property vector4d sourceRect: root.animationFrameState.sourceRect
            fragmentShader: "qrc:/Lr2/Lr2SpriteAtlas.frag.qsb"
        }

        Rectangle {
            visible: root.isSolidFill || (!!srcData && srcData.specialType === 5)
            anchors.fill: parent
            color: srcData && srcData.specialType === 5 ? "white" : "black"
        }
    }
}
