import QtMultimedia
import QtQuick
import RhythmGameQml 1.0

import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property var chart
    property real scaleOverride: 1.0
    property bool mediaActive: true
    property bool poorVisible: false

    readonly property alias baseSink: videoBase.videoSink
    readonly property alias layerSink: videoLayer.videoSink
    readonly property alias layer2Sink: videoLayer2.videoSink
    readonly property alias poorSink: videoPoor.videoSink
    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    readonly property var currentState: staticTimelineState
        || Lr2Timeline.getCurrentState(dsts, skinTime, timelineTimers, timelineActiveOptions)
    readonly property var anchor: Lr2Timeline.centerAnchor(currentState ? currentState.center : 0)
    readonly property var bgaContainer: chart && chart.bga ? chart.bga : null
    readonly property real sourceW: Math.max(videoBase.sourceRect.width,
                                             videoLayer.sourceRect.width,
                                             videoLayer2.sourceRect.width,
                                             videoPoor.sourceRect.width,
                                             256)
    readonly property real sourceH: Math.max(videoBase.sourceRect.height,
                                             videoLayer.sourceRect.height,
                                             videoLayer2.sourceRect.height,
                                             videoPoor.sourceRect.height,
                                             256)
    property var attachedBga: null

    function clearOutput() {
        if ("clearOutput" in videoBase) {
            videoBase.clearOutput();
            videoLayer.clearOutput();
            videoLayer2.clearOutput();
            videoPoor.clearOutput();
        }
    }

    function bgaLayer(container, index) {
        if (!container || !container.layers || container.layers.length <= index) {
            return null;
        }
        return container.layers[index];
    }

    function detachAttachedBga() {
        let container = attachedBga;
        if (!container) {
            clearOutput();
            return;
        }

        let base = bgaLayer(container, 0);
        let layer = bgaLayer(container, 1);
        let layer2 = bgaLayer(container, 2);
        let poor = bgaLayer(container, 3);

        if (base && base.videoSink === baseSink) base.videoSink = null;
        if (layer && layer.videoSink === layerSink) layer.videoSink = null;
        if (layer2 && layer2.videoSink === layer2Sink) layer2.videoSink = null;
        if (poor && poor.videoSink === poorSink) poor.videoSink = null;

        attachedBga = null;
        clearOutput();
    }

    function attachBga() {
        if (!mediaActive || !bgaContainer) {
            detachAttachedBga();
            return;
        }

        if (chart && chart.status === ChartRunner.Finished) {
            detachAttachedBga();
            return;
        }

        if (attachedBga && attachedBga !== bgaContainer) {
            detachAttachedBga();
        }

        let base = bgaLayer(bgaContainer, 0);
        let layer = bgaLayer(bgaContainer, 1);
        let layer2 = bgaLayer(bgaContainer, 2);
        let poor = bgaLayer(bgaContainer, 3);

        if (base) base.videoSink = baseSink;
        if (layer) layer.videoSink = layerSink;
        if (layer2) layer2.videoSink = layer2Sink;
        if (poor) poor.videoSink = poorSink;

        attachedBga = bgaContainer;
    }

    onBgaContainerChanged: Qt.callLater(attachBga)
    onMediaActiveChanged: Qt.callLater(attachBga)
    onChartChanged: Qt.callLater(attachBga)
    Component.onCompleted: Qt.callLater(attachBga)
    Component.onDestruction: detachAttachedBga()

    Connections {
        target: root.chart
        ignoreUnknownSignals: true

        function onBgaLoaded() {
            Qt.callLater(root.attachBga);
        }

        function onBgaChanged() {
            Qt.callLater(root.attachBga);
        }

        function onStatusChanged() {
            Qt.callLater(root.attachBga);
        }
    }

    Item {
        id: bga

        readonly property real dstW: root.currentState ? root.currentState.w * root.scaleOverride : 0
        readonly property real dstH: root.currentState ? root.currentState.h * root.scaleOverride : 0
        readonly property real sizeRatioX: Math.min(1.0, root.sourceW / 256.0)
        readonly property real sizeRatioY: Math.min(1.0, root.sourceH / 256.0)

        x: root.currentState ? (root.currentState.x * root.scaleOverride) + (dstW - width) * 0.5 : 0
        y: root.currentState ? root.currentState.y * root.scaleOverride : 0
        width: dstW * sizeRatioX
        height: dstH * sizeRatioY
        visible: root.mediaActive
        opacity: root.currentState ? root.currentState.a / 255.0 : 0

        transform: Rotation {
            origin.x: bga.width * root.anchor.x
            origin.y: bga.height * root.anchor.y
            angle: root.currentState ? root.currentState.angle : 0
        }

        VideoOutput {
            id: videoBase

            anchors.fill: parent
            fillMode: VideoOutput.Stretch
            visible: !root.poorVisible
        }

        VideoOutput {
            id: videoLayer

            anchors.fill: parent
            fillMode: VideoOutput.Stretch
            visible: !root.poorVisible
            z: videoBase.z + 1
        }

        VideoOutput {
            id: videoLayer2

            anchors.fill: parent
            fillMode: VideoOutput.Stretch
            visible: !root.poorVisible
            z: videoLayer.z + 1
        }

        ColorChanger {
            anchors.fill: videoLayer
            from: "black"
            to: "transparent"
            visible: videoLayer.visible
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
            visible: videoLayer2.visible
            z: videoLayer2.z

            source: ShaderEffectSource {
                hideSource: true
                sourceItem: videoLayer2
            }
        }

        VideoOutput {
            id: videoPoor

            anchors.fill: parent
            fillMode: VideoOutput.Stretch
            visible: root.poorVisible
            z: videoLayer2.z + 1
        }
    }
}
