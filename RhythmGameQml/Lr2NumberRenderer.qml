import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property real scaleOverride: 1.0
    property real offsetX: 0
    property real offsetY: 0
    property int value: 0

    readonly property var currentState: Lr2Timeline.getCurrentState(dsts, skinTime, timers, activeOptions)
    readonly property int blendMode: {
        let raw = currentState ? currentState.blend : 1;
        if (raw === 5 || raw === 6) return 2;
        if (raw === 3 || raw === 4 || raw === 9 || raw === 10 || raw === 11) return 1;
        return raw;
    }

    readonly property string resolvedSource: {
        if (!srcData || !srcData.source) return "";
        let absPath = srcData.source.replace(/\\/g, "/");
        if (/^[A-Za-z]:\//.test(absPath)) {
            return "file:///" + absPath;
        }
        if (absPath.startsWith("/")) {
            return "file://" + absPath;
        }
        return absPath;
    }

    function textForValue() {
        let text = Math.round(root.value).toString();
        let keta = root.srcData ? root.srcData.keta || 0 : 0;
        if (keta > 0 && root.srcData && root.srcData.align === 2) {
            while (text.length < keta) {
                text = "0" + text;
            }
        }
        return text;
    }

    readonly property string displayText: textForValue()
    readonly property real digitW: root.currentState ? root.currentState.w * root.scaleOverride : 0
    readonly property real digitH: root.currentState ? root.currentState.h * root.scaleOverride : 0
    readonly property real textW: displayText.length * digitW
    readonly property real alignOffset: {
        if (!currentState || !srcData) return 0;
        if (srcData.align === 1) {
            return Math.max(0, currentState.w * scaleOverride - textW);
        }
        if (srcData.align === 2) {
            return Math.max(0, (currentState.w * scaleOverride - textW) / 2);
        }
        return 0;
    }

    Item {
        id: numberBox
        x: root.currentState ? (root.currentState.x + root.offsetX) * root.scaleOverride + root.alignOffset : 0
        y: root.currentState ? (root.currentState.y + root.offsetY) * root.scaleOverride : 0
        width: root.textW
        height: root.digitH
        visible: root.currentState && root.currentState.a > 0 && root.digitW > 0 && root.digitH > 0 && root.resolvedSource !== ""
        opacity: root.currentState ? root.currentState.a / 255.0 : 0

        Image {
            id: digitAtlas
            source: root.resolvedSource
            cache: true
            visible: false
        }

        Repeater {
            model: root.displayText.length

            Item {
                id: digitRoot
                x: index * root.digitW
                width: root.digitW
                height: root.digitH

                readonly property string ch: root.displayText.charAt(index)
                readonly property int digit: {
                    let parsed = parseInt(ch);
                    return isNaN(parsed) ? 0 : parsed;
                }

                ShaderEffect {
                    anchors.fill: parent
                    visible: digitAtlas.status === Image.Ready
                    blending: true
                    property variant source: digitAtlas
                    property color tint: "white"
                    property color transColor: "black"
                    property real blendMode: root.blendMode
                    property real colorKeyEnabled: root.blendMode === 0 ? 1.0 : 0.0
                    property real tolerance: 0.03125
                    property vector4d sourceRect: {
                        if (!root.srcData
                            || digitAtlas.implicitWidth <= 0
                            || digitAtlas.implicitHeight <= 0) {
                            return Qt.vector4d(0, 0, 1, 1);
                        }
                        let sx = Math.max(0, root.srcData.x || 0);
                        let sy = Math.max(0, root.srcData.y || 0);
                        let sw = root.srcData.w || 0;
                        let sh = root.srcData.h || 0;
                        if (sw <= 0 || sh <= 0) {
                            return Qt.vector4d(0, 0, 1, 1);
                        }
                        let divX = Math.max(1, root.srcData.div_x || 1);
                        let divY = Math.max(1, root.srcData.div_y || 1);
                        let cellW = sw / divX;
                        let cellH = sh / divY;
                        let frame = digitRoot.digit % (divX * divY);
                        let col = frame % divX;
                        let row = Math.floor(frame / divX) % divY;
                        let atlasW = Math.max(1, digitAtlas.implicitWidth);
                        let atlasH = Math.max(1, digitAtlas.implicitHeight);
                        return Qt.vector4d(
                            (sx + col * cellW) / atlasW,
                            (sy + row * cellH) / atlasH,
                            cellW / atlasW,
                            cellH / atlasH);
                    }
                    fragmentShader: "qrc:/Lr2SpriteAtlas.frag.qsb"
                }
            }
        }
    }
}
