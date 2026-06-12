pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

Image {
    id: root

    property var srcData
    property var stateData
    property int skinTime: 0
    property var timers: ({ 0: 0 })
    property int sourceTimerFire: -2147483648
    property real scaleOverride: 1.0
    property bool tileVertically: false
    property int frameOverride: -1
    property bool preloadTexture: false
    property bool useAtlasShader: false

    readonly property bool hasTextureSource: !!srcData
        && !!srcData.source
        && srcData.w > 0
        && srcData.h > 0
    readonly property bool hasDrawableTexture: hasTextureSource
        && (!!stateData || preloadTexture)
    readonly property bool hasRenderableTexture: hasTextureSource
        && !!stateData
    readonly property string resolvedSource: hasDrawableTexture
        ? Lr2SkinUtils.fileUrlForPath(srcData.source)
        : ""
    property Lr2AnimationFrameState animationFrameState: Lr2AnimationFrameState {
        enabled: !!root.srcData || root.frameOverride >= 0
        sourceData: root.srcData
        skinTime: root.skinTime
        timers: root.timers
        timerFire: root.sourceTimerFire
        frameOverride: root.frameOverride
        textureWidth: root.useAtlasShader ? Math.max(0, atlasImage.implicitWidth) : 0
        textureHeight: root.useAtlasShader ? Math.max(0, atlasImage.implicitHeight) : 0
    }
    readonly property int frameIndex: animationFrameState.frameIndex
    readonly property real stateX: stateData ? stateData.x || 0 : 0
    readonly property real stateY: stateData ? stateData.y || 0 : 0
    readonly property real stateW: stateData ? stateData.w || 0 : 0
    readonly property real stateH: stateData ? stateData.h || 0 : 0
    readonly property int stateFilter: stateData ? stateData.filter || 0 : 0
    readonly property real drawX: stateX + (stateW < 0 ? stateW : 0)
    readonly property real drawY: stateY + (stateH < 0 ? stateH : 0)

    x: drawX * scaleOverride
    y: drawY * scaleOverride
    width: Math.abs(stateW) * scaleOverride
    height: Math.abs(stateH) * scaleOverride
    visible: hasRenderableTexture
        && (stateData.a === undefined || stateData.a > 0)
        && width > 0
        && height > 0
    opacity: stateData && stateData.a !== undefined ? stateData.a / 255.0 : 1.0
    source: hasDrawableTexture && !useAtlasShader ? resolvedSource : ""
    sourceClipRect: useAtlasShader ? Qt.rect(0, 0, 0, 0) : animationFrameState.sourceClipRect
    fillMode: tileVertically ? Image.TileVertically : Image.Stretch
    cache: true
    asynchronous: false
    smooth: stateFilter !== 0
    mipmap: false

    Image {
        id: atlasImage

        source: root.hasDrawableTexture && root.useAtlasShader ? root.resolvedSource : ""
        cache: true
        asynchronous: false
        retainWhileLoading: true
        mipmap: false
        visible: false
    }

    ShaderEffect {
        anchors.fill: parent
        visible: root.useAtlasShader
            && root.hasRenderableTexture
            && atlasImage.status === Image.Ready
        blending: true
        supportsAtlasTextures: true
        property var source: atlasImage
        property color tint: "white"
        property color transColor: "black"
        property real blendMode: 1
        property real colorKeyEnabled: 0
        property real tolerance: 0.001
        property real nearestMode: root.stateFilter === 0 ? 1 : 0
        property vector2d sourceSize: Qt.vector2d(
            Math.max(1, atlasImage.implicitWidth),
            Math.max(1, atlasImage.implicitHeight))
        property vector4d sourceRect: root.animationFrameState.sourceRect
        fragmentShader: "qrc:/Lr2/Lr2SpriteAtlas.frag.qsb"
    }
}
