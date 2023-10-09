import QtQuick 2.0

Image {
    property bool scrollingText: parent.scrollingText

    source: root.iniImagesUrl + "folders.png/folder_green"

    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: "black"
        height: parent.height
        scrolling: isCurrentItem && parent.scrollingText
        text: display
        width: parent.width * 0.7
    }
    MouseArea {
        anchors.fill: parent

        onClicked: {
            pathView.currentIndex = index;
            pathView.open(display);
        }
    }
}
