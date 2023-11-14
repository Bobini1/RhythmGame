import QtQuick
import "../common/helpers.js" as Helpers

WindowBg {
    id: score

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 36
        anchors.verticalCenter: parent.verticalCenter

        Column {
            spacing: 20
            width: 460

            Item {
                height: exScoreImg.sourceSize.height
                width: parent.width

                Image {
                    id: exScoreImg

                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    source: root.iniImagesUrl + "parts.png/ex_score"
                }
                Text {
                    id: exScoreText

                    anchors.baseline: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: 120
                    color: "lightgray"
                    font.pixelSize: 41
                    horizontalAlignment: Text.AlignRight
                    text: {
                        let points = result.result.points;
                        return "00000".slice(0, 5 - points.toString().length) + "<font color='DeepPink'>" + points + "</font>";
                    }
                }
                Text {
                    id: scoreRate

                    anchors.baseline: parent.bottom
                    anchors.leftMargin: 30
                    anchors.right: parent.right
                    font.pixelSize: 25
                    horizontalAlignment: Text.AlignRight
                    text: (result.result.maxPoints ? (result.result.points / result.result.maxPoints * 100).toFixed(2) : "0.00") + "%"
                }
            }
            Item {
                height: hiScoreImg.sourceSize.height
                width: parent.width

                Image {
                    id: hiScoreImg

                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    source: root.iniImagesUrl + "parts.png/hi_score"
                }
                Text {
                    id: hiScoreText

                    anchors.baseline: parent.bottom
                    anchors.right: parent.right
                    anchors.rightMargin: 120
                    color: "lightgray"
                    font.pixelSize: 34
                    horizontalAlignment: Text.AlignRight
                    text: {
                        let points = root.oldBestPointsScore ? root.oldBestPointsScore.points : 0;
                        return "00000".slice(0, 5 - points.toString().length) + "<font color='DeepPink'>" + points + "</font>";
                    }
                }
                Text {
                    id: hiScoreDelta

                    anchors.baseline: parent.bottom
                    anchors.left: hiScoreText.right
                    anchors.leftMargin: 41
                    color: {
                        let delta = result.result.points - (root.oldBestPointsScore ? root.oldBestPointsScore.points : 0);
                        return delta > 0 ? "darkgreen" : (delta < 0 ? "FireBrick" : "black");
                    }
                    font.pixelSize: 25
                    horizontalAlignment: Text.AlignLeft
                    text: {
                        let delta = result.result.points - (root.oldBestPointsScore ? root.oldBestPointsScore.points : 0);
                        if (delta > 0) {
                            return "+" + delta;
                        } else if (delta < 0) {
                            return "–" + (-delta);
                        } else {
                            return "";
                        }
                    }
                }
            }
        }
        Image {
            id: gradeImage

            anchors.verticalCenter: parent.verticalCenter
            source: root.iniImagesUrl + "parts.png/" + Helpers.getGrade(result.result.points, result.result.maxPoints)
        }
    }
}
