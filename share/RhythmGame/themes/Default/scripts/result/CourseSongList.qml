import QtQuick

Column {
    id: courseSongList
    required property var chartDatas
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
                ret.push(title);
            }
            return ret;
        }
        delegate: Text {
            text: modelData
            font.pixelSize: 30
            style: Text.Outline
            color: "white"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}