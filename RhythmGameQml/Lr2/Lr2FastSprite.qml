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

    readonly property bool hasDrawableTexture: !!srcData
        && !!stateData
        && !!srcData.source
        && srcData.w > 0
        && srcData.h > 0
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
    }
    readonly property int frameIndex: animationFrameState.frameIndex
    readonly property real stateX: stateData ? stateData.x || 0 : 0
    readonly property real stateY: stateData ? stateData.y || 0 : 0
    readonly property real stateW: stateData ? stateData.w || 0 : 0
    readonly property real stateH: stateData ? stateData.h || 0 : 0
    readonly property int stateFilter: stateData ? stateData.filter || 0 : 0
    readonly property int stateStretch: stateData && stateData.stretch !== undefined
        ? stateData.stretch
        : -1
    readonly property real drawX: stateX + (stateW < 0 ? stateW : 0)
    readonly property real drawY: stateY + (stateH < 0 ? stateH : 0)

    function fillModeForStretch(stretch: var) : var {
        switch (stretch) {
        case 1:
        case 4:
        case 6:
        case 8:
        case 9:
            return Image.PreserveAspectFit;
        case 2:
        case 3:
        case 5:
        case 7:
        case 10:
            return Image.PreserveAspectCrop;
        default:
            return Image.Stretch;
        }
    }

    x: drawX * scaleOverride
    y: drawY * scaleOverride
    width: Math.abs(stateW) * scaleOverride
    height: Math.abs(stateH) * scaleOverride
    visible: hasDrawableTexture
        && (stateData.a === undefined || stateData.a > 0)
        && width > 0
        && height > 0
    opacity: stateData && stateData.a !== undefined ? stateData.a / 255.0 : 1.0
    source: hasDrawableTexture ? resolvedSource : ""
    sourceClipRect: animationFrameState.sourceClipRect
    fillMode: tileVertically ? Image.TileVertically : fillModeForStretch(stateStretch)
    cache: true
    asynchronous: true
    smooth: stateFilter !== 0
    mipmap: false
}
