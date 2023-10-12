import QtQuick
import RhythmGameQml

Row {
    Column {
        spacing: 12

        Repeater {
            model: ["perfect", "great", "good", "bad", "poor"]

            delegate: ScoreInfoLine {
                source: root.iniImagesUrl + "parts.png/" + modelData
                text: {
                    if (root.scoreWithBestPoints !== null) {
                        let index = ["poor", "empty_poor", "bad", "good", "great", "perfect"].indexOf(modelData);
                        return root.scoreWithBestPoints.judgementCounts[index];
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
            text: root.scoreWithBestPoints !== null ? root.scoreWithBestPoints.points : 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/hi_score"
            text: root.scoreWithBestPoints !== null ? (100 * root.scoreWithBestPoints.points / root.scoreWithBestPoints.maxPoints).toFixed(2) + "%" : "0.00%"
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/combo"
            text: root.scoreWithBestPoints !== null ? root.scoreWithBestPoints.maxCombo : 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/total_notes"
            text: songList.current instanceof ChartData ? songList.current.noteCount : 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/miss_count"
            text: root.scoreWithBestPoints !== null ? root.scoreWithBestPoints.judgementCounts[Judgement.Poor] + root.scoreWithBestPoints.judgementCounts[Judgement.EmptyPoor] : 0
        }
    }
}
