import QtQuick
import RhythmGameQml

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
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property var chart
    property real scaleOverride: 1.0
    property real offsetX: 0
    property real offsetY: 0
    property var stateOverride: null
    property bool showUnresolvedTextFallback: true
    // Bound externally by the screen wrapper with the actual text for this st index
    property string resolvedText: ""

    Lr2TimelineFrame {
        id: drawState
        dsts: root.dsts
        srcData: root.srcData
        skinTime: root.skinTime
        skinClock: root.skinClock
        skinClockMode: root.skinClockMode
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
        timers: root.timers
        timerFire: root.timerFire
        stateOverride: root.stateOverride
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
    readonly property bool isLr2Font: srcData
        && (srcData.bitmapFont || (srcData.fontPath && srcData.fontPath.toLowerCase().endsWith(".lr2font")))
    readonly property int blendMode: drawState.blendMode
    readonly property string displayText: root.resolvedText
        || (root.showUnresolvedTextFallback
            ? ("ST_" + (root.srcData ? root.srcData.st : "?"))
            : "")
    readonly property int stateFilter: drawState.filter
    readonly property color textColor: root.hasCurrentState
        ? Qt.rgba(root.stateR / 255.0, root.stateG / 255.0, root.stateB / 255.0, 1.0)
        : "white"
    readonly property string fontFamily: root.srcData ? (root.srcData.fontFamily || root.srcData.fontPath || "") : ""
    readonly property string normalizedFontPath: root.srcData
        ? String(root.srcData.fontPath || "").replace(/\\/g, "/").toLowerCase()
        : ""
    readonly property bool uppercaseBitmapText: root.isLr2Font
        && root.normalizedFontPath.indexOf("/font/title/") !== -1
    readonly property string bitmapText: root.uppercaseBitmapText
        ? root.resolvedText.toUpperCase()
        : root.resolvedText
    readonly property int textAlignment: root.srcData ? root.srcData.align : 0
    readonly property int textFontSize: root.srcData ? root.srcData.fontSize : 0
    readonly property int textFontThickness: root.srcData ? root.srcData.fontThickness : 0
    readonly property int textFontType: root.srcData ? root.srcData.fontType : 0
    readonly property real maxLayerSize: 16384
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
            fontPath: root.blendMode !== 2 && root.srcData ? root.srcData.fontPath : ""
            text: root.blendMode !== 2 ? root.bitmapText : ""
            textColor: root.textColor
            textureFilter: root.stateFilter
            alignment: root.textAlignment
        }

        Lr2SystemFontText {
            visible: !root.isLr2Font && root.blendMode !== 2
            anchors.fill: parent
            text: root.blendMode !== 2 ? root.displayText : ""
            textColor: root.textColor
            family: root.blendMode !== 2 ? root.fontFamily : ""
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
                visible: root.blendMode === 2 && root.isLr2Font
                anchors.fill: parent
                fontPath: root.blendMode === 2 && root.srcData ? root.srcData.fontPath : ""
                text: root.blendMode === 2 ? root.bitmapText : ""
                textColor: root.textColor
                textureFilter: root.stateFilter
                alignment: root.textAlignment
            }

            Lr2SystemFontText {
                visible: root.blendMode === 2 && !root.isLr2Font
                anchors.fill: parent
                text: root.blendMode === 2 ? root.displayText : ""
                textColor: root.textColor
                family: root.blendMode === 2 ? root.fontFamily : ""
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
            fragmentShader: "qrc:/Lr2/Lr2AddBlend.frag.qsb"
        }
    }
}

