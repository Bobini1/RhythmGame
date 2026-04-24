pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml 1.0
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var screenRoot
    property var skinModel
    property real skinScale: 1
    property int renderSkinTime: 0
    property var runtimeActiveOptions: []
    property var timers: ({ 0: 0 })
    property color transColor: "black"
    property int framePulse: 0
    readonly property bool playfieldActive: root.enabled
        && !!screenRoot
        && !!skinModel
        && (!screenRoot.isGameplayScreen || screenRoot.isGameplayScreen())
        && !!root.playerForLr2Index(0)

    function listValue(list, index) {
        return list && index >= 0 && index < list.length ? list[index] : null;
    }

    function sourceAt(list, index) {
        let source = listValue(list, index);
        if (source && (source.source || source.specialType)) {
            return source;
        }
        source = listValue(list, 0);
        return source && (source.source || source.specialType) ? source : null;
    }

    function noteDsts(index) {
        return listValue(skinModel ? skinModel.noteDsts : [], index) || [];
    }

    function lineDsts(index) {
        return listValue(skinModel ? skinModel.lineDsts : [], index) || [];
    }

    function dstStateFor(dsts) {
        if (Lr2Timeline.canUseStaticState(dsts)) {
            return Lr2Timeline.copyDstAsState(dsts[0], dsts[0]);
        }
        return Lr2Timeline.getCurrentState(
            dsts,
            renderSkinTime,
            Lr2Timeline.dstsUseDynamicTimer(dsts) ? timers : null,
            Lr2Timeline.dstsUseActiveOptions(dsts) ? runtimeActiveOptions : []);
    }

    function noteDstState(index) {
        return dstStateFor(noteDsts(index));
    }

    function lineDstState(index) {
        return dstStateFor(lineDsts(index));
    }

    readonly property var laneIndexes: {
        let result = [];
        let destinations = skinModel ? skinModel.noteDsts : [];
        for (let i = 0; i < destinations.length; ++i) {
            if (destinations[i] && destinations[i].length > 0) {
                result.push(i);
            }
        }
        return result;
    }

    readonly property var lineIndexes: {
        let result = [];
        let destinations = skinModel ? skinModel.lineDsts : [];
        for (let i = 0; i < destinations.length; ++i) {
            if (destinations[i] && destinations[i].length > 0) {
                result.push(i);
            }
        }
        return result;
    }

    function sideForLr2Index(index) {
        return index >= 10 ? 2 : 1;
    }

    function engineColumnForLr2Index(index) {
        if (screenRoot && screenRoot.gameplayEngineColumnForLr2Lane) {
            return screenRoot.gameplayEngineColumnForLr2Lane(index);
        }
        if (index >= 10) {
            return index === 10 ? 15 : 8 + index - 11;
        }
        return index === 0 ? 7 : index - 1;
    }

    function playerForLr2Index(index) {
        let side = sideForLr2Index(index);
        if (screenRoot && screenRoot.gameplayLanePlayer) {
            return screenRoot.gameplayLanePlayer(side);
        }
        return screenRoot && screenRoot.gameplayPlayer ? screenRoot.gameplayPlayer(side) : null;
    }

    function lineSourceFor(index) {
        if (!skinModel) {
            return null;
        }
        return sourceAt(skinModel.lineSources, index);
    }

    function bpmFor(player) {
        let chartData = screenRoot ? screenRoot.gameplayChartData() : null;
        let vars = player && player.profile && player.profile.vars
            ? player.profile.vars.generalVars
            : null;
        if (!chartData || !vars) {
            return 120;
        }
        switch (vars.hiSpeedFix) {
        case HiSpeedFix.Off:
            return 120;
        case HiSpeedFix.Main:
            return chartData.mainBpm || 120;
        case HiSpeedFix.Start:
            return chartData.initialBpm || chartData.mainBpm || 120;
        case HiSpeedFix.Min:
            return chartData.minBpm || chartData.mainBpm || 120;
        case HiSpeedFix.Max:
            return chartData.maxBpm || chartData.mainBpm || 120;
        case HiSpeedFix.Avg:
            return chartData.avgBpm || chartData.mainBpm || 120;
        default:
            return chartData.mainBpm || 120;
        }
    }

    function heightMultiplier(player, dst) {
        let vars = player && player.profile && player.profile.vars
            ? player.profile.vars.generalVars
            : null;
        if (!vars) {
            return 1;
        }
        let visibleHeight = Math.max(1, Math.abs(dst && dst.y ? dst.y : 480));
        let bpm = Math.max(1, bpmFor(player));
        let baseSpeed = ((1 / (vars.noteScreenTimeMillis || 1)) || 0)
            * 60000 * visibleHeight / bpm;
        let laneCoverMod = (vars.laneCoverOn ? 1 : 0) * (vars.laneCoverRatio || 0);
        let liftMod = (vars.liftOn ? 1 : 0) * (vars.liftRatio || 0);
        return Math.max(0.0001, baseSpeed * Math.max(0, Math.min(1 - laneCoverMod - liftMod, 1)));
    }

    function notePosition(display) {
        return display && display.note && display.note.time ? display.note.time.position || 0 : 0;
    }

    function linePosition(display) {
        return display && display.time ? display.time.position || 0 : 0;
    }

    function movingLayerY(playerPosition, dst, multiplier) {
        return (dst ? dst.y || 0 : 0) + playerPosition * multiplier;
    }

    function lineLocalY(display, multiplier) {
        return -linePosition(display) * multiplier;
    }

    function nextNotePosition(display, notes) {
        if (!display || !notes) {
            return Infinity;
        }
        let next = notes[(display.index || 0) + 1];
        return next && next.time ? next.time.position || Infinity : Infinity;
    }

    function spriteState(dst, y, height) {
        if (!dst) {
            return null;
        }
        return {
            x: dst.x || 0,
            y: y,
            w: dst.w || 0,
            h: height > 0 ? height : dst.h || 0,
            a: dst.a === undefined ? 255 : dst.a,
            r: dst.r === undefined ? 255 : dst.r,
            g: dst.g === undefined ? 255 : dst.g,
            b: dst.b === undefined ? 255 : dst.b,
            blend: dst.blend === undefined ? 1 : dst.blend,
            filter: dst.filter || 0,
            angle: dst.angle || 0,
            center: dst.center || 0,
            op4: dst.op4 || 0
        };
    }

    FrameAnimation {
        running: root.playfieldActive
        onTriggered: root.framePulse += 1
    }

    Repeater {
        model: root.playfieldActive
                ? root.lineIndexes
                : []

        delegate: Item {
            id: lineArea

            required property int modelData

            property int lineIndex: modelData
            property var dstState: root.lineDstState(lineIndex)
            property var player: lineIndex === 1
                ? root.playerForLr2Index(10)
                : root.playerForLr2Index(0)
            property var barLinesState: player && player.state
                ? player.state.barLinesState
                : null
            property var lineSource: root.lineSourceFor(lineIndex)
            property real travelHeight: Math.max(1, Math.abs(dstState ? dstState.y || 0 : 0))
            property real multiplier: root.heightMultiplier(player, dstState)
            property real playerPosition: {
                root.framePulse;
                return player ? player.position || 0 : 0;
            }
            property real layerSkinY: root.movingLayerY(playerPosition, dstState, multiplier)

            width: parent.width
            height: parent.height
            z: -1000

            function syncBarLineWindow() {
                if (!lineArea.barLinesState || !lineArea.dstState || lineArea.multiplier <= 0) {
                    return;
                }
                lineArea.barLinesState.topPosition = lineArea.playerPosition + lineArea.travelHeight / lineArea.multiplier;
                lineArea.barLinesState.bottomPosition = lineArea.playerPosition;
            }

            onPlayerPositionChanged: syncBarLineWindow()
            onBarLinesStateChanged: syncBarLineWindow()
            onDstStateChanged: syncBarLineWindow()
            onTravelHeightChanged: syncBarLineWindow()
            onMultiplierChanged: syncBarLineWindow()
            Component.onCompleted: syncBarLineWindow()

            Item {
                id: lineLayer

                width: parent.width
                height: parent.height
                y: lineArea.layerSkinY * root.skinScale

                Repeater {
                    model: lineArea.barLinesState || []

                    delegate: Lr2FastSprite {
                        id: lineItem

                        required property var display

                        srcData: lineArea.lineSource
                        stateData: root.spriteState(
                            lineArea.dstState,
                            root.lineLocalY(display, lineArea.multiplier),
                            lineArea.dstState ? lineArea.dstState.h : 0)
                        skinTime: root.renderSkinTime
                        timers: root.timers
                        scaleOverride: root.skinScale
                    }
                }
            }
        }
    }

    Repeater {
        model: root.playfieldActive
                ? root.laneIndexes
                : []

        delegate: Item {
            id: lane

            required property int modelData

            property int lr2Index: modelData
            property var dstState: root.noteDstState(lr2Index)
            property int side: root.sideForLr2Index(lr2Index)
            property var player: root.playerForLr2Index(lr2Index)
            property int engineColumn: root.engineColumnForLr2Index(lr2Index)
            property var columnState: player
                && player.state
                && player.state.columnStates
                && engineColumn >= 0
                && engineColumn < player.state.columnStates.length
                    ? player.state.columnStates[engineColumn]
                    : null
            property var notes: player
                && player.notes
                && player.notes.notes
                && engineColumn >= 0
                && engineColumn < player.notes.notes.length
                    ? player.notes.notes[engineColumn]
                    : []
            property var normalSource: root.sourceAt(skinModel ? skinModel.noteSources : [], lr2Index)
                || root.sourceAt(skinModel ? skinModel.autoNoteSources : [], lr2Index)
            property var mineSource: root.sourceAt(skinModel ? skinModel.mineSources : [], lr2Index)
                || root.sourceAt(skinModel ? skinModel.autoMineSources : [], lr2Index)
            property var lnStartSource: root.sourceAt(skinModel ? skinModel.lnStartSources : [], lr2Index)
                || normalSource
                || root.sourceAt(skinModel ? skinModel.autoLnStartSources : [], lr2Index)
            property var lnEndSource: root.sourceAt(skinModel ? skinModel.lnEndSources : [], lr2Index)
                || normalSource
                || root.sourceAt(skinModel ? skinModel.autoLnEndSources : [], lr2Index)
            property var lnBodySource: root.sourceAt(skinModel ? skinModel.lnBodySources : [], lr2Index)
                || root.sourceAt(skinModel ? skinModel.autoLnBodySources : [], lr2Index)
            property real travelHeight: Math.max(1, Math.abs(dstState ? dstState.y || 0 : 0))
            property real multiplier: root.heightMultiplier(player, dstState)
            property real playerPosition: {
                root.framePulse;
                return player ? player.position || 0 : 0;
            }
            property real layerSkinY: root.movingLayerY(playerPosition, dstState, multiplier)

            width: parent.width
            height: parent.height

            function syncColumnWindow() {
                if (!lane.columnState || !lane.dstState || lane.multiplier <= 0) {
                    return;
                }
                lane.columnState.topPosition = lane.playerPosition + lane.travelHeight / lane.multiplier;
                lane.columnState.bottomPosition = lane.playerPosition;
            }

            function sourceForDisplay(display) {
                if (!display || !display.note) {
                    return null;
                }
                switch (display.note.type) {
                case note.Type.Normal:
                    return lane.normalSource;
                case note.Type.Landmine:
                    return lane.mineSource;
                case note.Type.LongNoteBegin:
                    return lane.lnStartSource;
                case note.Type.LongNoteEnd:
                    return lane.lnEndSource;
                default:
                    return null;
                }
            }

            onPlayerPositionChanged: syncColumnWindow()
            onColumnStateChanged: syncColumnWindow()
            onDstStateChanged: syncColumnWindow()
            onTravelHeightChanged: syncColumnWindow()
            onMultiplierChanged: syncColumnWindow()
            onHeightChanged: syncColumnWindow()
            Component.onCompleted: syncColumnWindow()

            Item {
                id: noteLayer

                width: parent.width
                height: parent.height
                y: lane.layerSkinY * root.skinScale

                Repeater {
                    model: lane.columnState || []

                    delegate: Item {
                        id: noteItem

                        required property var display
                        required property int index

                        readonly property var hitData: display ? display.hitData : null
                        readonly property bool visibleNote: display
                            && display.note
                            && (display.note.type === note.Type.LongNoteBegin
                                || display.note.type === note.Type.LongNoteEnd
                                || !hitData)
                        readonly property bool heldLongNote: display
                            && display.note
                            && display.note.type === note.Type.LongNoteBegin
                            && hitData
                            && !display.otherEndHitData
                        readonly property bool staticLongNote: display
                            && display.note
                            && display.note.type === note.Type.LongNoteBegin
                            && (heldLongNote || display.belowBottom)
                            && root.nextNotePosition(display, lane.notes) > lane.playerPosition
                        readonly property real localY: (staticLongNote
                            ? -lane.playerPosition
                            : -root.notePosition(display)) * lane.multiplier
                        readonly property var noteSource: lane.sourceForDisplay(display)
                        readonly property var noteState: root.spriteState(
                            lane.dstState,
                            localY,
                            lane.dstState ? lane.dstState.h : 0)

                        visible: visibleNote && !!noteSource && !!noteState
                        width: parent.width
                        height: parent.height
                        z: -index

                        Loader {
                            id: lnBodyLoader

                            active: noteItem.visible
                                && display.note.type === note.Type.LongNoteBegin
                                && !!lane.lnBodySource
                                && root.nextNotePosition(display, lane.notes) < Infinity

                            sourceComponent: Component {
                                Lr2FastSprite {
                                    srcData: lane.lnBodySource
                                    skinTime: root.renderSkinTime
                                    timers: root.timers
                                    scaleOverride: root.skinScale
                                    tileVertically: true
                                    stateData: {
                                        let nextY = -root.nextNotePosition(display, lane.notes) * lane.multiplier;
                                        let noteHeight = lane.dstState ? Math.abs(lane.dstState.h || 0) : 0;
                                        let top = Math.min(noteItem.localY, nextY) + noteHeight;
                                        let height = Math.max(1, Math.abs(noteItem.localY - nextY) - noteHeight);
                                        return root.spriteState(lane.dstState, top, height);
                                    }
                                }
                            }
                        }

                        Lr2FastSprite {
                            srcData: noteItem.noteSource
                            stateData: noteItem.noteState
                            skinTime: root.renderSkinTime
                            timers: root.timers
                            scaleOverride: root.skinScale
                        }
                    }
                }
            }
        }
    }
}
