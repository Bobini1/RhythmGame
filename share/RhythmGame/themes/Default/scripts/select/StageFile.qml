import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    source: songList.current instanceof ChartData
        ? songList.current.stageFileSource
        : ""
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
