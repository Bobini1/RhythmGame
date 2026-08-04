import QtQuick
import RhythmGameQml

Image {
    asynchronous: true
    height: sourceSize.height

    required property string chartDirectory
    required property string stageFileName
    source: Rg.songAssets.imageSource(chartDirectory, stageFileName)
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
