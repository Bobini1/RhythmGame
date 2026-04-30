import QtQuick
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

// Root fills the Loader (skin canvas); inner `textBox` does the real
// animated positioning/sizing so that QQuickLoader's size-sync can't
// clobber our width/height bindings.
Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var skinClock: null
    property int skinClockMode: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property var chart
    property real scaleOverride: 1.0
    property real offsetX: 0
    property real offsetY: 0
    property var stateOverride: null
    // Bound externally by the screen wrapper with the actual text for this st index
    property string resolvedText: ""

    readonly property bool hasStaticTimelineState: !stateOverride
        && Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.stateOverride && !root.hasStaticTimelineState
        skinClock: root.skinClock
        clockMode: root.skinClockMode
        dsts: root.dsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptions: root.timelineActiveOptions
    }
    readonly property var objectState: root.stateOverride || root.staticTimelineState
    readonly property bool hasCurrentState: !!objectState || root.timelineState.hasState
    readonly property real stateX: objectState ? (objectState.x || 0) : (timelineState.hasState ? timelineState.stateX : 0)
    readonly property real stateY: objectState ? (objectState.y || 0) : (timelineState.hasState ? timelineState.stateY : 0)
    readonly property real stateW: objectState ? (objectState.w || 0) : (timelineState.hasState ? timelineState.stateW : 0)
    readonly property real stateH: objectState ? (objectState.h || 0) : (timelineState.hasState ? timelineState.stateH : 0)
    readonly property real stateA: objectState ? (objectState.a === undefined ? 255 : objectState.a) : (timelineState.hasState ? timelineState.stateA : 0)
    readonly property real stateR: objectState ? (objectState.r === undefined ? 255 : objectState.r) : (timelineState.hasState ? timelineState.stateR : 255)
    readonly property real stateG: objectState ? (objectState.g === undefined ? 255 : objectState.g) : (timelineState.hasState ? timelineState.stateG : 255)
    readonly property real stateB: objectState ? (objectState.b === undefined ? 255 : objectState.b) : (timelineState.hasState ? timelineState.stateB : 255)
    readonly property int stateBlend: objectState ? (objectState.blend || 0) : (timelineState.hasState ? timelineState.stateBlend : 0)
    readonly property bool isLr2Font: srcData
        && (srcData.bitmapFont || (srcData.fontPath && srcData.fontPath.toLowerCase().endsWith(".lr2font")))
    readonly property int blendMode: {
        let raw = hasCurrentState ? stateBlend : 1;
        if (raw === 5 || raw === 6) return 2;
        if (raw === 3 || raw === 4 || raw === 9 || raw === 10 || raw === 11) return 1;
        return raw;
    }
    readonly property string displayText: root.resolvedText || ("ST_" + (root.srcData ? root.srcData.st : "?"))
    readonly property color textColor: root.hasCurrentState
        ? Qt.rgba(root.stateR / 255.0, root.stateG / 255.0, root.stateB / 255.0, 1.0)
        : "white"
    readonly property string fontFamily: root.srcData ? (root.srcData.fontFamily || root.srcData.fontPath || "") : ""
    readonly property int textAlignment: root.srcData ? root.srcData.align : 0
    readonly property int textFontSize: root.srcData ? root.srcData.fontSize : 0
    readonly property int textFontThickness: root.srcData ? root.srcData.fontThickness : 0
    readonly property int textFontType: root.srcData ? root.srcData.fontType : 0
    readonly property real maxLayerSize: 32760
    readonly property real dstWidth: root.hasCurrentState ? root.stateW * root.scaleOverride : 0
    readonly property real dstHeight: root.hasCurrentState ? root.stateH * root.scaleOverride : 0
    // LR2 skins use huge DST widths such as 99999 as "unclipped text". Qt
    // would turn that into an impossible layer/source texture, so cap only
    // the rendered item size; the value is still far wider than the viewport.
    readonly property real renderWidth: Math.min(Math.abs(dstWidth), maxLayerSize)
    readonly property real renderHeight: Math.min(Math.abs(dstHeight), maxLayerSize)
    readonly property real anchorOffsetX: {
        if (root.textAlignment === 1) return -root.renderWidth * 0.5;
        if (root.textAlignment === 2) return -root.renderWidth;
        return 0;
    }

    Item {
        id: textBox
        x: root.hasCurrentState ? (root.stateX + root.offsetX) * root.scaleOverride + root.anchorOffsetX : 0
        y: root.hasCurrentState ? (root.stateY + root.offsetY) * root.scaleOverride : 0
        width: root.renderWidth
        height: root.renderHeight
        visible: root.hasCurrentState && root.stateA > 0 && width > 0 && height > 0
        opacity: root.hasCurrentState ? root.stateA / 255.0 : 0

        Lr2BitmapFontText {
            visible: root.isLr2Font && root.blendMode !== 2
            anchors.fill: parent
            fontPath: root.srcData ? root.srcData.fontPath : ""
            text: root.resolvedText
            alignment: root.textAlignment
        }

        Lr2SystemFontText {
            visible: !root.isLr2Font && root.blendMode !== 2
            anchors.fill: parent
            text: root.displayText
            textColor: root.textColor
            family: root.fontFamily
            alignment: root.textAlignment
            fontSize: root.textFontSize
            fontThickness: root.textFontThickness
            fontType: root.textFontType
            blendMode: root.blendMode
        }

        Item {
            id: additiveTextSource
            visible: root.blendMode === 2
            anchors.fill: parent

            Lr2BitmapFontText {
                visible: root.isLr2Font
                anchors.fill: parent
                fontPath: root.srcData ? root.srcData.fontPath : ""
                text: root.resolvedText
                alignment: root.textAlignment
            }

            Lr2SystemFontText {
                visible: !root.isLr2Font
                anchors.fill: parent
                text: root.displayText
                textColor: root.textColor
                family: root.fontFamily
                alignment: root.textAlignment
                fontSize: root.textFontSize
                fontThickness: root.textFontThickness
                fontType: root.textFontType
                blendMode: root.blendMode
            }
        }

        // LR2 often layers an additive copy of text over the normal one for
        // the shimmering title effect. Reuse the sprite ADD shader here: it
        // emits rgb * alpha with zero alpha so Qt Quick's premultiplied blend
        // becomes dst + src.
        ShaderEffect {
            anchors.fill: parent
            visible: root.blendMode === 2
            blending: true
            property var source: ShaderEffectSource {
                hideSource: true
                sourceItem: additiveTextSource
                live: true
                sourceRect: Qt.rect(0, 0, additiveTextSource.width, additiveTextSource.height)
            }
            fragmentShader: "qrc:/Lr2AddBlend.frag.qsb"
        }
    }
}
