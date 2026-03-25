import QtQuick
import RhythmGameQml
import QtQuick.Controls
import QtQuick.Effects
import "../common/helpers.js" as Helpers

Item {
    id: grade

    required property var scoreWithBestPoints

    Image {
        id: gradeImage

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 100
        asynchronous: true
        source: root.iniImagesUrl + "parts.png/" + Helpers.getGrade(grade.scoreWithBestPoints.result.points, grade.scoreWithBestPoints.result.maxPoints)
    }

    Image {
        id: exScore

        anchors.top: gradeImage.bottom
        anchors.topMargin: 24
        asynchronous: true
        source: root.iniImagesUrl + "parts.png/ex_score"
    }

    Text {
        id: exScoreNumber
        anchors.baseline: exScore.bottom
        anchors.left: exScore.right
        anchors.leftMargin: 55
        anchors.right: parent.right
        anchors.rightMargin: 75

        text: grade.scoreWithBestPoints.result.points

        font.pixelSize: 42
        color: "#ff0066"
        horizontalAlignment: Text.AlignRight
    }

    Image {
        id: scoreRate
        anchors.top: exScore.bottom
        anchors.topMargin: 15
        asynchronous: true
        source: root.iniImagesUrl + "parts.png/score_rate"
    }

    Text {
        anchors.baseline: scoreRate.bottom
        anchors.baselineOffset: -1
        anchors.left: scoreRate.right
        anchors.leftMargin: 55
        anchors.right: exScoreNumber.right

        text: {
            let points = grade.scoreWithBestPoints.result.points;
            let maxPoints = grade.scoreWithBestPoints.result.maxPoints;
            let ratio = points / maxPoints;

            return (ratio * 100).toFixed(2) + "%";
        }

        font.pixelSize: 28
        horizontalAlignment: Text.AlignRight
    }
}