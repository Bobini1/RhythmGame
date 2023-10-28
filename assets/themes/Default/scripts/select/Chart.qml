import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Image {
    id: image

    property string clearType: getClearType()
    property list<BmsResult> scores: ScoreDb.getScoresForChart(modelData.sha256)
    property bool scrollingText: parent.scrollingText

    function getClearType() {
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
    function refreshScores() {
        scores = ScoreDb.getScoresForChart(display.sha256);
        clearType = getClearType();
    }

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

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -270
        number: Math.min(modelData.playLevel, 99)
        srcBeforeDecimal: root.iniImagesUrl + "parts.png/s_" + root.getDiffColorInt(modelData.difficulty) + "_"
    }
    NameLabel {
        anchors.right: parent.right
        anchors.rightMargin: 30
        color: (modelData.keymode === ChartData.Keymode.K14) ? "red" : "black"
        height: parent.height
        scrolling: isCurrentItem && parent.scrollingText
        text: modelData.title + (modelData.subtitle ? (" " + modelData.subtitle) : "")
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
