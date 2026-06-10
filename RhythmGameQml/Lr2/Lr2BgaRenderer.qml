import QtMultimedia
import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

Item {
    id: root

    property var dsts: []
    property int skinTime: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property var srcData: null
    property var chart
    property real scaleOverride: 1.0
    property bool mediaActive: true
    property bool poorVisible: false

    readonly property alias baseSink: videoBase.videoSink
    readonly property alias layerSink: videoLayer.videoSink
    readonly property alias layer2Sink: videoLayer2.videoSink
    readonly property alias poorSink: videoPoor.videoSink
    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState && timelineState.staticState.valid
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.dsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }
    readonly property var currentState: staticTimelineState
        || (timelineState.hasState ? timelineState.state : null)
    readonly property var anchor: Lr2SkinUtils.centerAnchor(currentState ? currentState.center : 0)
    readonly property var bgaContainer: chart && chart.bga ? chart.bga : null
    readonly property bool hasRenderableState: currentState !== null
        && currentState !== undefined
        && currentState.a > 0
        && Math.abs(currentState.w || 0) > 0
        && Math.abs(currentState.h || 0) > 0
    readonly property bool drawBaseLayer: !sourceFlag("noBase")
    readonly property bool drawAdditionalLayers: !sourceFlag("noLayer")
    readonly property bool drawPoorLayer: !sourceFlag("noPoor")
    readonly property bool showNormalLayers: !poorVisible || !drawPoorLayer
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

    function sourceFlag(name: string) : bool {
        if (!srcData) {
            return false;
        }
        let value = srcData[name];
        return value !== undefined && value !== null && Number(value) !== 0;
    }

    function clearOutput() : void {
        if ("clearOutput" in videoBase) {
            videoBase.clearOutput();
            videoLayer.clearOutput();
            videoLayer2.clearOutput();
            videoPoor.clearOutput();
        }
    }

    function bgaLayer(container: var, index: var) : var {
        if (!container || !container.layers || container.layers.length <= index) {
            return null;
        }
        return container.layers[index];
    }

    function detachAttachedBga() : var {
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

    function attachBga() : var {
        if (!mediaActive || !bgaContainer || !hasRenderableState) {
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

        if (base && drawBaseLayer) {
            base.videoSink = baseSink;
        } else if (base && base.videoSink === baseSink) {
            base.videoSink = null;
        }
        if (layer && drawAdditionalLayers) {
            layer.videoSink = layerSink;
        } else if (layer && layer.videoSink === layerSink) {
            layer.videoSink = null;
        }
        if (layer2 && drawAdditionalLayers) {
            layer2.videoSink = layer2Sink;
        } else if (layer2 && layer2.videoSink === layer2Sink) {
            layer2.videoSink = null;
        }
        if (poor && drawPoorLayer) {
            poor.videoSink = poorSink;
        } else if (poor && poor.videoSink === poorSink) {
            poor.videoSink = null;
        }

        attachedBga = bgaContainer;
    }

    onBgaContainerChanged: Qt.callLater(attachBga)
    onMediaActiveChanged: Qt.callLater(attachBga)
    onChartChanged: Qt.callLater(attachBga)
    onHasRenderableStateChanged: Qt.callLater(attachBga)
    onDrawBaseLayerChanged: Qt.callLater(attachBga)
    onDrawAdditionalLayersChanged: Qt.callLater(attachBga)
    onDrawPoorLayerChanged: Qt.callLater(attachBga)
    Component.onCompleted: Qt.callLater(attachBga)
    Component.onDestruction: detachAttachedBga()

    Connections {
        target: root.chart
        ignoreUnknownSignals: true

        function onBgaLoaded() : void {
            Qt.callLater(root.attachBga);
        }

        function onBgaChanged() : void {
            Qt.callLater(root.attachBga);
        }

        function onStatusChanged() : void {
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
        visible: root.mediaActive && root.hasRenderableState
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
            visible: root.drawBaseLayer && root.showNormalLayers
        }

        VideoOutput {
            id: videoLayer

            anchors.fill: parent
            fillMode: VideoOutput.Stretch
            visible: root.drawAdditionalLayers && root.showNormalLayers
            z: 1
        }

        VideoOutput {
            id: videoLayer2

            anchors.fill: parent
            fillMode: VideoOutput.Stretch
            visible: root.drawAdditionalLayers && root.showNormalLayers
            z: 2
        }

        ColorChanger {
            anchors.fill: videoLayer
            from: "black"
            to: "transparent"
            visible: videoLayer.visible
            z: 1

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
            z: 2

            source: ShaderEffectSource {
                hideSource: true
                sourceItem: videoLayer2
            }
        }

        VideoOutput {
            id: videoPoor

            anchors.fill: parent
            fillMode: VideoOutput.Stretch
            visible: root.drawPoorLayer && root.poorVisible
            z: 3
        }
    }
}

