import QtQuick

Rectangle {
    id: darkLabel
    color: "#404040"

    radius: 15

    property var highlightedKeys: []
    property alias text: textEdit.text

    TextEdit {
        id: textEdit
        readOnly: true
        font.pixelSize: 24
        color: "white"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 10
    }


    KeyImage {
        id: keyImage

        anchors.right: darkLabel.right
        anchors.rightMargin: 10
        anchors.verticalCenter: darkLabel.verticalCenter

        highlightedKeys: darkLabel.highlightedKeys
    }
}