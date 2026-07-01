import QtQuick
import RhythmGameQml
import "../common"

Column {
    id: courseSongList
    required property var chartDatas

    ThemeFont {
        id: courseSongListFont
        fileName: root.themeVars.resultTitleFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
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
            font.family: courseSongListFont.fontFamily
            font.weight: courseSongListFont.fontWeight
            font.italic: courseSongListFont.italic
            font.pixelSize: 30
            style: Text.Outline
            color: "white"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
