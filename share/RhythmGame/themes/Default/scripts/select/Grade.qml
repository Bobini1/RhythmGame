import QtQuick
import RhythmGameQml
import "../common/helpers.js" as Helpers

Item {
    id: grade

    required property var scoreWithBestPoints

    width: 300
    height: 400

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
        anchors.topMargin: 50
        asynchronous: true
        source: root.iniImagesUrl + "parts.png/ex_score"
    }

    Text {
        anchors.baseline: exScore.bottom
        anchors.left: exScore.right
        anchors.leftMargin: 55
        anchors.right: parent.right
        anchors.rightMargin: 55

        text: grade.scoreWithBestPoints.result.points

        font.pixelSize: 48
        color: "#ff0066"
        horizontalAlignment: Text.AlignRight
    }

    Image {
        id: nextRank
        anchors.top: exScore.bottom
        anchors.topMargin: 15
        asynchronous: true
        source: root.iniImagesUrl + "parts.png/next_rank"
    }

    Text {
        anchors.baseline: nextRank.bottom
        anchors.baselineOffset: -1
        anchors.left: nextRank.right
        anchors.leftMargin: 55
        anchors.right: parent.right
        anchors.rightMargin: 55

        text: {
            let points = grade.scoreWithBestPoints.result.points;
            let maxPoints = grade.scoreWithBestPoints.result.maxPoints;
            let ratio = points / maxPoints;

            const targets = [
                { min: 0.88, target: 1.00 },
                { min: 0.77, target: 0.88 },
                { min: 0.66, target: 0.77 },
                { min: 0.55, target: 0.66 },
                { min: 0.44, target: 0.55 },
                { min: 0.33, target: 0.44 },
                { min: 0.22, target: 0.33 }
            ];

            for (const step of targets) {
                if (ratio >= step.min)
                    return Math.ceil(step.target * maxPoints - points);
            }
            return Math.ceil(0.22 * maxPoints - points);
        }

        font.pixelSize: 25
        horizontalAlignment: Text.AlignRight
    }
}