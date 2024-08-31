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
        return "file://" + dir + current.stageFile;
    }
    sourceSize.height: 450
    sourceSize.width: 600

    onSourceChanged: {
        shadow.active = false;
    }
    onStatusChanged: {
        if (status === Image.Ready) {
            shadow.active = true;
            return;
        }
    }

    Loader {
        id: shadow

        active: false
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
