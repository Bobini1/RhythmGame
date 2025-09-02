import QtMultimedia
import QtQuick
import RhythmGameQml

Item {
    readonly property alias baseSink: videoBase.videoSink
    readonly property alias layerSink: videoLayer.videoSink
    readonly property alias layer2Sink: videoLayer2.videoSink
    readonly property alias poorSink: videoPoor.videoSink
    property alias poorVisible: videoPoor.visible

    function clearOutput() {
        // available since Qt 6.9
        if ("clearOutput" in videoBase) {
            videoBase.clearOutput();
            videoLayer.clearOutput();
            videoLayer2.clearOutput();
            videoPoor.clearOutput();
        }
    }

    clip: true

    Item {
        anchors.centerIn: parent
        height: Math.max(videoBase.sourceRect.height, videoLayer.sourceRect.height, videoLayer2.sourceRect.height, videoPoor.sourceRect.height, 256)
        layer.enabled: true
        layer.smooth: true
        scale: parent.height / height
        width: Math.max(videoBase.sourceRect.width, videoLayer.sourceRect.width, videoLayer2.sourceRect.width, videoPoor.sourceRect.width, 256)

        VideoOutput {
            id: videoBase

            anchors.centerIn: parent
            height: sourceRect.height ? sourceRect.height : 256
            width: sourceRect.width ? sourceRect.width : 256
        }
        VideoOutput {
            id: videoLayer

            anchors.centerIn: videoBase
            height: sourceRect.height ? sourceRect.height : 256
            width: sourceRect.width ? sourceRect.width : 256
            z: videoBase.z + 1
        }
        VideoOutput {
            id: videoLayer2

            anchors.centerIn: videoBase
            height: sourceRect.height ? sourceRect.height : 256
            width: sourceRect.width ? sourceRect.width : 256
            z: videoLayer.z + 1
        }
        ColorChanger {
            anchors.fill: videoLayer
            from: "black"
            to: "transparent"
            z: videoLayer.z

            source: ShaderEffectSource {
                hideSource: true
                sourceItem: videoLayer
            }
        }
        ColorChanger {
            anchors.fill: videoLayer2
            from: "black"
            to: "transparent"
            z: videoLayer2.z

            source: ShaderEffectSource {
                hideSource: true
                sourceItem: videoLayer
            }
        }
        VideoOutput {
            id: videoPoor

            anchors.centerIn: videoBase
            height: sourceRect.height ? sourceRect.height : 256
            visible: false
            width: sourceRect.width ? sourceRect.width : 256
            z: videoLayer2.z + 1
        }
    }
}
