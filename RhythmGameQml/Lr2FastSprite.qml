pragma ValueTypeBehavior: Addressable
import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

Image {
    id: root

    property var srcData
    property var stateData
    property int skinTime: 0
    property var timers: ({ 0: 0 })
    property real scaleOverride: 1.0
    property bool tileVertically: false
    property int frameOverride: -1

    readonly property bool hasDrawableTexture: !!srcData
        && !!stateData
        && !!srcData.source
        && srcData.w > 0
        && srcData.h > 0
    readonly property string resolvedSource: {
        if (!hasDrawableTexture) {
            return "";
        }
        let absPath = srcData.source.replace(/\\/g, "/");
        if (/^[A-Za-z]:\//.test(absPath)) {
            return "file:///" + absPath;
        }
        if (absPath.startsWith("/")) {
            return "file://" + absPath;
        }
        return absPath;
    }
    readonly property int frameIndex: {
        if (!srcData) {
            return 0;
        }
        if (root.frameOverride >= 0) {
            let divX = Math.max(1, srcData.div_x || 1);
            let divY = Math.max(1, srcData.div_y || 1);
            return Math.max(0, Math.min(divX * divY - 1, root.frameOverride));
        }
        let timerIdx = srcData.timer || 0;
        let fire = timers && timers[timerIdx] !== undefined ? timers[timerIdx] : -1;
        return Lr2Timeline.getAnimationFrame(srcData, skinTime, fire);
    }
    readonly property real stateX: stateData ? stateData.x || 0 : 0
    readonly property real stateY: stateData ? stateData.y || 0 : 0
    readonly property real stateW: stateData ? stateData.w || 0 : 0
    readonly property real stateH: stateData ? stateData.h || 0 : 0
    readonly property real drawX: stateX + (stateW < 0 ? stateW : 0)
    readonly property real drawY: stateY + (stateH < 0 ? stateH : 0)

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
    sourceClipRect: {
        if (!srcData) {
            return Qt.rect(0, 0, 0, 0);
        }
        let divX = Math.max(1, srcData.div_x || 1);
        let divY = Math.max(1, srcData.div_y || 1);
        let cellW = srcData.w / divX;
        let cellH = srcData.h / divY;
        let col = frameIndex % divX;
        let row = Math.floor(frameIndex / divX) % divY;
        return Qt.rect(
            (srcData.x || 0) + col * cellW,
            (srcData.y || 0) + row * cellH,
            cellW,
            cellH);
    }
    fillMode: tileVertically ? Image.TileVertically : Image.Stretch
    cache: true
    asynchronous: true
    smooth: false
    mipmap: false
}
