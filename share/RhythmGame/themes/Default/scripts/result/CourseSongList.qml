import QtQuick
import RhythmGameQml
import "../common"

Column {
    id: courseSongList
    required property var chartDatas

    ThemeFont {
        id: courseSongListFont
        fileName: root.themeVars.resultTitleFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    Repeater {
        model: {
            let ret = [];
            for (let chartData of courseSongList.chartDatas) {
                let infos = Rg.tables.search(chartData.md5);
                let title = chartData.title;
                let subtitle = chartData.subtitle;
                if (subtitle) {
                    title += " " + subtitle;
                }
                if (infos && infos.length > 0) {
                    title = infos[0].symbol + infos[0].levelName + " " + title;
                }
                title = title.replace(/\r\n|\n|\r/g, " ")
                ret.push(title);
            }
            return ret;
        }
        delegate: Text {
            text: modelData
            font: courseSongListFont.songMetadataFont({
                weight: courseSongListFont.fontWeight,
                variableAxes: courseSongListFont.variableAxes,
                italic: courseSongListFont.italic,
                pixelSize: 30
            }, modelData)
            style: Text.Outline
            color: "white"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
