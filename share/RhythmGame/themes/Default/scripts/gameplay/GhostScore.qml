import QtQuick
import RhythmGameQml

Item {
    id: ghostScore
    property real points
    property alias alignment: pointsText.horizontalAlignment

    Text {
        id: pointsText
        color: ghostScore.points > 0 ? "white" : "red"
        text: ghostScore.points > 0 ? "+" + ghostScore.points : ghostScore.points
        anchors.centerIn: parent
        fontSizeMode: Text.VerticalFit
        textFormat: Text.PlainText
        font.pixelSize: 48
        minimumPixelSize: 6
        horizontalAlignment: Text.AlignHCenter
    }
}