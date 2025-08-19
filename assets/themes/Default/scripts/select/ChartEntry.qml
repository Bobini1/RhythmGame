pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Layouts
import RhythmGameQml
import "../common/helpers.js" as Helpers

Image {
    id: image

    property string clearType: Helpers.getClearType(scores)
    required property var scores
    property var bestStats: Helpers.getBestStats(scores)
    property var scoreWithBestPoints: Helpers.getScoreWithBestPoints(scores)
    property bool scrollingText: false
    property bool isCurrentItem: false
    readonly property bool isCourse: modelData instanceof course

    asynchronous: true
    source: root.iniImagesUrl + "folders.png/" + (isCourse ? "folder_pink" : "white")

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
        visible: !image.isCourse

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
    Image {
        visible: modelData.lnCount > 0
        source: root.iniImagesUrl + "folders.png/ln"
        asynchronous: true
        anchors.horizontalCenter: parent.left
        anchors.horizontalCenterOffset: 95
        anchors.verticalCenter: playlevelText.verticalCenter
    }
    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: (modelData instanceof ChartData || modelData instanceof course) ? "black" : "red"
        height: parent.height
        scrolling: image.isCurrentItem && image.scrollingText
        text: (modelData.title || modelData.name || "") + (modelData.subtitle ? (" " + modelData.subtitle) : "")
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
