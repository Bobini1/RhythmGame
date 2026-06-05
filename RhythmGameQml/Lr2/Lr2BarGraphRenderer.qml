import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property int sourceSkinTime: skinTime
    property var skinClock: null
    property int skinClockMode: 0
    property int sourceSkinClockMode: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property var chart
    property string chartAssetSource: ""
    property real scaleOverride: 1.0
    property real value: 0
    property bool animateValue: false
    property int valueAnimationDuration: 100
    property bool colorKeyEnabled: false
    property color transColor: "black"
    property bool sourceHasFrameAnimation: Lr2SkinUtils.sourceCyclesContinuously(srcData)

    Lr2TimelineFrame {
        id: drawState
        dsts: root.dsts
        skinTime: root.skinTime
        skinClock: root.skinClock
        skinClockMode: root.skinClockMode
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
        timers: root.timers
        timerFire: root.timerFire
        colorKeyEnabled: root.colorKeyEnabled
        supportsInvertedBlend: true
    }

    readonly property bool hasCurrentState: drawState.hasState
    readonly property real stateX: drawState.x
    readonly property real stateY: drawState.y
    readonly property real stateW: drawState.w
    readonly property real stateH: drawState.h
    readonly property real stateA: drawState.a
    readonly property real stateR: drawState.r
    readonly property real stateG: drawState.g
    readonly property real stateB: drawState.b
    readonly property int stateBlend: drawState.blend
    readonly property int stateFilter: drawState.filter
    readonly property real clampedValue: Math.max(0, Math.min(1, value))
    property real displayedValue: clampedValue
    readonly property int blendMode: drawState.blendMode
    readonly property color tintColor: drawState.tintColor
    readonly property bool hasFrameAnimation: sourceHasFrameAnimation
    property Lr2AnimationFrameState animationFrameState: Lr2AnimationFrameState {
        enabled: root.hasFrameAnimation
        skinClock: root.skinClock
        clockMode: root.hasFrameAnimation ? root.sourceSkinClockMode : 0
        sourceData: root.srcData
        skinTime: root.sourceSkinTime
        timers: root.timers
        timerFire: root.sourceTimerFire
        textureWidth: Math.max(0, graphAtlas.implicitWidth)
        textureHeight: Math.max(0, graphAtlas.implicitHeight)
    }
    readonly property string resolvedSource: Lr2SkinUtils.resolvedSource(
        srcData,
        chart,
        chartAssetSource)

    onClampedValueChanged: displayedValue = clampedValue

    Behavior on displayedValue {
        enabled: root.animateValue
        NumberAnimation {
            duration: root.valueAnimationDuration
            easing.type: Easing.Linear
        }
    }

    Item {
        id: graph
        readonly property real signedW: root.hasCurrentState ? root.stateW * root.scaleOverride : 0
        readonly property real signedH: root.hasCurrentState ? root.stateH * root.scaleOverride : 0
        readonly property real fullW: Math.abs(signedW)
        readonly property real fullH: Math.abs(signedH)
        readonly property bool horizontal: root.srcData ? root.srcData.direction === 0 : true
        readonly property real drawnW: horizontal ? fullW * root.displayedValue : fullW
        readonly property real drawnH: horizontal ? fullH : fullH * root.displayedValue
        readonly property vector4d clippedSourceRect: {
            let rect = root.animationFrameState.sourceRect;
            if (!rect) {
                return Qt.vector4d(0, 0, 1, 1);
            }

            let x = rect.x;
            let y = rect.y;
            let w = rect.z;
            let h = rect.w;
            let amount = root.displayedValue;
            if (horizontal) {
                let clippedW = w * amount;
                if (signedW < 0) {
                    x += w - clippedW;
                }
                w = clippedW;
            } else {
                let clippedH = h * amount;
                if (signedH < 0) {
                    y += h - clippedH;
                }
                h = clippedH;
            }
            return Qt.vector4d(x, y, w, h);
        }

        x: root.hasCurrentState
            ? root.stateX * root.scaleOverride
                + (horizontal && signedW < 0 ? signedW * root.displayedValue : 0)
            : 0
        y: root.hasCurrentState
            ? root.stateY * root.scaleOverride
                + (!horizontal && signedH < 0 ? signedH * root.displayedValue : 0)
            : 0
        width: drawnW
        height: drawnH
        visible: root.hasCurrentState && root.stateA > 0 && width > 0 && height > 0 && root.resolvedSource !== ""
        opacity: root.hasCurrentState ? root.stateA / 255.0 : 0

        Image {
            id: graphAtlas
            source: root.resolvedSource
            asynchronous: root.srcData
                && (root.srcData.specialType === 1
                    || root.srcData.specialType === 3
                    || root.srcData.specialType === 4)
            cache: true
            smooth: false
            mipmap: false
            visible: false
        }

        ShaderEffect {
            anchors.fill: parent
            visible: graphAtlas.status === Image.Ready
            blending: true
            supportsAtlasTextures: true
            property var source: graphAtlas
            property color tint: root.tintColor
            property color transColor: root.transColor
            property real blendMode: root.blendMode
            property real colorKeyEnabled: root.blendMode === 0 ? 1.0 : 0.0
            property real tolerance: 0.001
            property real nearestMode: root.hasCurrentState && root.stateFilter === 0 ? 1.0 : 0.0
            property vector2d sourceSize: Qt.vector2d(
                Math.max(1, graphAtlas.implicitWidth),
                Math.max(1, graphAtlas.implicitHeight))
            property vector4d sourceRect: graph.clippedSourceRect
            fragmentShader: "qrc:/Lr2/Lr2SpriteAtlas.frag.qsb"
        }
    }
}

