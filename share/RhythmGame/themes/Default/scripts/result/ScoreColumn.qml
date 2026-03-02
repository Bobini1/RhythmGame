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
            width: 318
            height: parent.height

            oldRankingPosition: 0
            newRankingPosition: 1
            totalEntries: 100
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


