import QtQuick

// Existing renderer API, backed by one provider-generated texture per string.
Item {
    id: root

    property string fontPath: ""
    property string text: ""
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

    Image {
        id: textImage

        x: root.alignedX
        width: root.drawnWidth
        height: root.height
        visible: root.fontPath.length > 0 && root.text.length > 0
        source: visible
            ? "image://lr2font/" + encodeURIComponent(root.fontPath)
                + "?text=" + encodeURIComponent(root.text)
            : ""
        cache: true
        smooth: false
        mipmap: false
        fillMode: Image.Stretch
    }
}
