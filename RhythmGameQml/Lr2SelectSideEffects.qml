pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml 1.0

Item {
    id: root

    required property var host
    required property var selectContext
    required property var skinModel

    property bool active: false
    property int selectRevision: 0
    property bool ready: false
    property bool scratchSoundReady: false
    property var deferredChart: null
    property string activePreviewSource: ""
    property int pendingPreviewRevision: -1
    property int pendingPreviewRequest: 0
    property string pendingPreviewSource: ""
    property alias scratchSoundPlayer: scratchSound

    readonly property var renderChart: root.host.effectiveScreenKey === "select"
        ? root.deferredChart
        : (root.host.chart || root.host.resultChartData())

    function updateDisplayedChart() {
        if (root.host.effectiveScreenKey === "select") {
            root.deferredChart = root.selectContext.selectedChartWrapper();
        } else {
            root.deferredChart = root.host.chart;
        }
    }

    function update() {
        if (!root.ready) {
            return;
        }

        root.updateDisplayedChart();

        if (!root.active) {
            root.stopAudio();
            return;
        }

        let nextPreviewSource = root.selectContext.selectedPreviewSource() || "";
        root.pendingPreviewRevision = root.selectRevision;
        if (nextPreviewSource === root.pendingPreviewSource) {
            if (nextPreviewSource !== ""
                    && root.activePreviewSource !== nextPreviewSource
                    && !previewDelayTimer.running) {
                previewDelayTimer.restart();
            }
            return;
        }

        previewDelayTimer.stop();
        root.pendingPreviewRequest += 1;
        root.pendingPreviewSource = nextPreviewSource;
        root.activePreviewSource = "";
        playMusic.stop();
        if (nextPreviewSource !== "") {
            previewDelayTimer.restart();
        }
    }

    function stopAudio() {
        previewDelayTimer.stop();
        selectBgmDelayTimer.stop();
        root.activePreviewSource = "";
        root.pendingPreviewRevision = -1;
        root.pendingPreviewSource = "";
        root.pendingPreviewRequest += 1;
        playMusic.stop();
        selectBgm.stop();
        scratchSound.stop();
    }

    function playScratch() {
        if (!root.host.enabled
            || root.host.effectiveScreenKey !== "select"
            || !root.scratchSoundReady) {
            return;
        }
        root.host.playOneShot(scratchSound);
    }

    AudioPlayer {
        id: playMusic
        looping: true
        fadeInMillis: 1000
        source: root.active ? root.activePreviewSource : ""
        onSourceChanged: stop()
    }

    AudioPlayer {
        id: selectBgm
        looping: true
        fadeInMillis: 1000
        source: root.host.mainGeneralVars() ? root.host.mainGeneralVars().bgmPath + "select" : ""
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
        source: root.host.mainGeneralVars() ? root.host.mainGeneralVars().soundsetPath + "scratch" : ""
    }

    Timer {
        id: previewDelayTimer
        interval: 1000
        repeat: false
        onTriggered: {
            if (!root.active
                || root.pendingPreviewRevision !== root.selectRevision) {
                return;
            }

            let source = root.pendingPreviewSource;
            let request = root.pendingPreviewRequest;
            root.activePreviewSource = source || "";
            if (source) {
                Qt.callLater(() => {
                    if (root.active
                        && root.activePreviewSource === source
                        && root.pendingPreviewRevision === root.selectRevision
                        && root.pendingPreviewRequest === request) {
                        playMusic.play();
                    }
                });
            }
        }
    }
}
