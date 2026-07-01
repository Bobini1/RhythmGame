import QtQuick
import "../common"

Item {
    id: titleDisplay

    property string title: ""
    property string subtitle: ""
    property string fontFile: "file:NotoSansJP-VariableFont_wght.ttf"
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
            font.pixelSize: height * 0.85
            font.family: titleDisplayFont.fontFamily
            font.weight: titleDisplayFont.boldFontWeight
            font.italic: titleDisplayFont.italic
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
            font.pixelSize: height * 0.8
            font.family: titleDisplayFont.fontFamily
            font.weight: titleDisplayFont.fontWeight
            font.italic: titleDisplayFont.italic
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



