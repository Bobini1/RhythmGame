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

    Score {
        height: 150
        width: 668

        points: scoreColumn.points
        maxPoints: scoreColumn.maxPoints
        oldBestPoints: scoreColumn.oldBestPoints
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
