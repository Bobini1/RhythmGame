import QtQuick

WindowBg {
    id: title

    required property var title
    required property var artist
    required property var subtitle
    required property var subartist

    Text {
        id: titleText

        anchors.bottom: artistText.top
        anchors.horizontalCenter: parent.horizontalCenter
        font.pixelSize: 40
        text: title.title + (title.subtitle ? (" " + title.subtitle) : "")
    }
    Text {
        id: artistText

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 18
        anchors.horizontalCenter: parent.horizontalCenter
        font.pixelSize: 30
        text: title.artist + (title.subartist ? (" " + title.subartist) : "")
    }
}
