import RhythmGameQml
import QtQuick

Item {
    property alias source: image.source
    property alias text: text.text

    height: image.height
    width: 250

    Image {
        id: image

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
    }
    Text {
        id: text

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: 25
        horizontalAlignment: Text.AlignRight
    }
}
