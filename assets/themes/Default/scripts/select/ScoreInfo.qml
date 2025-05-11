import QtQuick
import RhythmGameQml

Row {
    id: scoreInfoRow
    required property var scoreWithBestPoints
    required property var bestStats
    required property var current


    Column {
        spacing: 12

        Repeater {
            model: ["perfect", "great", "good", "bad", "poor"]

            delegate: ScoreInfoLine {
                source: root.iniImagesUrl + "parts.png/" + modelData
                text: {
                    if (scoreInfoRow.scoreWithBestPoints !== null) {
                        let index = ["poor", "empty_poor", "bad", "good", "great", "perfect"].indexOf(modelData);
                        return scoreInfoRow.scoreWithBestPoints.result.judgementCounts[index];
                    } else {
                        return 0;
                    }
                }
            }
        }
    }
    Column {
        spacing: 12

        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/ex_score"
            text: scoreInfoRow.scoreWithBestPoints?.points || 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/hi_score"
            text: (100 * (scoreInfoRow.scoreWithBestPoints?.points || 0) / (scoreInfoRow.scoreWithBestPoints?.maxPoints || 1)).toFixed(2) + "%"
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/combo"
            text: scoreInfoRow.bestStats?.maxCombo || 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/total_notes"
            text: scoreInfoRow.current instanceof ChartData ? scoreInfoRow.current.normalNoteCount + scoreInfoRow.current.lnCount : 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/miss_count"
            text: scoreInfoRow.bestStats?.missCount || 0
        }
    }
}
