import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Image {
    id: image

    source: root.iniImagesUrl + "folders.png/white"

    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: (display.keymode === ChartData.Keymode.K14) ? "red" : (isCurrentItem ? "yellow" : "black")
        height: parent.height
        scrolling: isCurrentItem
        text: display.title + (display.subtitle ? (" " + display.subtitle) : "")
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
