import QtQuick 2.0
import RhythmGameQml

Image {
    source: root.iniImagesUrl + "folders.png/white"

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 30
        anchors.verticalCenter: parent.verticalCenter
        clip: true
        color: isCurrentItem ? "yellow" : "black"
        font.pixelSize: 20
        text: display.title + (display.subtitle ? (" " + display.subtitle) : "")

        Component.onCompleted: {
            if (display.keymode === ChartData.Keymode.K14) {
                color = "red";
            }
        }
    }
    MouseArea {
        anchors.fill: parent

        onClicked: {
            pathView.open(display);
            pathView.currentIndex = index;
        }
    }
}
