import QtQuick
import RhythmGameQml
import "../common"

Text {
    id: pointsText
    function padToFour(number) {
        if (number<=9999) { number = ("000"+number).slice(-4); }
        return number;
    }
    property real points
    property string fontFile: "file:NotoSansJP-VariableFont_wght.ttf"

    ThemeFont {
        id: ghostScoreFont
        fileName: pointsText.fontFile
        fallbackFamily: "Monospace"
    }

    color: points >= 0 ? "white" : "red"
    text: (points >= 0 ? "+" : "-") + padToFour(Math.abs(Math.round(points)))
    fontSizeMode: Text.VerticalFit
    textFormat: Text.PlainText
    font.pixelSize: 1000
    font.family: ghostScoreFont.fontFamily
    font.weight: ghostScoreFont.fontWeight
    font.italic: ghostScoreFont.italic
    minimumPixelSize: 6
    horizontalAlignment: Text.AlignHCenter
    width: fontInfo.width
}
