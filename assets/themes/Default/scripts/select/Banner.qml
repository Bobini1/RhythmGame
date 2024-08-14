import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    source: {
        let currentItem = songList.current;
        if (!(currentItem instanceof ChartData) || currentItem.banner === "") {
            return "";
        }
        let dir = currentItem.chartDirectory;
        if (dir[0] !== "/") {
            dir = "/" + dir;
        }
        return "file://" + dir + currentItem.banner;
    }
    sourceSize.height: 80
    sourceSize.width: 300
}