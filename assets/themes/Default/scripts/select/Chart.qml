import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Image {
    id: image

    source: root.iniImagesUrl + "folders.png/white"

    Text {
        id: playlevelText

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 23
        color: "black"
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        text: display.playLevel
        width: normalTextMetrics.width

        TextMetrics {
            id: normalTextMetrics

            font: playlevelText.font
            text: "00"
        }
        TextMetrics {
            id: myTextMetrics

            font: playlevelText.font
            text: playlevelText.text
        }
    }
    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: (display.keymode === ChartData.Keymode.K14) ? "red" : "black"
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
