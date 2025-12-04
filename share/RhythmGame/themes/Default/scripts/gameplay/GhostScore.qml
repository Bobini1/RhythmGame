import QtQuick
import RhythmGameQml

Text {
    id: pointsText
    function padToFour(number) {
        if (number<=9999) { number = ("000"+number).slice(-4); }
        return number;
    }
    property real points
    color: points >= 0 ? "white" : "red"
    text: (points >= 0 ? "+" : "-") + padToFour(Math.abs(Math.round(points)))
    fontSizeMode: Text.VerticalFit
    textFormat: Text.PlainText
    font.pixelSize: 1000
    minimumPixelSize: 6
    horizontalAlignment: Text.AlignHCenter
    width: fontInfo.width
    font.family: "Monospace"
}