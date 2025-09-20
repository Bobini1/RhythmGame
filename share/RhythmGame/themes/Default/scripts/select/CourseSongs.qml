pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQml

Column {
    id: courseSongsColumn
    property var chartDatas: Rg.chartLoader.loadChartDataFromDb(songList.current.md5s)
    property var canPlay: songList.current.md5s.every(md5 => chartDatas[md5] !== undefined)
    Repeater {
        model: {
            let md5s = songList.current.md5s;
            let chartDatas = courseSongsColumn.chartDatas;
            let names = []
            for (let md5 of md5s) {
                let info = Rg.tables.search(md5);
                let chartData = chartDatas[md5];
                let red = (chartData === undefined);
                if (info.length) {
                    names.push({red, text: info[0].symbol + info[0].levelName + " " + info[0].entry.title + (info[0].entry.subtitle ? " " + info[0].entry.subtitle : "")});
                } else {
                    if (chartData !== undefined) {
                        names.push({red, text: chartData.title + (chartData.subtitle ? " " + chartData.subtitle : "")});
                    } else {
                        names.push({red, text: before + md5 + after});
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
                font.pixelSize: 25
                text: modelData.text
                scrolling: songList.scrollingText
            }
        }
    }
}