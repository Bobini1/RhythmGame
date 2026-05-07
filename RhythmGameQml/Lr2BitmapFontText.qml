import QtQuick

// Existing renderer API, backed by one provider-generated texture per string.
Item {
    id: root

    property string fontPath: ""
    property string text: ""
    property color textColor: "white"
    // 0 = left (default), 1 = center, 2 = right. Matches LR2's #SRC_TEXT align.
    property int alignment: 0

    readonly property real naturalWidth: textImage.implicitWidth
    readonly property real naturalHeight: textImage.implicitHeight
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
    readonly property bool hasColorTint: Math.abs(root.textColor.r - 1.0) > 0.001
        || Math.abs(root.textColor.g - 1.0) > 0.001
        || Math.abs(root.textColor.b - 1.0) > 0.001

    Image {
        id: textImage

        x: root.alignedX
        width: root.drawnWidth
        height: root.height
        visible: root.hasRenderableText && !root.hasColorTint
        source: root.hasRenderableText
            ? "image://lr2font/" + encodeURIComponent(root.fontPath)
                + "?text=" + encodeURIComponent(root.text)
            : ""
        cache: true
        smooth: true
        mipmap: false
        fillMode: Image.Stretch
    }

    ShaderEffect {
        x: root.alignedX
        width: root.drawnWidth
        height: root.height
        visible: root.hasColorTint && textImage.status === Image.Ready && root.hasRenderableText
        blending: true
        property var source: textImage
        property color tint: root.textColor
        fragmentShader: "qrc:/Lr2Tint.frag.qsb"
    }
}
