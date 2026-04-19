import QtQuick

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
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property var chart
    property real scaleOverride: 1.0
    property real offsetX: 0
    property real offsetY: 0
    property bool colorKeyEnabled: true
    property int frameOverride: -1
    property var stateOverride: null

    readonly property var currentState: stateOverride || Lr2Timeline.getCurrentState(dsts, skinTime, timers, activeOptions)
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
        let raw = currentState ? currentState.blend : 1;
        if (raw === 0 && !root.colorKeyEnabled) return 1;
        if (raw === 5 || raw === 6) return 2;
        if (raw === 3 || raw === 4 || raw === 9 || raw === 11) return 1;
        return raw;
    }
    readonly property var anchor: Lr2Timeline.centerAnchor(currentState ? currentState.center : 4)
    readonly property bool isSolidFill: srcData && srcData.specialType === 2
    readonly property bool hasWholeTextureSource: srcData && !root.isSolidFill
        && (srcData.x < 0 || srcData.y < 0 || srcData.w < 0 || srcData.h < 0)
    readonly property bool hasCroppedTextureSource: srcData && !root.isSolidFill && srcData.w > 0 && srcData.h > 0
    readonly property bool hasDrawableTexture: root.resolvedSource !== ""
        && (root.hasWholeTextureSource || root.hasCroppedTextureSource)

    readonly property string resolvedSource: {
        if (!srcData) return "";
        if (srcData.specialType === 1 || srcData.specialType === 3 || srcData.specialType === 4) {
            let chartData = root.chart ? root.chart.chartData : null;
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

    readonly property int frameIndex: {
        if (!srcData) return 0;
        if (root.frameOverride >= 0) {
            let divX = Math.max(1, srcData.div_x || 1);
            let divY = Math.max(1, srcData.div_y || 1);
            return Math.max(0, Math.min(divX * divY - 1, root.frameOverride));
        }
        let timerIdx = srcData.timer || 0;
        let fire = (timers && timers[timerIdx] !== undefined) ? timers[timerIdx] : -1;
        return Lr2Timeline.getAnimationFrame(srcData, skinTime, fire);
    }

    Item {
        id: sprite
        x: root.currentState ? (root.currentState.x + root.offsetX) * root.scaleOverride : 0
        y: root.currentState ? (root.currentState.y + root.offsetY) * root.scaleOverride : 0
        width: root.currentState ? root.currentState.w * root.scaleOverride : 0
        height: root.currentState ? root.currentState.h * root.scaleOverride : 0
        visible: root.currentState && root.currentState.a > 0 && width > 0 && height > 0
        opacity: root.currentState ? root.currentState.a / 255.0 : 0

        transform: Rotation {
            origin.x: sprite.width * root.anchor.x
            origin.y: sprite.height * root.anchor.y
            angle: root.currentState ? root.currentState.angle : 0
        }

        Image {
            id: spriteImg
            anchors.fill: parent
            source: root.hasDrawableTexture ? root.resolvedSource : ""
            fillMode: Image.Stretch
            cache: true
            // For special blend modes, sibling effects sample this Image via
            // ShaderEffectSource and draw the final composite.
            // Keep the source item renderable for ShaderEffectSource. The
            // active effect hides it via hideSource; setting visible=false
            // prevents some blend-0 LR2 sprites (notably select bars) from
            // being sampled at all.
            visible: root.hasDrawableTexture

            sourceClipRect: {
                if (!srcData || !root.hasDrawableTexture || root.hasWholeTextureSource) {
                    return Qt.rect(0, 0, 0, 0);
                }

                let sx = Math.max(0, srcData.x || 0);
                let sy = Math.max(0, srcData.y || 0);
                let sw = srcData.w;
                let sh = srcData.h;

                let divX = Math.max(1, srcData.div_x || 1);
                let divY = Math.max(1, srcData.div_y || 1);
                let cellW = sw / divX;
                let cellH = sh / divY;

                let col = root.frameIndex % divX;
                let row = Math.floor(root.frameIndex / divX) % divY;

                return Qt.rect(sx + col * cellW, sy + row * cellH, cellW, cellH);
            }
        }

        // Blend mode 0: TRANSCOLOR (black -> transparent), then alpha blend.
        ColorChanger {
            anchors.fill: parent
            visible: root.hasDrawableTexture && root.blendMode === 0
            from: "black"
            to: "transparent"
            source: ShaderEffectSource {
                hideSource: root.blendMode === 0
                sourceItem: spriteImg
                live: true
                sourceRect: Qt.rect(0, 0, spriteImg.width, spriteImg.height)
            }
        }

        // Blend mode 2: ADD. fragColor = vec4(rgb*a, 0) collapses Qt's premult
        // blend (out = src + dst*(1-src.a)) to pure additive.
        // TODO: blend modes 3 (SUB), 4 (MULT), 9/11 fall through to plain
        // alpha until we wire up custom GL blend functions.
        ShaderEffect {
            anchors.fill: parent
            visible: root.hasDrawableTexture && root.blendMode === 2
            blending: true
            property variant source: ShaderEffectSource {
                hideSource: root.blendMode === 2
                sourceItem: spriteImg
                live: true
                sourceRect: Qt.rect(0, 0, spriteImg.width, spriteImg.height)
            }
            fragmentShader: "qrc:/Lr2AddBlend.frag.qsb"
        }

        // Blend mode 10: INVSRC / ANTI_COLOR.
        ShaderEffect {
            anchors.fill: parent
            visible: root.hasDrawableTexture && root.blendMode === 10
            blending: true
            property variant source: ShaderEffectSource {
                hideSource: root.blendMode === 10
                sourceItem: spriteImg
                live: true
                sourceRect: Qt.rect(0, 0, spriteImg.width, spriteImg.height)
            }
            fragmentShader: "qrc:/Lr2InvertBlend.frag.qsb"
        }

        Rectangle {
            visible: root.isSolidFill || (srcData && srcData.specialType === 5)
            anchors.fill: parent
            color: srcData && srcData.specialType === 5 ? "white" : "black"
        }
    }
}
