pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

Item {
    id: root

    property var screenRoot
    property var skinModel
    property real skinScale: 1
    property int renderSkinTime: 0
    property var runtimeActiveOptions: []
    property var timers: ({ 0: 0 })
    property color transColor: "black"
    property Lr2TimelineState timelineResolver: Lr2TimelineState {}
    readonly property var skinRuntime: screenRoot ? screenRoot.skinRuntimeRef : null
    readonly property int runtimeRevision: skinRuntime ? skinRuntime.revision : 0
    readonly property int runtimeTimerRevision: skinRuntime ? skinRuntime.timerRevision : 0
    readonly property int runtimeActiveOptionsRevision: skinRuntime ? skinRuntime.activeOptionsRevision : 0
    readonly property bool playfieldActive: root.enabled
        && !!screenRoot
        && !!skinModel
        && (screenRoot.gameplayScreenActive === undefined || screenRoot.gameplayScreenActive)
        && !!root.playerForLr2Index(0)
    readonly property var gameplayFrameState: screenRoot ? screenRoot.gameplayFrameStateRef : null
    readonly property var fallbackPlayer1: gameplayFrameState ? null : root.playerForLr2Index(0)
    readonly property var fallbackPlayer2: gameplayFrameState ? null : root.playerForLr2Index(10)
    readonly property real sampledPosition1: gameplayFrameState
        ? gameplayFrameState.position1 || 0
        : (fallbackPlayer1 ? fallbackPlayer1.position || 0 : 0)
    readonly property real sampledPosition2: gameplayFrameState
        ? gameplayFrameState.position2 || 0
        : (fallbackPlayer2 ? fallbackPlayer2.position || 0 : 0)

    function listValue(list: var, index: var) : var {
        return list && index >= 0 && index < list.length ? list[index] : null;
    }

    function sourceAt(list: var, index: var) : var {
        let source = listValue(list, index);
        if (source && (source.source || source.specialType)) {
            return source;
        }
        source = listValue(list, 0);
        return source && (source.source || source.specialType) ? source : null;
    }

    function noteDsts(index: var) : var {
        return listValue(skinModel ? skinModel.noteDsts : [], index) || [];
    }

    function lineDsts(index: var) : var {
        return listValue(skinModel ? skinModel.lineDsts : [], index) || [];
    }

    function dstStateFor(dsts: var) : var {
        if (timelineResolver.canUseStaticStateFor(dsts)) {
            return timelineResolver.staticStateFor(dsts);
        }
        let timerFire = 0;
        if (timelineResolver.usesDynamicTimerFor(dsts)) {
            timerFire = root.timerFireFor(timelineResolver.firstTimerFor(dsts));
        }
        return timelineResolver.stateFromTimerFire(
            dsts,
            renderSkinTime,
            timerFire,
            timelineResolver.usesActiveOptionsFor(dsts) ? runtimeActiveOptions : []);
    }

    function timerFireFor(timer: var) : var {
        let idx = Number(timer || 0);
        if (idx === 0) {
            return 0;
        }
        if (screenRoot && screenRoot.skinTimerFireTime) {
            return screenRoot.skinTimerFireTime(idx, false);
        }
        return timelineResolver.timerFireFor(timers, idx);
    }

    function sourceCyclesContinuously(src: var) : var {
        return src
            && (src.cycle || 0) > 0
            && Math.max(1, src.div_x || 1) * Math.max(1, src.div_y || 1) > 1;
    }

    function sourceTimerFor(src: var, fallbackTimer: var) : var {
        if (!sourceCyclesContinuously(src)) {
            return 0;
        }
        let timer = Number(src.timer || 0);
        if (timer !== 0) {
            return timer;
        }
        return fallbackTimer !== undefined && fallbackTimer > 0 ? fallbackTimer : 0;
    }

    function sourceTimerFireFor(src: var, fallbackTimer: var) : var {
        if (!sourceCyclesContinuously(src)) {
            return -2147483648;
        }
        return root.timerFireFor(sourceTimerFor(src, fallbackTimer));
    }

    function sourceSkinTimeFor(src: var, timerFire: var, fallbackTimer: var) : var {
        if (!sourceCyclesContinuously(src)) {
            return 0;
        }
        if (sourceTimerFor(src, fallbackTimer) !== 0 && timerFire < 0) {
            return 0;
        }
        return renderSkinTime;
    }

    function noteDstState(index: var) : var {
        if (skinRuntime) {
            runtimeRevision;
            runtimeActiveOptionsRevision;
            if (skinRuntime.noteDstStateUsesSkinTime(index)) {
                runtimeTimerRevision;
                return skinRuntime.noteDstState(index, renderSkinTime);
            }
            return skinRuntime.noteDstState(index, 0);
        }
        return dstStateFor(noteDsts(index));
    }

    function lineDstState(index: var) : var {
        if (skinRuntime) {
            runtimeRevision;
            runtimeActiveOptionsRevision;
            if (skinRuntime.lineDstStateUsesSkinTime(index)) {
                runtimeTimerRevision;
                return skinRuntime.lineDstState(index, renderSkinTime);
            }
            return skinRuntime.lineDstState(index, 0);
        }
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

    function sideForLr2Index(index: var) : var {
        return index >= 10 ? 2 : 1;
    }

    function engineColumnForLr2Index(index: var) : var {
        if (screenRoot && screenRoot.gameplayEngineColumnForLr2Lane) {
            return screenRoot.gameplayEngineColumnForLr2Lane(index);
        }
        if (index >= 10) {
            return index === 10 ? 15 : 8 + index - 11;
        }
        return index === 0 ? 7 : index - 1;
    }

    function playerForLr2Index(index: var) : var {
        let side = sideForLr2Index(index);
        if (screenRoot && screenRoot.gameplayLanePlayer) {
            return screenRoot.gameplayLanePlayer(side);
        }
        return screenRoot && screenRoot.gameplayPlayer ? screenRoot.gameplayPlayer(side) : null;
    }

    function lineSourceFor(index: var) : var {
        if (!skinModel) {
            return null;
        }
        return sourceAt(skinModel.lineSources, index);
    }

    function bpmFor(player: var) : var {
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

    function dstTravelHeight(dst: var) : var {
        return Math.max(1, Math.abs(dst && dst.y ? dst.y : 480));
    }

    function lr2HidSudMode(side: var) : var {
        if (screenRoot && screenRoot.lr2HidSudIndex) {
            return screenRoot.lr2HidSudIndex(side);
        }
        return 0;
    }

    function generalVarsForSide(side: var) : var {
        return screenRoot && screenRoot.generalVarsForSide
            ? screenRoot.generalVarsForSide(side)
            : null;
    }

    function laneCoverRatio(side: var) : var {
        let vars = generalVarsForSide(side);
        return vars ? Math.max(0, Math.min(1, vars.laneCoverRatio || 0)) : 0;
    }

    function hiddenRatio(side: var) : var {
        let vars = generalVarsForSide(side);
        return vars ? Math.max(0, Math.min(1, vars.hiddenRatio || 0)) : 0;
    }

    function laneBottom(dst: var) : var {
        return dst && dst.y !== undefined ? dst.y : 480;
    }

    function laneVisibleTravelHeight(side: var, dst: var, includeLaneCover: var) : var {
        let vars = generalVarsForSide(side);
        let visibleHeight = dstTravelHeight(dst);
        if (vars && vars.liftOn) {
            visibleHeight *= Math.max(0, Math.min(1 - (vars.liftRatio || 0), 1));
        }
        if (includeLaneCover && vars && vars.laneCoverOn) {
            visibleHeight *= Math.max(0, Math.min(1 - (vars.laneCoverRatio || 0), 1));
        }
        return Math.max(1, visibleHeight);
    }

    function laneCoverClipTop(side: var, dst: var) : var {
        let vars = generalVarsForSide(side);
        if (!vars || !vars.laneCoverOn) {
            return 0;
        }
        return Math.max(0, laneBottom(dst) - laneVisibleTravelHeight(side, dst, true));
    }

    function hiddenClipBottom(side: var, dst: var) : var {
        let vars = generalVarsForSide(side);
        let fullHeight = skinModel && skinModel.skinHeight ? skinModel.skinHeight : 480;
        if (!vars || !vars.hiddenOn) {
            return fullHeight;
        }
        let bottom = laneBottom(dst);
        let visibleHeight = laneVisibleTravelHeight(side, dst, false);
        return Math.max(0, bottom - visibleHeight * hiddenRatio(side));
    }

    function hidSudClipTop(side: var, dst: var) : var {
        let mode = lr2HidSudMode(side);
        if (mode === 2 || mode === 3) {
            return laneCoverClipTop(side, dst);
        }
        return 0;
    }

    function hidSudClipBottom(side: var, dst: var) : var {
        let mode = lr2HidSudMode(side);
        let fullHeight = skinModel && skinModel.skinHeight ? skinModel.skinHeight : 480;
        if (mode === 1 || mode === 3) {
            return hiddenClipBottom(side, dst);
        }
        return fullHeight;
    }

    function sideSpeedHeight(side: var, fallbackDst: var) : var {
        let start = side === 2 ? 10 : 0;
        let end = side === 2 ? 20 : 10;
        for (let i = start; i < end; ++i) {
            let state = noteDstState(i);
            if (state) {
                return dstTravelHeight(state);
            }
        }
        return dstTravelHeight(fallbackDst);
    }

    function heightMultiplier(player: var, visibleHeight: var) : var {
        let vars = player && player.profile && player.profile.vars
            ? player.profile.vars.generalVars
            : null;
        if (!vars) {
            return 1;
        }
        let bpm = Math.max(1, bpmFor(player));
        let baseSpeed = ((1 / (vars.noteScreenTimeMillis || 1)) || 0)
            * 60000 * visibleHeight / bpm;
        let laneCoverMod = (vars.laneCoverOn ? 1 : 0) * (vars.laneCoverRatio || 0);
        let liftMod = (vars.liftOn ? 1 : 0) * (vars.liftRatio || 0);
        return Math.max(0.0001, baseSpeed * Math.max(0, Math.min(1 - laneCoverMod - liftMod, 1)));
    }

    function notePosition(display: var) : var {
        return display && display.note && display.note.time ? display.note.time.position || 0 : 0;
    }

    function linePosition(display: var) : var {
        return display && display.time ? display.time.position || 0 : 0;
    }

    function lineLocalY(display: var, multiplier: var) : var {
        return -linePosition(display) * multiplier;
    }

    function nextNotePosition(display: var, notes: var) : var {
        if (!display || !notes) {
            return Infinity;
        }
        let next = notes[(display.index || 0) + 1];
        return next && next.time ? next.time.position || Infinity : Infinity;
    }

    function spriteState(dst: var, y: var, height: var) : var {
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

    Repeater {
        model: root.playfieldActive
                ? root.lineIndexes
                : []

        delegate: Item {
            id: lineArea

            required property int modelData

            property int lineIndex: modelData
            property var dstState: root.lineDstState(lineIndex)
            property int side: lineIndex === 1 ? 2 : 1
            property var player: lineIndex === 1
                ? root.playerForLr2Index(10)
                : root.playerForLr2Index(0)
            property var barLinesState: player && player.state
                ? player.state.barLinesState
                : null
            property var lineSource: root.lineSourceFor(lineIndex)
            property real travelHeight: root.dstTravelHeight(dstState)
            property real multiplier: root.heightMultiplier(
                player,
                root.sideSpeedHeight(side, dstState))
            property real playerPosition: side === 2 ? root.sampledPosition2 : root.sampledPosition1
            property real layerSkinY: (dstState ? dstState.y || 0 : 0) + playerPosition * multiplier
            property int hidSudMode: root.lr2HidSudMode(side)
            property real clipTopSkin: root.hidSudClipTop(side, dstState)
            property real clipBottomSkin: root.hidSudClipBottom(side, dstState)

            width: parent.width
            height: parent.height
            z: -1

            function syncBarLineWindow() : var {
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
                id: lineClip

                width: parent.width
                height: lineArea.hidSudMode > 0
                    ? Math.max(1, (lineArea.clipBottomSkin - lineArea.clipTopSkin) * root.skinScale)
                    : parent.height
                y: lineArea.hidSudMode > 0 ? lineArea.clipTopSkin * root.skinScale : 0
                clip: lineArea.hidSudMode > 0

                Item {
                    id: lineLayer

                    width: parent.width
                    height: lineArea.height
                    y: lineArea.layerSkinY * root.skinScale - lineClip.y

                    Repeater {
                        model: lineArea.barLinesState || []

                        delegate: Lr2FastSprite {
                            id: lineItem

                            required property var display

                            readonly property int computedSourceTimerFire: root.sourceTimerFireFor(
                                lineArea.lineSource)

                            srcData: lineArea.lineSource
                            stateData: root.spriteState(
                                lineArea.dstState,
                                root.lineLocalY(display, lineArea.multiplier),
                                lineArea.dstState ? lineArea.dstState.h : 0)
                            skinTime: root.sourceSkinTimeFor(
                                lineArea.lineSource,
                                lineItem.computedSourceTimerFire)
                            timers: null
                            sourceTimerFire: lineItem.computedSourceTimerFire
                            scaleOverride: root.skinScale
                        }
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
            property var lnBodyInactiveSource: root.sourceAt(skinModel ? skinModel.lnBodySources : [], lr2Index)
                || root.sourceAt(skinModel ? skinModel.autoLnBodySources : [], lr2Index)
            property var lnBodyActiveSource: root.sourceAt(skinModel ? skinModel.lnBodyActiveSources : [], lr2Index)
                || root.sourceAt(skinModel ? skinModel.autoLnBodyActiveSources : [], lr2Index)
            property real travelHeight: root.dstTravelHeight(dstState)
            property real multiplier: root.heightMultiplier(
                player,
                root.sideSpeedHeight(side, dstState))
            property real playerPosition: side === 2 ? root.sampledPosition2 : root.sampledPosition1
            property real layerSkinY: (dstState ? dstState.y || 0 : 0) + playerPosition * multiplier
            property int hidSudMode: root.lr2HidSudMode(side)
            property real clipTopSkin: root.hidSudClipTop(side, dstState)
            property real clipBottomSkin: root.hidSudClipBottom(side, dstState)

            width: parent.width
            height: parent.height

            function syncColumnWindow() : var {
                if (!lane.columnState || !lane.dstState || lane.multiplier <= 0) {
                    return;
                }
                lane.columnState.topPosition = lane.playerPosition + lane.travelHeight / lane.multiplier;
                lane.columnState.bottomPosition = lane.playerPosition;
            }

            function sourceForDisplay(display: var) : var {
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
                id: noteClip

                width: parent.width
                height: lane.hidSudMode > 0
                    ? Math.max(1, (lane.clipBottomSkin - lane.clipTopSkin) * root.skinScale)
                    : parent.height
                y: lane.hidSudMode > 0 ? lane.clipTopSkin * root.skinScale : 0
                clip: lane.hidSudMode > 0

                Item {
                    id: noteLayer

                    width: parent.width
                    height: lane.height
                    y: lane.layerSkinY * root.skinScale - noteClip.y

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
                            readonly property var lnBodySource: heldLongNote
                                ? (lane.lnBodyActiveSource || lane.lnBodyInactiveSource)
                                : (lane.lnBodyInactiveSource || lane.lnBodyActiveSource)
                            readonly property int lnTimer: 70 + lane.lr2Index
                            readonly property int noteSourceTimerFire: root.sourceTimerFireFor(noteSource)
                            readonly property int lnBodySourceTimerFire: root.sourceTimerFireFor(
                                lnBodySource,
                                heldLongNote ? lnTimer : 0)
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
                                    && !!noteItem.lnBodySource
                                    && root.nextNotePosition(display, lane.notes) < Infinity

                                sourceComponent: Component {
                                    Lr2FastSprite {
                                        srcData: noteItem.lnBodySource
                                        skinTime: root.sourceSkinTimeFor(
                                            noteItem.lnBodySource,
                                            noteItem.lnBodySourceTimerFire,
                                            noteItem.heldLongNote ? noteItem.lnTimer : 0)
                                        timers: null
                                        sourceTimerFire: noteItem.lnBodySourceTimerFire
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
                                skinTime: root.sourceSkinTimeFor(
                                    noteItem.noteSource,
                                    noteItem.noteSourceTimerFire)
                                timers: null
                                sourceTimerFire: noteItem.noteSourceTimerFire
                                scaleOverride: root.skinScale
                            }
                        }
                    }
                }
            }
        }
    }
}

