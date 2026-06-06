import QtQuick
import RhythmGameQml

// Existing renderer API, backed by a native texture item instead of image-provider URLs.
Item {
    id: root

    property string fontPath: ""
    property string text: ""
    property color textColor: "white"
    property int textureFilter: 1
    // 0 = left (default), 1 = center, 2 = right. Matches LR2's #SRC_TEXT align.
    property int alignment: 0

    readonly property real naturalWidth: textImage.naturalWidth
    readonly property real naturalHeight: textImage.naturalHeight
    readonly property real textureHeight: textImage.textureHeight
    readonly property real scaleY: naturalHeight > 0 ? height / naturalHeight : 1
    readonly property real fitScaleX: naturalWidth > width && naturalWidth > 0
        ? width / naturalWidth
        : 1
    readonly property real drawnWidth: naturalWidth * scaleY * fitScaleX
    readonly property real alignedX: root.alignment === 1
        ? (root.width - root.drawnWidth) / 2
        : root.alignment === 2
            ? root.width - root.drawnWidth
            : 0
    readonly property bool hasRenderableText: root.fontPath.length > 0 && root.text.length > 0

    Lr2BitmapFontTexture {
        id: textImage

        x: root.alignedX
        width: root.drawnWidth
        height: root.textureHeight * root.scaleY
        visible: root.hasRenderableText
        fontPath: root.hasRenderableText ? root.fontPath : ""
        text: root.hasRenderableText ? root.text : ""
        textColor: root.textColor
        textureFilter: root.textureFilter
    }
}
