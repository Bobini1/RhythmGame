import QtQuick
import RhythmGameQml
import QtQuick.Controls
import QtQuick.Effects
import "../common/helpers.js" as Helpers

Item {
    id: grade

    required property var scoreWithBestPoints
    property var rankingTotalEntries: 0
    property var rankingPosition: 0
    property bool loading: false
    property string rankingLink
    property var provider

    width: 350
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

    MouseArea {
        anchors {
            left: rankingPosition.left
            right: total.right
            top: rankingPosition.top
            bottom: rankingPosition.bottom
        }
        cursorShape: enabled ? Qt.PointingHandCursor : undefined
        enabled: grade.rankingLink !== ""
        onClicked: {
            Qt.openUrlExternally(grade.rankingLink)
        }
    }

    Image {
        source: root.imagesUrl + "arrow"
        anchors.right: providerIcon.left
        anchors.rightMargin: 10
        anchors.top: providerIcon.top
        anchors.bottom: providerIcon.bottom
        fillMode: Image.PreserveAspectFit
        mirror: true
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
        }
    }
    Image {
        id: providerIcon
        source: {
            switch (grade.provider) {
                case OnlineRankingModel.RhythmGame:
                    return root.commonImagesUrl + "rhythmgame";
                case OnlineRankingModel.LR2IR:
                    return root.commonImagesUrl + "lr2ir";
                case OnlineRankingModel.Tachi:
                    return root.commonImagesUrl + "bokutachi";
            }
        }
        anchors.top: scoreRate.bottom
        anchors.topMargin: 15
        anchors.left: parent.left
        anchors.leftMargin: 30
        fillMode: Image.PreserveAspectFit
        width: rankingPosition.height
        height: rankingPosition.height
        sourceSize.width: 64
        sourceSize.height: 64
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
        }
        z: 1
    }
    Image {
        source: root.imagesUrl + "arrow"
        anchors.left: providerIcon.right
        anchors.leftMargin: 10
        anchors.top: providerIcon.top
        anchors.bottom: providerIcon.bottom
        fillMode: Image.PreserveAspectFit
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
        }
    }
    Image {
        id: rankingPosition
        anchors.top: scoreRate.bottom
        anchors.topMargin: 15
        anchors.left: parent.left
        anchors.leftMargin: 100
        asynchronous: true
        source: root.iniImagesUrl + "parts.png/ir"
    }
    // position/ total (total is supposed to be a bit smaller, same baseline
    Text {
        id: rankingPositionNumber
        anchors.baseline: rankingPosition.bottom
        anchors.baselineOffset: -1
        anchors.right: parent.right
        anchors.rightMargin: 130

        text: grade.rankingPosition + "/ ";

        font.pixelSize: 28
        horizontalAlignment: Text.AlignRight

        visible: !grade.loading
    }
    Text {
        id: total

        anchors.baseline: rankingPosition.bottom
        anchors.baselineOffset: -1
        anchors.right: exScoreNumber.right

        text: grade.rankingTotalEntries;

        font.pixelSize: 22
        horizontalAlignment: Text.AlignRight

        visible: !grade.loading
    }

    BusyIndicator {
        anchors.top: rankingPosition.top
        anchors.bottom: rankingPosition.bottom
        anchors.left: rankingPositionNumber.left
        anchors.right: total.right
        visible: grade.loading
    }
}