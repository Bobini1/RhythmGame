import QtQuick

WindowBg {
    id: title

    Text {
        id: titleText

        anchors.bottom: artistText.top
        anchors.horizontalCenter: parent.horizontalCenter
        font.pixelSize: 40
        text: chartData.title + (chartData.subtitle ? (" " + chartData.subtitle) : "")
    }
    Text {
        id: artistText

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 18
        anchors.horizontalCenter: parent.horizontalCenter
        font.pixelSize: 30
        text: chartData.artist + (chartData.subartist ? (" " + chartData.subartist) : "")
    }
}
