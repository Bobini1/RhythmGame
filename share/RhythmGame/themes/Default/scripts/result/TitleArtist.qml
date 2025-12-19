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
        anchors.left: parent.left
        anchors.right: parent.right
        horizontalAlignment: Text.AlignHCenter
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        elide: Text.ElideRight
        font.pixelSize: 40
        text: title.title + (title.subtitle ? (" " + title.subtitle) : "")
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
        font.pixelSize: 30
        text: title.artist + (title.subartist ? (" " + title.subartist) : "")
    }
}
