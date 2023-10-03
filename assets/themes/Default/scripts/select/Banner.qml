import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    source: {
        let currentItem = songList.model.at(songList.currentIndex);
        if (!(currentItem instanceof ChartData) || currentItem.banner === "") {
            return "";
        }
        return "file://" + currentItem.directory + currentItem.banner;
    }
    sourceSize.height: 80
    sourceSize.width: 300
}