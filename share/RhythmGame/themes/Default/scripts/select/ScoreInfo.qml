import QtQuick
import RhythmGameQml

Row {
    id: scoreInfoRow
    required property var scoreWithBestPoints
    required property var bestStats
    required property var current
    property string fontFile: "file:NotoSansJP-VariableFont_wght.ttf"
    readonly property int lineWidth: 264


    Column {
        spacing: 12

        Repeater {
            model: ["perfect", "great", "good", "bad", "poor"]

            delegate: ScoreInfoLine {
                source: root.iniImagesUrl + "parts.png/" + modelData
                fontFile: scoreInfoRow.fontFile
                width: scoreInfoRow.lineWidth
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
            fontFile: scoreInfoRow.fontFile
            width: scoreInfoRow.lineWidth
            text: scoreInfoRow.scoreWithBestPoints?.result?.points || 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/hi_score"
            fontFile: scoreInfoRow.fontFile
            width: scoreInfoRow.lineWidth
            text: (100 * (scoreInfoRow.scoreWithBestPoints?.result?.points || 0) / (scoreInfoRow.scoreWithBestPoints?.result?.maxPoints || 1)).toFixed(2) + "%"
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/combo"
            fontFile: scoreInfoRow.fontFile
            width: scoreInfoRow.lineWidth
            text: scoreInfoRow.bestStats?.maxCombo || 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/total_notes"
            fontFile: scoreInfoRow.fontFile
            valueLeftMargin: 169
            width: scoreInfoRow.lineWidth
            text: scoreInfoRow.current instanceof ChartData ? scoreInfoRow.current.normalNoteCount + scoreInfoRow.current.lnCount + scoreInfoRow.current.bssCount + scoreInfoRow.current.scratchCount : 0
        }
        ScoreInfoLine {
            source: root.iniImagesUrl + "parts.png/miss_count"
            fontFile: scoreInfoRow.fontFile
            width: scoreInfoRow.lineWidth
            text: scoreInfoRow.bestStats?.missCount || 0
        }
    }
}
