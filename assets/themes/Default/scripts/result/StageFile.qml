import QtQuick
import RhythmGameQml

Image {
    id: stageFile

    asynchronous: true
    height: sourceSize.height
    source: {
        let stageFile = "file://" + chartData.directory + chartData.stageFile;
        return FileQuery.exists(stageFile) ? stageFile : "";
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
