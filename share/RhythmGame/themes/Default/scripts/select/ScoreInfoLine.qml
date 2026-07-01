import RhythmGameQml
import QtQuick
import "../common"

Item {
    id: scoreInfoLine

    property alias source: image.source
    property alias text: text.text
    property string fontFile: "file:NotoSansJP-VariableFont_wght.ttf"
    property real valueLeftMargin: Math.max(0, image.width - 40)

    height: image.height
    width: 264

    ThemeFont {
        id: scoreInfoLineFont
        fileName: scoreInfoLine.fontFile
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    Image {
        id: image

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
    }
    Text {
        id: text

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.leftMargin: scoreInfoLine.valueLeftMargin
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 1
        height: parent.height + 10
        font.family: scoreInfoLineFont.fontFamily
        font.weight: scoreInfoLineFont.fontWeight
        font.italic: scoreInfoLineFont.italic
        font.pixelSize: 25
        fontSizeMode: Text.HorizontalFit
        clip: true
        horizontalAlignment: Text.AlignRight
        minimumPixelSize: 5
        textFormat: Text.PlainText
        verticalAlignment: Text.AlignVCenter
    }
}
