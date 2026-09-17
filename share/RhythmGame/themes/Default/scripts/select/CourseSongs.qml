pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQml

Column {
    id: courseSongsColumn
    readonly property var chartDatas: songList.current.loadCharts()
    readonly property bool canPlay: !songList.current.unavailableReason
        && chartDatas.length > 0 && chartDatas.every(chart => chart instanceof ChartData)
    Repeater {
        model: {
            let md5s = songList.current.md5s;
            let chartDatas = courseSongsColumn.chartDatas;
            let names = []
            for (let index = 0; index < md5s.length; ++index) {
                const md5 = md5s[index];
                let info = Rg.tables.search(md5);
                let chartData = chartDatas[index];
                let red = !(chartData instanceof ChartData);
                if (info.length) {
                    names.push({red, text: (info[0].symbol + info[0].levelName + " " + info[0].entry.title + (info[0].entry.subtitle ? " " + info[0].entry.subtitle : "")).replace(/\r\n|\n|\r/g, " ")});
                } else {
                    if (!red) {
                        names.push({red, text: (chartData.title + (chartData.subtitle ? " " + chartData.subtitle : "")).replace(/\r\n|\n|\r/g, " ")});
                    } else {
                        names.push({red, text: String(chartData)});
                    }
                }
            }
            return names;
        }
        delegate: Image {
            source: root.iniImagesUrl + "parts.png/course_chart_bar"
            Image {
                source: root.iniImagesUrl + "parts.png/" + (index+1) + "th"
                anchors.top: parent.top
                anchors.topMargin: 4
            }
            NameLabel {
                anchors.left: parent.left
                anchors.leftMargin: 192
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                color: modelData.red ? "red" : "black"
                fontPixelSize: 25
                text: modelData.text
                scrolling: songList.scrollingText
            }
        }
    }
    Text {
        width: courseSongsColumn.width
        text: songList.current.unavailableReason
        visible: text.length > 0
        color: "#ffc979"
        wrapMode: Text.Wrap
    }
}
