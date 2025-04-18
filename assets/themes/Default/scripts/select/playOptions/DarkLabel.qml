import QtQuick

Rectangle {
    id: darkLabel
    color: "#404040"

    radius: 15
    implicitHeight: 40

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
        anchors.leftMargin: 20
    }


    KeyImage {
        id: keyImage

        anchors.right: darkLabel.right
        anchors.rightMargin: 20
        anchors.verticalCenter: darkLabel.verticalCenter

        highlightedKeys: darkLabel.highlightedKeys
    }
}