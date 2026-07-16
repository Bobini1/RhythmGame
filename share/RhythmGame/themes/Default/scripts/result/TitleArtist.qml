import QtQuick
import "../common"

WindowBg {
    id: title

    required property var title
    required property var artist
    required property var subtitle
    required property var subartist

    ThemeFont {
        id: resultTitleFont
        fileName: root.themeVars.resultTitleFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    Text {
        id: titleText

        anchors.bottom: artistText.top
        anchors.left: parent.left
        anchors.right: parent.right
        horizontalAlignment: Text.AlignHCenter
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        elide: Text.ElideRight
        font: resultTitleFont.songMetadataFont({
            weight: resultTitleFont.fontWeight,
            variableAxes: resultTitleFont.variableAxes,
            italic: resultTitleFont.italic,
            pixelSize: 40
        }, titleText.text)
        text: (title.title + (title.subtitle ? (" " + title.subtitle) : "")).replace(/\r\n|\n|\r/g, " ")
    }
    Text {
        id: artistText

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 18
        anchors.left: parent.left
        anchors.right: parent.right
        horizontalAlignment: Text.AlignHCenter
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        elide: Text.ElideRight
        font: resultTitleFont.songMetadataFont({
            weight: resultTitleFont.fontWeight,
            variableAxes: resultTitleFont.variableAxes,
            italic: resultTitleFont.italic,
            pixelSize: 30
        }, artistText.text)
        text: title.artist + (title.subartist ? (" " + title.subartist) : "")
    }
}
