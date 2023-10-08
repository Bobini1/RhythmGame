import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Image {
    id: image

    property string clearType: getClearType()

    function getClearType() {
        let scores = ScoreDb.getScoresForChart(display.sha256);
        let clearTypePriorities = ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC"];
        let clearType = "NOPLAY";
        for (let i = 0; i < scores.length; i++) {
            let score = scores[i];
            if (score.clearType === "FC") {
                if (score.points === score.maxPoints) {
                    return "MAX";
                }
                let judgementCounts = score.judgementCounts;
                if (judgementCounts[Judgement.Perfect] + judgementCounts[Judgement.Great] === score.maxHits) {
                    return "PERFECT";
                }
                return "FC";
            }
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
        source: root.iniImagesUrl + "parts.png/" + image.clearType
    }
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
        scrolling: isCurrentItem && pathView.scrollingText
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
