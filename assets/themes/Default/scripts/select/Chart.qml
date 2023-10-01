import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Image {
    id: image

    source: root.iniImagesUrl + "folders.png/white"

    NameLabel {
        color: (display.keymode === ChartData.Keymode.K14) ? "red" : (isCurrentItem ? "yellow" : "black")
        scrolling: isCurrentItem
        text: display.title + (display.subtitle ? (" " + display.subtitle) : "")
    }
    MouseArea {
        anchors.fill: parent

        onClicked: {
            pathView.open(display);
            pathView.currentIndex = index;
        }
    }
}
