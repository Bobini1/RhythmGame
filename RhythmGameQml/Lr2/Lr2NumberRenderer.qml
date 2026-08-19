import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var skinClock: null
    property int skinClockMode: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property real scaleOverride: 1.0
    property real offsetX: 0
    property real offsetY: 0
    property int value: 0
    property bool forceHidden: false
    property bool asynchronousLoading: false
    property bool colorKeyEnabled: false
    property color transColor: "black"
    readonly property string sourcePathLower: srcData && srcData.source
        ? String(srcData.source).toLowerCase()
        : ""
    readonly property bool effectiveColorKeyEnabled: root.colorKeyEnabled
        && root.sourcePathLower.slice(-4) !== ".dds"
    property var stateOverride: null
    property Lr2TimelineState stateOverrideSource: null
    property var screenRoot: null
    readonly property int dstOffsetSide: srcData && srcData.side === 2 ? 2 : 1
    readonly property bool hasDstOffsets: dsts
        && dsts.length > 0
        && dsts[0]
        && dsts[0].offsets
        && dsts[0].offsets.length > 0
    readonly property bool dstOffsetsEnabled: !!root.screenRoot && hasDstOffsets

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
        dstOffsetsEnabled: root.dstOffsetsEnabled
        dstOffsetLiftY: root.screenRoot
            ? (root.dstOffsetSide === 2
                ? root.screenRoot.gameplayDstOffsetLiftY2
                : root.screenRoot.gameplayDstOffsetLiftY1)
            : 0
        dstOffsetLaneCoverY: root.screenRoot
            ? (root.dstOffsetSide === 2
                ? root.screenRoot.gameplayDstOffsetLaneCoverY2
                : root.screenRoot.gameplayDstOffsetLaneCoverY1)
            : 0
        dstOffsetHiddenY: root.screenRoot
            ? (root.dstOffsetSide === 2
                ? root.screenRoot.gameplayDstOffsetHiddenY2
                : root.screenRoot.gameplayDstOffsetHiddenY1)
            : 0
        dstOffsetHiddenA: root.screenRoot
            ? (root.dstOffsetSide === 2
                ? root.screenRoot.gameplayDstOffsetHiddenA2
                : root.screenRoot.gameplayDstOffsetHiddenA1)
            : 0
        colorKeyEnabled: root.effectiveColorKeyEnabled
        supportsInvertedBlend: false
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
    readonly property int stateBlend: drawState.blend
    readonly property bool hasRenderableState: drawState.renderable
    readonly property real effectiveOpacity: drawState.opacity
    readonly property int stateFilter: drawState.filter
    readonly property int blendMode: drawState.blendMode

    readonly property string resolvedSource: Lr2SkinUtils.fileUrlForPath(srcData ? srcData.source : "")

    function numberFrameGroupSize() : var {
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

    function numberAnimationBaseFrame() : var {
        let groupSize = root.frameGroupSize;
        if (!root.srcData || !root.srcData.cycle || root.srcData.cycle <= 0 || groupSize <= 0) {
            return 0;
        }
        let divX = Math.max(1, root.srcData.div_x || 1);
        let divY = Math.max(1, root.srcData.div_y || 1);
        let frames = divX * divY;
        if (frames <= groupSize) {
            return 0;
        }
        let timerIdx = root.srcData.timer || 0;
        let fire = root.sourceTimerFire > -2147483648
            ? root.sourceTimerFire
            : ((root.timers && root.timers[timerIdx] !== undefined)
                ? root.timers[timerIdx]
                : -1);
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
    readonly property int animationBaseFrame: root.numberAnimationBaseFrame()
    readonly property int sourceX: root.srcData ? Math.max(0, root.srcData.x || 0) : 0
    readonly property int sourceY: root.srcData ? Math.max(0, root.srcData.y || 0) : 0
    readonly property int sourceW: root.srcData ? root.srcData.w || 0 : 0
    readonly property int sourceH: root.srcData ? root.srcData.h || 0 : 0
    readonly property int sourceDivX: root.srcData ? Math.max(1, root.srcData.div_x || 1) : 1
    readonly property int sourceDivY: root.srcData ? Math.max(1, root.srcData.div_y || 1) : 1
    readonly property int sourceFrameCount: sourceDivX * sourceDivY
    readonly property int sourceCellW: root.sourceW > 0 ? Math.max(1, Math.floor(root.sourceW / root.sourceDivX)) : 0
    readonly property bool sourceAnimates: !!root.srcData
        && (root.srcData.cycle || 0) > 0
        && root.frameGroupSize > 0
        && root.sourceFrameCount > root.frameGroupSize
    readonly property int atlasW: Math.max(1, digitAtlas.implicitWidth)
    readonly property int atlasH: Math.max(1, digitAtlas.implicitHeight)
    readonly property bool hasAtlasSourceRects: !!root.srcData
        && digitAtlas.implicitWidth > 0
        && digitAtlas.implicitHeight > 0
        && root.sourceW > 0
        && root.sourceH > 0
    readonly property vector4d fullSourceRect: Qt.vector4d(0, 0, 1, 1)
    readonly property var atlasSourceRects: {
        if (!root.hasAtlasSourceRects) {
            return [];
        }

        const cellW = Math.max(1, Math.floor(root.sourceW / root.sourceDivX));
        const cellH = Math.max(1, Math.floor(root.sourceH / root.sourceDivY));
        const rects = [];
        for (let frame = 0; frame < root.sourceFrameCount; ++frame) {
            const col = frame % root.sourceDivX;
            const row = Math.floor(frame / root.sourceDivX) % root.sourceDivY;
            rects.push(Qt.vector4d(
                (root.sourceX + col * cellW) / root.atlasW,
                (root.sourceY + row * cellH) / root.atlasH,
                cellW / root.atlasW,
                cellH / root.atlasH));
        }
        return rects;
    }
    readonly property bool hasSignedFrames: frameGroupSize === 24
    readonly property bool isNegativeValue: root.value < 0
    readonly property int zeroPaddingMode: {
        if (!root.srcData || root.srcData.zeropadding === undefined || root.srcData.zeropadding < 0) {
            return 0;
        }
        return root.srcData.zeropadding;
    }
    readonly property int displayKeta: root.srcData ? root.srcData.keta || 0 : 0
    readonly property double absoluteRoundedValue: isFinite(root.value)
        ? Math.abs(Math.round(root.value))
        : 0
    readonly property int decimalDigitCount: root.absoluteRoundedValue < 10
        ? 1
        : Math.floor(Math.log(root.absoluteRoundedValue) / Math.LN10 + 1e-10) + 1
    readonly property int signSlotCount: root.hasSignedFrames ? 1 : 0
    readonly property int digitSlotCount: root.displayKeta > 0
        ? root.displayKeta
        : root.decimalDigitCount
    readonly property int displayLength: root.signSlotCount + root.digitSlotCount
    readonly property int displayPaddingCount: Math.max(0, root.displayKeta - root.decimalDigitCount)
    readonly property real digitW: root.hasCurrentState
        ? root.stateW * root.scaleOverride
        : 0
    readonly property real digitH: root.hasCurrentState ? root.stateH * root.scaleOverride : 0
    readonly property real textW: root.displayLength * digitW
    readonly property color tintColor: drawState.tintColor
    readonly property int centeredMissingDigits: srcData && srcData.align === 2 && srcData.keta > 0
        ? Math.max(0, srcData.keta - root.decimalDigitCount)
        : 0
    readonly property real alignOffset: centeredMissingDigits * digitW * 0.5
    readonly property bool isNowCombo: srcData
        && (srcData.nowCombo
            || (srcData.num === 104 || srcData.num === 124))
        && (srcData.timer === 46 || srcData.timer === 47)
    readonly property real nowComboOffset: isNowCombo ? -root.decimalDigitCount * digitW * 0.5 : 0

    Item {
        id: numberBox
        x: root.hasCurrentState ? (root.stateX + root.offsetX) * root.scaleOverride + root.alignOffset + root.nowComboOffset : 0
        y: root.hasCurrentState ? (root.stateY + root.offsetY) * root.scaleOverride : 0
        width: root.textW
        height: root.digitH
        visible: root.hasRenderableState
            && root.frameGroupSize > 0
            && root.digitW > 0
            && root.digitH > 0
            && root.resolvedSource !== ""
        opacity: root.effectiveOpacity

        Image {
            id: digitAtlas
            source: root.resolvedSource
            cache: true
            asynchronous: root.asynchronousLoading
                || (!!root.screenRoot && root.screenRoot.customizeMode === true)
            retainWhileLoading: true
            smooth: root.hasCurrentState && root.stateFilter !== 0
            mipmap: false
            visible: false
        }

        Repeater {
            model: root.displayLength

            Item {
                id: digitRoot
                x: index * root.digitW
                width: root.digitW
                height: root.digitH

                readonly property bool signSlot: root.hasSignedFrames && index === 0
                readonly property int digitSlot: index - root.signSlotCount
                readonly property bool rightAligned: !!root.srcData && root.srcData.align === 0
                readonly property int sourceDigitIndex: {
                    if (digitRoot.signSlot) {
                        return -1;
                    }
                    if (root.displayKeta <= 0) {
                        return digitRoot.digitSlot;
                    }
                    if (digitRoot.rightAligned) {
                        if (root.decimalDigitCount >= root.displayKeta) {
                            return digitRoot.digitSlot
                                + root.decimalDigitCount
                                - root.displayKeta;
                        }
                        return digitRoot.digitSlot < root.displayPaddingCount
                            ? -1
                            : digitRoot.digitSlot - root.displayPaddingCount;
                    }
                    return digitRoot.digitSlot < root.decimalDigitCount
                        ? digitRoot.digitSlot
                        : -1;
                }
                readonly property bool zeroPadding: !digitRoot.signSlot
                    && digitRoot.sourceDigitIndex < 0
                    && digitRoot.rightAligned
                    && root.zeroPaddingMode === 1
                readonly property int numericDigit: digitRoot.sourceDigitIndex < 0
                    ? -1
                    : Math.floor(root.absoluteRoundedValue
                        / Math.pow(10, root.decimalDigitCount - 1 - digitRoot.sourceDigitIndex)) % 10
                readonly property int frameIndex: {
                    if (digitRoot.signSlot) {
                        return root.isNegativeValue ? 23 : 11;
                    }
                    if (digitRoot.numericDigit >= 0) {
                        return digitRoot.numericDigit;
                    }
                    if (digitRoot.zeroPadding) {
                        return 0;
                    }
                    if (root.frameGroupSize === 24) {
                        return root.isNegativeValue ? 22 : 10;
                    }
                    return root.frameGroupSize === 11 ? 10 : -1;
                }
                readonly property int signedFrameOffset: root.frameGroupSize === 24
                    && root.isNegativeValue
                    && digitRoot.numericDigit >= 0
                    ? 12
                    : 0

                ShaderEffect {
                    anchors.fill: parent
                    visible: digitAtlas.status === Image.Ready && digitRoot.frameIndex >= 0
                    blending: true
                    supportsAtlasTextures: true
                    property var source: digitAtlas
                    property color tint: root.tintColor
                    property color transColor: root.transColor
                    property real blendMode: root.blendMode
                    property real colorKeyEnabled: root.effectiveColorKeyEnabled ? 1.0 : 0.0
                    property real tolerance: 0.001
                    property real nearestMode: root.hasCurrentState && root.stateFilter === 0 ? 2.0 : 0.0
                    property vector2d sourceSize: Qt.vector2d(root.atlasW, root.atlasH)
                    property vector4d sourceRect: {
                        if (digitRoot.frameIndex < 0 || root.atlasSourceRects.length <= 0) {
                            return root.fullSourceRect;
                        }
                        const frame = (root.animationBaseFrame
                            + digitRoot.frameIndex
                            + digitRoot.signedFrameOffset) % root.sourceFrameCount;
                        return root.atlasSourceRects[frame] || root.fullSourceRect;
                    }
                    fragmentShader: "qrc:/Lr2/Lr2SpriteAtlas.frag.qsb"
                }
            }
        }
    }
}
