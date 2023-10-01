import QtQuick 2.0

Image {
    source: root.iniImagesUrl + "folders.png/folder_green"

    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: isCurrentItem ? "yellow" : "black"
        height: parent.height
        scrolling: isCurrentItem
        text: display
        width: parent.width * 0.7
    }
    MouseArea {
        anchors.fill: parent

        onClicked: {
            pathView.open(display);
            pathView.currentIndex = index;
        }
    }
}
