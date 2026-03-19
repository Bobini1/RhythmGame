import QtQuick

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
    property alias oldRankingPosition: rankingPosition.oldRankingPosition
    property alias newRankingPosition: rankingPosition.newRankingPosition
    property alias totalEntries: rankingPosition.totalEntries
    property alias loading: rankingPosition.loading
    property alias scoreSubmissionFailed: rankingPosition.scoreSubmissionFailed
    property string rankingUrl

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

        RankingPosition {
            id: rankingPosition
            width: 318
            height: parent.height

            oldRankingPosition: 0
            newRankingPosition: 1
            totalEntries: 100
            MouseArea {
                anchors.fill: parent
                cursorShape: scoreColumn.rankingUrl ? Qt.PointingHandCursor : undefined
                enabled: scoreColumn.rankingUrl !== ""

                onClicked: {
                    Qt.openUrlExternally(scoreColumn.rankingUrl);
                }
            }
        }
        LampDiff {
            width: 350
            height: parent.height

            clearType: scoreColumn.clearType
            oldBestClear: scoreColumn.oldBestClear
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


