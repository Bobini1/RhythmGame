import QtQuick
import RhythmGameQml
import QtQuick.Effects
import QtQuick.Controls

Column {
    id: scoreColumn

    required property real points
    required property real maxPoints
    required property real oldBestPoints
    required property var judgementCounts
    required property var oldBestStats
    required property var earlyLate
    required property int maxCombo
    required property var clearType
    required property var oldBestClear
    required property string websiteUrl
    property alias oldRankingPosition: rankingPosition.oldRankingPosition
    property alias newRankingPosition: rankingPosition.newRankingPosition
    property alias totalEntries: rankingPosition.totalEntries
    property alias loading: rankingPosition.loading
    property alias scoreSubmissionFailed: rankingPosition.scoreSubmissionFailed
    property string rankingUrl
    property var provider

    signal leftClicked()
    signal rightClicked()

    Score {
        height: 150
        width: 668

        points: scoreColumn.points
        maxPoints: scoreColumn.maxPoints
        oldBestPoints: scoreColumn.oldBestPoints
    }
    Row {
        width: 668
        height: 104
        spacing: 3

        Image {
            source: root.imagesUrl + "arrow"
            width: 32
            height: parent.height
            fillMode: Image.PreserveAspectFit
            mirror: true
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
            }
            MouseArea {
                anchors.fill: parent

                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    scoreColumn.rightClicked();
                }
            }
        }

        Item {
            width: 290
            height: parent.height

            RankingPosition {
                id: rankingPosition
                anchors.fill: parent
                anchors.margins: -14

                Image {
                    source: {
                        switch (scoreColumn.provider) {
                            case OnlineRankingModel.RhythmGame:
                                return root.commonImagesUrl + "rhythmgame";
                            case OnlineRankingModel.LR2IR:
                                return root.commonImagesUrl + "lr2ir";
                            case OnlineRankingModel.Tachi:
                                return root.commonImagesUrl + "bokutachi";
                        }
                    }
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                    }
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 20
                    height: 32
                    width: 32
                    sourceSize.width: 64
                    sourceSize.height: 64
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: 14
                    cursorShape: scoreColumn.rankingUrl ? Qt.PointingHandCursor : undefined
                    enabled: scoreColumn.rankingUrl !== ""

                    onClicked: {
                        Qt.openUrlExternally(scoreColumn.rankingUrl);
                    }
                }
            }
        }
        Image {
            source: root.imagesUrl + "arrow"
            width: 32
            height: parent.height
            fillMode: Image.PreserveAspectFit
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor

                onClicked: {
                    scoreColumn.rightClicked();
                }
            }
        }

        Item {
            width: 290
            height: parent.height

            LampDiff {
                anchors.fill: parent
                anchors.margins: -14
                clearType: scoreColumn.clearType
                oldBestClear: scoreColumn.oldBestClear
            }
        }
    }
    HitInfo {
        height: 180
        width: 668
        maxCombo: scoreColumn.maxCombo
        judgementCounts: scoreColumn.judgementCounts
        oldBestStats: scoreColumn.oldBestStats
    }
    HitStats {
        height: 364
        width: 668
        judgementCounts: scoreColumn.judgementCounts
        earlyLate: scoreColumn.earlyLate
    }
}


