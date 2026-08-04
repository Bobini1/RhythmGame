import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    source: songList.current instanceof ChartData
        ? songList.current.bannerSource
        : ""
    sourceSize.height: 80
    sourceSize.width: 300
}
