import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    source: {
        let current = songList.current;
        if (!(current instanceof ChartData) || current.stageFile === "") {
            return "";
        }
        let dir = current.chartDirectory;
        if (dir[0] !== "/") {
            dir = "/" + dir;
        }
        let stageFileWithoutExt = current.stageFile.replace(/\.[^/.]+$/, "");
        return "file://" + dir + stageFileWithoutExt;
    }
    sourceSize.height: 450
    sourceSize.width: 600

    Loader {
        id: shadow

        active: stageFile.status === Image.Ready
        asynchronous: true

        sourceComponent: Component {
            Image {
                source: root.imagesUrl + "shadow.png"
                x: -80
                y: -60
            }
        }
    }
}
