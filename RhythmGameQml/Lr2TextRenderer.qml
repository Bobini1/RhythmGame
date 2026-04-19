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
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property var chart
    property real scaleOverride: 1.0
    property real offsetX: 0
    property real offsetY: 0
    // Bound externally by the screen wrapper with the actual text for this st index
    property string resolvedText: ""

    readonly property var currentState: Lr2Timeline.getCurrentState(dsts, skinTime, timers, activeOptions)
    readonly property bool isLr2Font: srcData
        && (srcData.bitmapFont || (srcData.fontPath && srcData.fontPath.toLowerCase().endsWith(".lr2font")))
    readonly property int blendMode: {
        let raw = currentState ? currentState.blend : 1;
        if (raw === 5 || raw === 6) return 2;
        if (raw === 3 || raw === 4 || raw === 9 || raw === 10 || raw === 11) return 1;
        return raw;
    }
    readonly property string displayText: root.resolvedText || ("ST_" + (root.srcData ? root.srcData.st : "?"))
    readonly property color textColor: root.currentState
        ? Qt.rgba(root.currentState.r / 255.0, root.currentState.g / 255.0, root.currentState.b / 255.0, 1.0)
        : "white"
    readonly property string fontFamily: root.srcData ? (root.srcData.fontFamily || root.srcData.fontPath || "") : ""
    readonly property int textAlignment: root.srcData ? root.srcData.align : 0
    readonly property int textFontSize: root.srcData ? root.srcData.fontSize : 0
    readonly property int textFontThickness: root.srcData ? root.srcData.fontThickness : 0
    readonly property int textFontType: root.srcData ? root.srcData.fontType : 0

    Item {
        id: textBox
        x: root.currentState ? (root.currentState.x + root.offsetX) * root.scaleOverride : 0
        y: root.currentState ? (root.currentState.y + root.offsetY) * root.scaleOverride : 0
        width: root.currentState ? root.currentState.w * root.scaleOverride : 0
        height: root.currentState ? root.currentState.h * root.scaleOverride : 0
        visible: root.currentState && root.currentState.a > 0 && width > 0 && height > 0
        opacity: root.currentState ? root.currentState.a / 255.0 : 0

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
            property variant source: ShaderEffectSource {
                hideSource: true
                sourceItem: additiveTextSource
                live: true
                sourceRect: Qt.rect(0, 0, additiveTextSource.width, additiveTextSource.height)
            }
            fragmentShader: "qrc:/Lr2AddBlend.frag.qsb"
        }
    }
}
