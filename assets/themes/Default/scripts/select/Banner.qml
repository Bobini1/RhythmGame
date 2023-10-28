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
        let banner = "file://" + currentItem.directory + currentItem.banner;
        return FileValidator.exists(banner) ? banner : "";
    }
    sourceSize.height: 80
    sourceSize.width: 300
}