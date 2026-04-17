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
    // 0 = left (default), 1 = right, 2 = center. Matches LR2's #SRC_TEXT align.
    property int alignment: 0

    // Force recomputation whenever fontPath or text changes; the binding
    // below depends on both, so Qt invalidates it automatically.
    readonly property var glyphs: (fontPath && text)
        ? Lr2FontCatalog.layout(fontPath, text)
        : []

    readonly property int lineHeight: fontPath ? Lr2FontCatalog.lineHeight(fontPath) : 0

    // Scale so that one line of glyphs fits our height exactly. If either
    // dimension is unknown, render at 1:1 so the text is at least visible.
    readonly property real glyphScale: (lineHeight > 0 && height > 0)
        ? (height / lineHeight)
        : 1.0

    readonly property real totalWidth: {
        let sum = 0;
        for (let i = 0; i < glyphs.length; ++i) {
            let g = glyphs[i];
            let advance = g ? (g.advance || 0) : 0;
            sum += advance * glyphScale;
        }
        return sum;
    }

    readonly property real offsetX: {
        if (alignment === 1) return Math.max(0, width - totalWidth);
        if (alignment === 2) return Math.max(0, (width - totalWidth) / 2);
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
                    ? modelData.w * root.glyphScale
                    : advance * root.glyphScale
                height: root.height

                Image {
                    visible: hasGlyph
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    cache: true
                    smooth: true
                    source: hasGlyph
                        ? ("image://lr2font/" + root.fontPath + "?atlas=" + modelData.atlas)
                        : ""
                    sourceClipRect: hasGlyph
                        ? Qt.rect(modelData.x, modelData.y, modelData.w, modelData.h)
                        : Qt.rect(0, 0, 0, 0)
                }
            }
        }
    }
}
