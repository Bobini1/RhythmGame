import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property var screenRoot
    property real scaleOverride: 1.0
    property bool mediaActive: true
    property color transColor: "black"

    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    readonly property var currentState: staticTimelineState
        || Lr2Timeline.getCurrentState(dsts, skinTime, timelineTimers, timelineActiveOptions)
    readonly property int side: srcData && srcData.side > 0 ? srcData.side : 1
    readonly property var player: screenRoot ? screenRoot.gameplayPlayer(side) : null
    readonly property var score: player ? player.score : null
    readonly property real gaugeValue: screenRoot && score ? screenRoot.gameplayGaugeValue(score) : 0
    readonly property int hpSegments: Math.max(0, Math.min(50, Math.floor(gaugeValue / 2)))
    readonly property bool survivalGauge: {
        let vars = player && player.profile && player.profile.vars
            ? player.profile.vars.generalVars
            : null;
        let gauge = String(vars ? vars.gaugeType : "").toUpperCase();
        return gauge === "HARD" || gauge === "EXHARD" || gauge === "EXDAN"
            || gauge === "EXHARDDAN" || gauge === "FC" || gauge === "PERFECT"
            || gauge === "MAX";
    }

    function segmentFrame(segment) {
        let oneBased = segment + 1;
        if (survivalGauge) {
            return oneBased <= hpSegments ? 0 : 2;
        }
        if (oneBased < 40) {
            return oneBased <= hpSegments ? 1 : 3;
        }
        return oneBased <= hpSegments ? 0 : 2;
    }

    function segmentState(segment) {
        if (!currentState || !srcData) {
            return null;
        }
        return {
            x: (currentState.x || 0) + segment * (srcData.op1 || 0),
            y: (currentState.y || 0) + segment * (srcData.op2 || 0),
            w: currentState.w || 0,
            h: currentState.h || 0,
            a: currentState.a === undefined ? 255 : currentState.a,
            r: currentState.r === undefined ? 255 : currentState.r,
            g: currentState.g === undefined ? 255 : currentState.g,
            b: currentState.b === undefined ? 255 : currentState.b,
            blend: currentState.blend === undefined ? 1 : currentState.blend,
            filter: currentState.filter || 0,
            angle: currentState.angle || 0,
            center: currentState.center || 0,
            op4: currentState.op4 || 0
        };
    }

    Repeater {
        model: root.currentState && root.srcData ? 50 : 0

        Lr2FastSprite {
            srcData: root.srcData
            stateData: root.segmentState(index)
            skinTime: root.skinTime
            timers: root.timers
            scaleOverride: root.scaleOverride
            frameOverride: root.segmentFrame(index)
        }
    }
}
