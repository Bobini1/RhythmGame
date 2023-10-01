import QtQuick 2.0

Image {
    source: root.iniImagesUrl + "folders.png/folder_green"

    NameLabel {
        color: isCurrentItem ? "yellow" : "black"
        scrolling: isCurrentItem
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
