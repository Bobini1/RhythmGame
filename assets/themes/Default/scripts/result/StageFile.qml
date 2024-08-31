import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    height: sourceSize.height
    source: {
        let dir = chartData.chartDirectory;
        if (dir[0] !== "/") {
            dir = "/" + dir;
        }
        let stageFile = "file://" + dir + chartData.stageFile;
        return stageFile;
    }
    sourceSize.height: 192
    sourceSize.width: 256
    width: sourceSize.width

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.5
        z: -1
    }
}
