import QtQuick
import "../common"

Item {
    id: titleDisplay

    property string title: ""
    property string subtitle: ""
    property string fontFile: "file:NotoSans-VariableFont_wdth,wght.ttf"
    property bool contentVisible: true

    ThemeFont {
        id: titleDisplayFont
        fileName: titleDisplay.fontFile
    }

    Column {
        anchors.fill: parent
        spacing: 2
        visible: titleDisplay.contentVisible

        // Title
        Text {
            width: parent.width
            height: titleDisplay.subtitle !== "" ? parent.height * 0.58 : parent.height
            text: titleDisplay.title
            font: titleDisplayFont.songMetadataFont({
                pixelSize: height * 0.85,
                weight: titleDisplayFont.boldFontWeight,
                variableAxes: titleDisplayFont.boldVariableAxes,
                italic: titleDisplayFont.italic
            }, titleDisplay.title)
            fontSizeMode: Text.Fit
            minimumPixelSize: 6
            color: "white"
            elide: Text.ElideRight
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }

        // Subtitle
        Text {
            width: parent.width
            height: parent.height * 0.38
            text: titleDisplay.subtitle
            font: titleDisplayFont.songMetadataFont({
                pixelSize: height * 0.8,
                weight: titleDisplayFont.fontWeight,
                variableAxes: titleDisplayFont.variableAxes,
                italic: titleDisplayFont.italic
            }, titleDisplay.subtitle)
            fontSizeMode: Text.Fit
            minimumPixelSize: 6
            color: "#cccccc"
            elide: Text.ElideRight
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            visible: titleDisplay.subtitle !== ""
        }
    }
}



