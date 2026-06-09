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
    property bool uppercase: false

    readonly property real naturalWidth: textImage.naturalWidth
    readonly property real naturalHeight: textImage.naturalHeight
    readonly property real textureHeight: textImage.textureHeight

    Lr2BitmapFontTexture {
        id: textImage

        anchors.fill: parent
        fontPath: root.fontPath
        text: root.text
        textColor: root.textColor
        textureFilter: root.textureFilter
        alignment: root.alignment
        uppercase: root.uppercase
    }
}
