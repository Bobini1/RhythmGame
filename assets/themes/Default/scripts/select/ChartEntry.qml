import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import "../common/helpers.js" as Helpers

Image {
    id: image

    property string clearType: Helpers.getClearType(scores)
    required property list<BmsScore> scores
    property var bestStats: Helpers.getBestStats(scores)
    property BmsScore scoreWithBestPoints: Helpers.getScoreWithBestPoints(scores)
    property bool scrollingText: false
    property bool isCurrentItem: false

    asynchronous: true
    source: root.iniImagesUrl + "folders.png/white"

    Image {
        id: clearImage

        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.top: parent.top
        anchors.topMargin: 9
        asynchronous: true
        source: root.iniImagesUrl + "parts.png/C_" + image.clearType
    }
    TextureText {
        id: playlevelText

        function getDiffColorInt(diff) {
            switch (diff) {
                case 1:
                    return "green";
                case 2:
                    return "blue";
                case 3:
                    return "orange";
                case 4:
                    return "red";
                default:
                    return "purple";
            }
        }

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -270
        number: Math.min(modelData.playLevel || 0, 99)
        srcBeforeDecimal: root.iniImagesUrl + "parts.png/s_" + getDiffColorInt(modelData.difficulty) + "_"
    }
    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: (modelData.keymode === ChartData.Keymode.K14) ? "red" : "black"
        height: parent.height
        scrolling: image.isCurrentItem && image.scrollingText
        text: modelData.title + (modelData.subtitle ? (" " + modelData.subtitle) : "")
        width: parent.width * 0.7
    }
    MouseArea {
        anchors.fill: parent

        onClicked: {
            pathView.goForward(modelData);
            pathView.positionViewAtIndex(index + 1, PathView.Center);
        }
    }
}
