import QtQuick
import RhythmGameQml

Image {
    asynchronous: true
    height: sourceSize.height

    required property string chartDirectory
    required property string stageFileName
    source: {
        if (stageFileName === "") {
            return "";
        }
        let dir = chartDirectory;
        if (dir[0] !== "/") {
            dir = "/" + dir;
        }
        let stageFile = "file://" + dir + stageFileName;
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
