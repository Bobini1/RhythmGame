import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    source: {
        let current = songList.current;
        if (!(current instanceof ChartData) || current.banner === "") {
            return "";
        }
        let dir = current.chartDirectory;
        if (dir[0] !== "/") {
            dir = "/" + dir;
        }
        let bannerWithoutExt = current.banner.replace(/\.[^/.]+$/, "");
        return "file://" + dir + bannerWithoutExt;
    }
    sourceSize.height: 80
    sourceSize.width: 300
}