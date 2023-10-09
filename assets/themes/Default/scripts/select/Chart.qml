import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Image {
    id: image

    property string clearType: getClearType()
    property bool scrollingText

    function getClearType() {
        let scores = ScoreDb.getScoresForChart(display.sha256);
        let clearTypePriorities = ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"];
        let clearType = "NOPLAY";
        for (let i = 0; i < scores.length; i++) {
            let score = scores[i];
            if (clearTypePriorities.indexOf(score.clearType) > clearTypePriorities.indexOf(clearType)) {
                clearType = score.clearType;
            }
        }
        return clearType;
    }

    source: root.iniImagesUrl + "folders.png/white"

    Image {
        id: clearImage

        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.top: parent.top
        anchors.topMargin: 9
        source: root.iniImagesUrl + "parts.png/C_" + image.clearType
    }
    TextureText {
        id: playlevelText

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -270
        number: Math.min(display.playLevel, 99)
        srcBeforeDecimal: root.iniImagesUrl + "parts.png/s_" + root.getDiffColorInt(display.difficulty) + "_"
    }
    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: (display.keymode === ChartData.Keymode.K14) ? "red" : "black"
        height: parent.height
        scrolling: isCurrentItem && parent.scrollingText
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
