import QtQuick 2.0

Image {
    source: root.iniImagesUrl + "folders.png/folder_green"

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 30
        anchors.verticalCenter: parent.verticalCenter
        color: isCurrentItem ? "yellow" : "black"
        font.pixelSize: 20
        text: display
    }
    MouseArea {
        anchors.fill: parent

        onClicked: {
            pathView.open(display);
            pathView.currentIndex = index;
        }
    }
}
