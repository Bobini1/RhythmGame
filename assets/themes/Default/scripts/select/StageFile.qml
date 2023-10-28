import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    source: {
        let currentItem = songList.currentItem
        if (!(currentItem instanceof ChartData) || currentItem.stageFile === "") {
            return "";
        }
        let stageFile = "file://" + currentItem.directory + currentItem.stageFile;
        return FileValidator.exists(stageFile) ? stageFile : "";
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
        if (status === Image.Error) {
            let currentItem = songList.model.at(songList.currentIndex);
            console.warn("Could not load stagefile for " + currentItem.path + ":", currentItem.stageFile);
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
