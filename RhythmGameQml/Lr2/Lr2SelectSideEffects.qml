pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

Item {
    id: root

    required property var host
    required property var skinModel
    required property string previewSource

    property bool active: false
    property bool ready: false
    property bool scratchSoundReady: false
    property string playingPreviewSource: ""
    property int previewRequestToken: 0
    property string queuedPreviewSource: ""
    property alias scratchSoundPlayer: scratchSound

    readonly property string requestedPreviewSource: root.ready && root.active
        ? (root.previewSource || "")
        : ""

    onActiveChanged: {
        if (root.ready && !root.active) {
            root.stopAudio();
        }
    }
    onReadyChanged: {
        if (root.ready && root.active) {
            root.syncPreviewRequest();
        }
    }
    onRequestedPreviewSourceChanged: {
        if (root.active) {
            root.syncPreviewRequest();
        }
    }

    function syncPreviewRequest() : var {
        if (!root.ready) {
            return;
        }

        if (!root.active) {
            root.stopAudio();
            return;
        }

        let nextPreviewSource = root.requestedPreviewSource;
        if (nextPreviewSource === root.queuedPreviewSource) {
            if (nextPreviewSource !== ""
                    && root.playingPreviewSource !== nextPreviewSource
                    && !previewDelayTimer.running) {
                previewDelayTimer.restart();
            }
            return;
        }

        previewDelayTimer.stop();
        root.previewRequestToken += 1;
        root.queuedPreviewSource = nextPreviewSource;
        root.playingPreviewSource = "";
        playMusic.stop();
        if (nextPreviewSource !== "") {
            previewDelayTimer.restart();
        }
    }

    function stopAudio() : void {
        previewDelayTimer.stop();
        selectBgmDelayTimer.stop();
        root.playingPreviewSource = "";
        root.queuedPreviewSource = "";
        root.previewRequestToken += 1;
        playMusic.stop();
        selectBgm.stop();
        scratchSound.stop();
    }

    function playScratch() : void {
        root.playScratchBurst(1);
    }

    function playScratchBurst(repeats: var) : var {
        if (!root.host.enabled
            || root.host.effectiveScreenKey !== "select"
            || !root.scratchSoundReady) {
            return;
        }
        let count = Math.max(0, Math.round(repeats || 0));
        for (let i = 0; i < count; ++i) {
            scratchSound.playOverlapping();
        }
    }

    AudioPlayer {
        id: playMusic
        looping: true
        fadeInMillis: 1000
        source: root.active ? root.playingPreviewSource : ""
        onSourceChanged: stop()
    }

    AudioPlayer {
        id: selectBgm
        looping: true
        fadeInMillis: 1000
        source: root.host.mainGeneralVarsRef ? root.host.mainGeneralVarsRef.bgmPath + "select" : ""
        property bool canPlay: root.active
            && (!playMusic.playing || playMusic.source === "")
        onCanPlayChanged: {
            if (!canPlay) {
                stop();
            }
        }
    }

    Timer {
        id: selectBgmDelayTimer
        interval: 500
        running: selectBgm.canPlay
        repeat: false
        onTriggered: selectBgm.play()
    }

    AudioPlayer {
        id: scratchSound
        source: root.host.mainGeneralVarsRef ? root.host.mainGeneralVarsRef.soundsetPath + "scratch" : ""
    }

    Timer {
        id: previewDelayTimer
        interval: 1000
        repeat: false
        onTriggered: {
            if (!root.active) {
                return;
            }

            let source = root.queuedPreviewSource;
            let request = root.previewRequestToken;
            root.playingPreviewSource = source || "";
            if (source) {
                Qt.callLater(() => {
                    if (root.active
                        && root.playingPreviewSource === source
                        && root.previewRequestToken === request) {
                        playMusic.play();
                    }
                });
            }
        }
    }
}
