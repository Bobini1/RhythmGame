import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property var screenRoot
    property real scaleOverride: 1.0
    property bool mediaActive: true
    property color transColor: "black"
    property int trembleRoll: 0

    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(dsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(dsts[0], dsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(dsts) ? activeOptions : []
    readonly property var currentState: staticTimelineState
        || Lr2Timeline.getCurrentStateWithOptionalTimerFire(
            dsts, skinTime, timelineTimers, timerFire, timelineActiveOptions)
    readonly property int side: srcData && srcData.side > 0 ? srcData.side : 1
    readonly property var player: screenRoot ? screenRoot.gameplayPlayer(side) : null
    readonly property var score: player ? player.score : null
    readonly property real gaugeValue: screenRoot && score ? screenRoot.gameplayGaugeValue(score) : 0
    readonly property int hpSegments: Math.max(0, Math.min(50, Math.floor(gaugeValue / 2)))
    readonly property string activeGaugeName: screenRoot ? screenRoot.activeGaugeNameForSide(side) : ""
    readonly property bool survivalGauge: screenRoot
        ? screenRoot.gaugeNameIsSurvival(activeGaugeName)
        : false
    readonly property int exGaugeFrameOffset: srcData
        && srcData.grooveGaugeEx
        && screenRoot
        && screenRoot.gaugeNameUsesExGaugeSprites(activeGaugeName)
        ? 4
        : 0

    function segmentFrame(segment, hp, trembleRoll, survival) {
        let oneBased = segment + 1;
        let tremble = hp - trembleRoll;
        if (!survival) {
            if (oneBased < 40) {
                return ((oneBased < tremble || hp <= oneBased || oneBased === 1) && oneBased <= hp)
                    ? 1
                    : 3;
            }
            if ((tremble <= oneBased && oneBased < hp && oneBased !== 1) || hp < oneBased) {
                return 2;
            }
            return 0;
        }

        if (oneBased < tremble || hp <= oneBased || oneBased === 1) {
            return hp < oneBased ? 2 : 0;
        }
        return 2;
    }

    FrameAnimation {
        running: root.mediaActive && root.currentState && root.srcData
        onTriggered: {
            // DXLib GetRand(2) returns an integer in the inclusive 0..2 range.
            root.trembleRoll = Math.floor(Math.random() * 3);
        }
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
            frameOverride: root.segmentFrame(
                index, root.hpSegments, root.trembleRoll, root.survivalGauge)
                + root.exGaugeFrameOffset
        }
    }
}
