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
    property bool forceHidden: false
    property int animationRevision: 0
    property bool colorKeyEnabled: false
    property color transColor: "black"

    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    readonly property var currentState: root.forceHidden
        ? null
        : (staticTimelineState || Lr2Timeline.getCurrentState(dsts, skinTime, timelineTimers, timelineActiveOptions))
    readonly property int blendMode: {
        let raw = currentState ? currentState.blend : 1;
        if (raw === 0 && !root.colorKeyEnabled) return 1;
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

    function digitCount(value) {
        return Math.abs(Math.round(value)).toString().length;
    }

    function leftPad(text, width) {
        let result = text;
        while (result.length < width) {
            result = " " + result;
        }
        return result.slice(result.length - width);
    }

    function rightPad(text, width) {
        let result = text;
        while (result.length < width) {
            result += " ";
        }
        return result.slice(0, width);
    }

    function numberFrameGroupSize() {
        if (!root.srcData || !root.srcData.cycle || root.srcData.cycle <= 0) {
            let divX = root.srcData ? Math.max(1, root.srcData.div_x || 1) : 1;
            let divY = root.srcData ? Math.max(1, root.srcData.div_y || 1) : 1;
            let frames = divX * divY;
            if (frames % 24 === 0) return 24;
            if (frames % 11 === 0) return 11;
            if (frames % 10 === 0) return 10;
            return 0;
        }
        let divX = Math.max(1, root.srcData.div_x || 1);
        let divY = Math.max(1, root.srcData.div_y || 1);
        let frames = divX * divY;
        if (frames % 24 === 0) return 24;
        if (frames % 11 === 0) return 11;
        if (frames % 10 === 0) return 10;
        return 0;
    }

    function numberAnimationBaseFrame() {
        let groupSize = root.numberFrameGroupSize();
        if (!root.srcData || !root.srcData.cycle || root.srcData.cycle <= 0 || groupSize <= 0) {
            return 0;
        }
        let divX = Math.max(1, root.srcData.div_x || 1);
        let divY = Math.max(1, root.srcData.div_y || 1);
        let frames = divX * divY;
        let timerIdx = root.srcData.timer || 0;
        let fire = (root.timers && root.timers[timerIdx] !== undefined)
            ? root.timers[timerIdx]
            : -1;
        if (fire < 0) {
            return 0;
        }
        let elapsed = root.skinTime - fire;
        if (elapsed < 0) {
            return 0;
        }
        let rows = frames / groupSize;
        return Math.floor((elapsed % root.srcData.cycle) * rows / root.srcData.cycle) * groupSize;
    }

    readonly property int frameGroupSize: root.numberFrameGroupSize()
    readonly property bool hasSignedFrames: frameGroupSize === 24
    readonly property bool negativeUnsupported: root.value < 0 && frameGroupSize !== 24
    readonly property bool isNegativeValue: root.value < 0

    function textForValue() {
        let text = Math.abs(Math.round(root.value)).toString();
        let keta = root.srcData ? root.srcData.keta || 0 : 0;
        if (keta <= 0) {
            return root.hasSignedFrames ? (root.isNegativeValue ? "-" : "+") + text : text;
        }

        // LR2 align=0 is "%10d" followed by lastCut(keta).
        if (root.srcData && root.srcData.align === 0) {
            text = leftPad(text, keta);
        } else {
            text = rightPad(text, keta);
        }
        return root.hasSignedFrames ? (root.isNegativeValue ? "-" : "+") + text : text;
    }

    readonly property string displayText: textForValue()
    readonly property real digitW: root.currentState ? root.currentState.w * root.scaleOverride : 0
    readonly property real digitH: root.currentState ? root.currentState.h * root.scaleOverride : 0
    readonly property real textW: displayText.length * digitW
    function colorComponent(value) {
        if (value === undefined || value === null) return 1.0;
        return Math.max(0, Math.min(255, value)) / 255.0;
    }
    readonly property real tintR: root.currentState ? root.colorComponent(root.currentState.r) : 1.0
    readonly property real tintG: root.currentState ? root.colorComponent(root.currentState.g) : 1.0
    readonly property real tintB: root.currentState ? root.colorComponent(root.currentState.b) : 1.0
    readonly property color tintColor: Qt.rgba(root.tintR, root.tintG, root.tintB, 1.0)
    readonly property int centeredMissingDigits: srcData && srcData.align === 2 && srcData.keta > 0
        ? Math.max(0, srcData.keta - digitCount(root.value))
        : 0
    readonly property real alignOffset: centeredMissingDigits * digitW * 0.5
    readonly property bool isNowCombo: srcData
        && (srcData.nowCombo
            || (srcData.num === 104 || srcData.num === 124))
        && (srcData.timer === 46 || srcData.timer === 47)
    readonly property real nowComboOffset: isNowCombo ? -digitCount(root.value) * digitW * 0.5 : 0

    Item {
        id: numberBox
        x: root.currentState ? (root.currentState.x + root.offsetX) * root.scaleOverride + root.alignOffset + root.nowComboOffset : 0
        y: root.currentState ? (root.currentState.y + root.offsetY) * root.scaleOverride : 0
        width: root.textW
        height: root.digitH
        visible: root.currentState
            && !root.negativeUnsupported
            && root.frameGroupSize > 0
            && root.currentState.a > 0
            && root.digitW > 0
            && root.digitH > 0
            && root.resolvedSource !== ""
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
                readonly property int frameIndex: {
                    if (ch.length <= 0) {
                        return -1;
                    }
                    let code = ch.charCodeAt(0);
                    if (code >= 48 && code <= 57) {
                        return code - 48;
                    }
                    let groupSize = root.frameGroupSize;
                    if (groupSize === 24) {
                        if (ch === "+") return 11;
                        if (ch === "-") return 23;
                        if (ch === " ") return root.isNegativeValue ? 22 : 10;
                    }
                    return ch === " " && groupSize === 11 ? 10 : -1;
                }
                readonly property int signedFrameOffset: root.frameGroupSize === 24
                    && root.isNegativeValue
                    && digitRoot.ch >= "0"
                    && digitRoot.ch <= "9"
                    ? 12
                    : 0

                ShaderEffect {
                    anchors.fill: parent
                    visible: digitAtlas.status === Image.Ready && digitRoot.frameIndex >= 0
                    blending: true
                    property variant source: digitAtlas
                    property color tint: root.tintColor
                    property color transColor: root.transColor
                    property real blendMode: root.blendMode
                    property real colorKeyEnabled: root.blendMode === 0 ? 1.0 : 0.0
                    property real tolerance: 0.03125
                    property real nearestMode: root.currentState && (root.currentState.filter || 0) === 0 ? 1.0 : 0.0
                    property vector2d sourceSize: Qt.vector2d(
                        Math.max(1, digitAtlas.implicitWidth),
                        Math.max(1, digitAtlas.implicitHeight))
                    property vector4d sourceRect: {
                        if (!root.srcData
                            || digitRoot.frameIndex < 0
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
                        let cellW = Math.max(1, Math.floor(sw / divX));
                        let cellH = Math.max(1, Math.floor(sh / divY));
                        root.animationRevision;
                        let frame = (root.numberAnimationBaseFrame()
                                     + digitRoot.frameIndex
                                     + digitRoot.signedFrameOffset) % (divX * divY);
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
