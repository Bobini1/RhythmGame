import QtQuick
import RhythmGameQml

Item {
    id: root

    property string previewSource: ""

    Timer {
        id: delay
        readonly property string source: root.enabled ? root.previewSource : ""
        property string delayedSource: ""
        interval: 300
        running: delay.source !== ""
        onSourceChanged: {
            delay.delayedSource = "";
            if (delay.source !== "") {
                delay.restart();
            }
        }
        onTriggered: delay.delayedSource = delay.source
    }

    AudioPlayer {
        id: player
        source: root.enabled ? delay.delayedSource : ""
        playing: root.enabled && player.loaded && player.source !== ""
        looping: true
    }
}
