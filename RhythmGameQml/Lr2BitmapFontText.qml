import QtQuick
import RhythmGameQml 1.0

// Renders an .lr2font-bitmap string by emitting one Image per glyph that
// clips out of the shared atlas texture via sourceClipRect. The atlas is
// served by the "lr2font" QQuickImageProvider and cached by Qt per URL,
// so every glyph sharing an atlas costs one texture upload total.
Item {
    id: root

    property string fontPath: ""
    property string text: ""
    // 0 = left (default), 1 = center, 2 = right. Matches LR2's #SRC_TEXT align.
    property int alignment: 0

    // Force recomputation whenever fontPath or text changes; the binding
    // below depends on both, so Qt invalidates it automatically.
    readonly property var glyphs: (fontPath && text)
        ? Lr2FontCatalog.layout(fontPath, text)
        : []

    readonly property int lineHeight: fontPath ? Lr2FontCatalog.lineHeight(fontPath) : 0
    readonly property string encodedFontPath: fontPath ? encodeURIComponent(fontPath) : ""

    // Scale so that one line of glyphs fits our height exactly. If either
    // dimension is unknown, render at 1:1 so the text is at least visible.
    readonly property real glyphScaleY: (lineHeight > 0 && height > 0)
        ? (height / lineHeight)
        : 1.0

    readonly property real sourceTotalWidth: {
        let sum = 0;
        for (let i = 0; i < glyphs.length; ++i) {
            let g = glyphs[i];
            let advance = g ? (g.advance || 0) : 0;
            sum += advance;
        }
        return sum;
    }
    readonly property real fitScaleX: sourceTotalWidth > width && width > 0
        ? width / sourceTotalWidth
        : 1.0
    readonly property real glyphScaleX: glyphScaleY * fitScaleX
    readonly property real totalWidth: sourceTotalWidth * glyphScaleX

    readonly property real offsetX: {
        if (alignment === 1) return -totalWidth / 2;
        if (alignment === 2) return -totalWidth;
        return 0;
    }

    Row {
        x: root.offsetX
        y: 0
        height: root.height

        Repeater {
            model: root.glyphs

            delegate: Item {
                readonly property bool hasGlyph: modelData && modelData.found === true
                readonly property real advance: modelData ? (modelData.advance || 0) : 0

                width: hasGlyph
                    ? modelData.w * root.glyphScaleX
                    : advance * root.glyphScaleX
                height: root.height

                Image {
                    visible: hasGlyph
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    cache: true
                    smooth: false
                    source: hasGlyph
                        ? ("image://lr2font/" + root.encodedFontPath
                            + "?atlas=" + modelData.atlas
                            + "&x=" + modelData.x
                            + "&y=" + modelData.y
                            + "&w=" + modelData.w
                            + "&h=" + modelData.h)
                        : ""
                }
            }
        }
    }
}
