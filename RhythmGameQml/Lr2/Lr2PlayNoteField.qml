pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import "Lr2SkinUtils.js" as Lr2SkinUtils

Item {
    id: root

    property var screenRoot
    property var skinTiming: null
    property var skinModel
    property Lr2GameplayFrameState gameplayFrameState: screenRoot
        ? screenRoot.gameplayFrameStateRef
        : null
    property real skinScale: 1
    property int renderSkinTime: 0
    property var runtimeActiveOptions: []
    property var timers: ({ 0: 0 })
    property color transColor: "black"
    property Lr2TimelineState timelineResolver: Lr2TimelineState {}
    readonly property var skinRuntime: screenRoot ? screenRoot.skinRuntimeRef : null
    readonly property var skinTimerLookup: skinTiming || (screenRoot ? screenRoot.skinTimingRef : null)
    readonly property int runtimeDescriptorRevision: skinRuntime ? skinRuntime.descriptorRevision : 0
    readonly property var runtimeNoteDstTimerFires: skinRuntime ? skinRuntime.noteDstTimerFires : []
    readonly property var runtimeLineDstTimerFires: skinRuntime ? skinRuntime.lineDstTimerFires : []
    readonly property string laneCoverImageSource: skinModel
        ? String(skinModel.laneCoverSource || "")
        : ""
    readonly property bool playfieldActive: root.enabled
        && !!screenRoot
        && !!skinModel
        && (screenRoot.gameplayScreenActive === undefined || screenRoot.gameplayScreenActive)
        && !!root.playerForLr2Index(0)
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
            let staticState = timelineResolver.staticStateFor(dsts);
            return staticState && staticState.valid ? staticState : null;
        }
        let timerFire = 0;
        if (timelineResolver.usesDynamicTimerFor(dsts)) {
            timerFire = root.timerFireFor(timelineResolver.firstTimerFor(dsts));
        }
        let state = timelineResolver.stateFromTimerFire(
            dsts,
            renderSkinTime,
            timerFire,
            timelineResolver.usesActiveOptionsFor(dsts) ? runtimeActiveOptions : []);
        return state && state.valid ? state : null;
    }

    function timerFireFor(timer: var) : var {
        let idx = Number(timer || 0);
        if (idx === 0) {
            return 0;
        }
        if (skinTimerLookup && skinTimerLookup.skinTimerFireTime) {
            return skinTimerLookup.skinTimerFireTime(idx, false);
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
            runtimeDescriptorRevision;
            runtimeActiveOptions;
            let state = null;
            if (skinRuntime.noteDstStateUsesSkinTime(index)) {
                let timerFire = index >= 0 && index < runtimeNoteDstTimerFires.length
                    ? runtimeNoteDstTimerFires[index]
                    : -1;
                state = skinRuntime.noteDstStateForTimerFire(index, renderSkinTime, timerFire);
            } else {
                state = skinRuntime.noteDstState(index, 0);
            }
            return state && state.valid ? state : null;
        }
        return dstStateFor(noteDsts(index));
    }

    function lineDstState(index: var) : var {
        if (skinRuntime) {
            runtimeDescriptorRevision;
            runtimeActiveOptions;
            let state = null;
            if (skinRuntime.lineDstStateUsesSkinTime(index)) {
                let timerFire = index >= 0 && index < runtimeLineDstTimerFires.length
                    ? runtimeLineDstTimerFires[index]
                    : -1;
                state = skinRuntime.lineDstStateForTimerFire(index, renderSkinTime, timerFire);
            } else {
                state = skinRuntime.lineDstState(index, 0);
            }
            return state && state.valid ? state : null;
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

    function beatorajaLiftRatio(side: var) : var {
        let vars = generalVarsForSide(side);
        return screenRoot && screenRoot.lr2SkinUsesBeatorajaSemantics && vars && vars.liftOn
            ? Math.max(0, Math.min(1, vars.liftRatio || 0))
            : 0;
    }

    function laneBottom(side: var, dst: var) : var {
        let bottom = dst && dst.y !== undefined ? dst.y : 480;
        return bottom - dstTravelHeight(dst) * beatorajaLiftRatio(side);
    }

    function laneVisibleTravelHeight(side: var, dst: var, includeLaneCover: var) : var {
        let vars = generalVarsForSide(side);
        let visibleHeight = dstTravelHeight(dst);
        visibleHeight *= Math.max(0, Math.min(1 - beatorajaLiftRatio(side), 1));
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
        return Math.max(0, laneBottom(side, dst) - laneVisibleTravelHeight(side, dst, true));
    }

    function hiddenClipBottom(side: var, dst: var) : var {
        let vars = generalVarsForSide(side);
        let fullHeight = skinModel && skinModel.skinHeight ? skinModel.skinHeight : 480;
        if (!vars || !vars.hiddenOn) {
            return fullHeight;
        }
        let bottom = laneBottom(side, dst);
        let visibleHeight = laneVisibleTravelHeight(side, dst, false);
        return Math.max(0, bottom - visibleHeight * hiddenRatio(side));
    }

    function hidSudClipTop(side: var, dst: var) : var {
        return root.laneCoverClipTop(side, dst);
    }

    function hidSudClipBottom(side: var, dst: var) : var {
        return root.hiddenClipBottom(side, dst);
    }

    function hidSudClipActive(side: var) : var {
        let vars = generalVarsForSide(side);
        return !!vars && (!!vars.laneCoverOn || !!vars.hiddenOn);
    }

    function laneBounds(side: var) : var {
        let start = side === 2 ? 10 : 0;
        let end = side === 2 ? 20 : 10;
        let minX = Infinity;
        let minY = Infinity;
        let maxX = -Infinity;
        let maxY = -Infinity;
        for (let i = start; i < end; ++i) {
            let state = noteDstState(i);
            if (!state) {
                continue;
            }
            let width = Math.abs(state.w || 0);
            if (width <= 0) {
                continue;
            }
            let x1 = state.x || 0;
            let x2 = x1 + (state.w || 0);
            let bottom = laneBottom(side, state);
            let top = bottom - laneVisibleTravelHeight(side, state, false);
            minX = Math.min(minX, x1, x2);
            maxX = Math.max(maxX, x1, x2);
            minY = Math.min(minY, top, bottom);
            maxY = Math.max(maxY, top, bottom);
        }
        if (minX === Infinity || maxX <= minX || maxY <= minY) {
            return null;
        }
        return {
            x: minX,
            y: minY,
            w: maxX - minX,
            h: maxY - minY,
            bottom: maxY
        };
    }

    function laneCoverVisualHeight(side: var, bounds: var) : var {
        if (!bounds) {
            return 0;
        }
        let vars = generalVarsForSide(side);
        if (!vars || !vars.laneCoverOn) {
            return 0;
        }
        let coverRatio = Math.max(0, Math.min(1, vars.laneCoverRatio || 0));
        let visibleRatio = Math.max(0, 1 - coverRatio);
        return Math.max(0, Math.min(bounds.h, bounds.h * (1 - visibleRatio)));
    }

    function sideSpeedHeight(side: var, fallbackDst: var) : var {
        let start = side === 2 ? 10 : 0;
        let end = side === 2 ? 20 : 10;
        for (let i = start; i < end; ++i) {
            let state = noteDstState(i);
            if (state) {
                return laneVisibleTravelHeight(side, state, false);
            }
        }
        return laneVisibleTravelHeight(side, fallbackDst, false);
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
        if (screenRoot && screenRoot.lr2SkinUsesBeatorajaSemantics) {
            return Math.max(0.0001, baseSpeed);
        }
        let laneCoverMod = (vars.laneCoverOn ? 1 : 0) * (vars.laneCoverRatio || 0);
        return Math.max(0.0001, baseSpeed * Math.max(0, Math.min(1 - laneCoverMod, 1)));
    }

    function visualPosition(player: var, side: int) : real {
        if (gameplayFrameState) {
            return side === 2 ? gameplayFrameState.position2 : gameplayFrameState.position1;
        }
        return player ? player.position || 0 : 0;
    }

    function noteVisiblePositionSpan(player: var) : var {
        let span = 0;
        for (let index of laneIndexes) {
            if (playerForLr2Index(index) !== player) {
                continue;
            }
            let dst = noteDstState(index);
            if (!dst) {
                continue;
            }
            let side = sideForLr2Index(index);
            let multiplier = heightMultiplier(player, sideSpeedHeight(side, dst));
            if (multiplier > 0) {
                span = Math.max(span, laneVisibleTravelHeight(side, dst, false) / multiplier);
            }
        }
        return span;
    }

    function barLineVisiblePositionSpan(player: var) : var {
        let span = 0;
        for (let index of lineIndexes) {
            let side = index === 1 ? 2 : 1;
            let linePlayer = playerForLr2Index(side === 2 ? 10 : 0);
            if (linePlayer !== player) {
                continue;
            }
            let dst = lineDstState(index);
            if (!dst) {
                continue;
            }
            let multiplier = heightMultiplier(player, sideSpeedHeight(side, dst));
            if (multiplier > 0) {
                span = Math.max(span, laneVisibleTravelHeight(side, dst, false) / multiplier);
            }
        }
        return span;
    }

    function syncPlayerViewport(player: var) : void {
        if (!player || !player.state || !player.state.setVisiblePositionSpans) {
            return;
        }
        let noteSpan = noteVisiblePositionSpan(player);
        let barLineSpan = barLineVisiblePositionSpan(player);
        if (noteSpan > 0) {
            player.state.setVisiblePositionSpans(
                noteSpan,
                barLineSpan > 0 ? barLineSpan : noteSpan);
        }
    }

    function linePosition(display: var) : var {
        return display && display.time ? display.time.position || 0 : 0;
    }

    function lineLocalY(display: var, multiplier: var) : var {
        return -linePosition(display) * multiplier;
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
            property real travelHeight: root.laneVisibleTravelHeight(side, dstState, false)
            property real multiplier: root.heightMultiplier(
                player,
                root.sideSpeedHeight(side, dstState))
            readonly property real laneBottomPosition: root.laneBottom(side, dstState)
            property bool clipActive: root.hidSudClipActive(side)
            property real clipTopSkin: root.hidSudClipTop(side, dstState)
            property real clipBottomSkin: root.hidSudClipBottom(side, dstState)

            width: parent.width
            height: parent.height
            z: -1

            function syncBarLineWindow() : var {
                root.syncPlayerViewport(lineArea.player);
            }

            onPlayerChanged: syncBarLineWindow()
            onBarLinesStateChanged: syncBarLineWindow()
            onDstStateChanged: syncBarLineWindow()
            onTravelHeightChanged: syncBarLineWindow()
            onMultiplierChanged: syncBarLineWindow()
            Component.onCompleted: syncBarLineWindow()

            Item {
                id: lineClip

                width: parent.width
                height: lineArea.clipActive
                    ? Math.max(1, (lineArea.clipBottomSkin - lineArea.clipTopSkin) * root.skinScale)
                    : parent.height
                y: lineArea.clipActive ? lineArea.clipTopSkin * root.skinScale : 0
                clip: lineArea.clipActive

                Item {
                    id: lineLayer

                    width: parent.width
                    height: lineArea.height
                    y: lineArea.laneBottomPosition * root.skinScale - lineClip.y
                    transform: Lr2GameplayPositionTransform {
                        frameState: root.gameplayFrameState
                        player: lineArea.player
                        targetItem: lineLayer
                        side: lineArea.side
                        multiplier: lineArea.multiplier * root.skinScale
                    }

                    Repeater {
                        model: lineArea.barLinesState || []

                        delegate: Lr2FastSprite {
                            id: lineItem

                            required property var display

                            readonly property int computedSourceTimerFire: root.sourceTimerFireFor(
                                lineArea.lineSource)

                            srcData: lineArea.lineSource
                            asynchronousLoading: !!root.screenRoot
                                && root.screenRoot.customizeMode === true
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
            property real travelHeight: root.laneVisibleTravelHeight(side, dstState, false)
            property real multiplier: root.heightMultiplier(
                player,
                root.sideSpeedHeight(side, dstState))
            readonly property real laneBottomPosition: root.laneBottom(side, dstState)
            property bool clipActive: root.hidSudClipActive(side)
            property real clipTopSkin: root.hidSudClipTop(side, dstState)
            property real clipBottomSkin: root.hidSudClipBottom(side, dstState)

            width: parent.width
            height: parent.height

            function syncColumnWindow() : var {
                root.syncPlayerViewport(lane.player);
            }

            function sourceForType(noteType: int) : var {
                switch (noteType) {
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

            onPlayerChanged: syncColumnWindow()
            onColumnStateChanged: syncColumnWindow()
            onDstStateChanged: syncColumnWindow()
            onTravelHeightChanged: syncColumnWindow()
            onMultiplierChanged: syncColumnWindow()
            onHeightChanged: syncColumnWindow()
            Component.onCompleted: syncColumnWindow()

            Item {
                id: noteClip

                width: parent.width
                height: lane.clipActive
                    ? Math.max(1, (lane.clipBottomSkin - lane.clipTopSkin) * root.skinScale)
                    : parent.height
                y: lane.clipActive ? lane.clipTopSkin * root.skinScale : 0
                clip: lane.clipActive

                Item {
                    id: noteLayer

                    width: parent.width
                    height: lane.height
                    y: lane.laneBottomPosition * root.skinScale - noteClip.y
                    transform: Lr2GameplayPositionTransform {
                        frameState: root.gameplayFrameState
                        player: lane.player
                        targetItem: noteLayer
                        side: lane.side
                        multiplier: lane.multiplier * root.skinScale
                    }

                    Repeater {
                        model: lane.columnState || []

                        delegate: Item {
                            id: noteItem

                            required property int noteType
                            required property real notePosition
                            required property real longNoteEndPosition
                            required property bool visibleNote
                            required property bool heldLongNote
                            required property bool staticLongNoteCandidate
                            required property int index

                            readonly property real staticPlayerPosition: staticLongNoteCandidate
                                ? root.visualPosition(lane.player, lane.side)
                                : 0
                            readonly property bool staticLongNote: staticLongNoteCandidate
                                && longNoteEndPosition > staticPlayerPosition
                            readonly property real localY: (staticLongNote
                                ? -staticPlayerPosition
                                : -notePosition) * lane.multiplier
                            readonly property var noteSource: lane.sourceForType(noteType)
                            readonly property int noteSourceTimerFire: root.sourceTimerFireFor(noteSource)

                            visible: visibleNote && !!noteSource && !!lane.dstState
                            width: parent.width
                            height: parent.height
                            z: -index

                            Loader {
                                id: lnBodyLoader

                                active: noteItem.visible
                                    && noteItem.noteType === note.Type.LongNoteBegin
                                    && !!(noteItem.heldLongNote
                                        ? (lane.lnBodyActiveSource || lane.lnBodyInactiveSource)
                                        : (lane.lnBodyInactiveSource || lane.lnBodyActiveSource))
                                    && noteItem.longNoteEndPosition < Infinity
                                asynchronous: !!root.screenRoot
                                    && root.screenRoot.customizeMode === true

                                sourceComponent: Component {
                                    Lr2FastSprite {
                                        readonly property var bodySource: noteItem.heldLongNote
                                            ? (lane.lnBodyActiveSource || lane.lnBodyInactiveSource)
                                            : (lane.lnBodyInactiveSource || lane.lnBodyActiveSource)
                                        readonly property int bodyTimer: 70 + lane.lr2Index
                                        readonly property int bodyTimerFire: root.sourceTimerFireFor(
                                            bodySource,
                                            noteItem.heldLongNote ? bodyTimer : 0)

                                        srcData: bodySource
                                        asynchronousLoading: !!root.screenRoot
                                            && root.screenRoot.customizeMode === true
                                        skinTime: root.sourceSkinTimeFor(
                                            bodySource,
                                            bodyTimerFire,
                                            noteItem.heldLongNote ? bodyTimer : 0)
                                        timers: null
                                        sourceTimerFire: bodyTimerFire
                                        scaleOverride: root.skinScale
                                        tileVertically: true
                                        readonly property real nextY:
                                            -noteItem.longNoteEndPosition * lane.multiplier
                                        readonly property real noteHeight: lane.dstState
                                            ? Math.abs(lane.dstState.h || 0)
                                            : 0
                                        stateData: lane.dstState
                                        stateYOverride: Math.min(noteItem.localY, nextY) + noteHeight
                                        stateHeightOverride: Math.max(
                                            1,
                                            Math.abs(noteItem.localY - nextY) - noteHeight)
                                    }
                                }
                            }

                            Lr2FastSprite {
                                srcData: noteItem.noteSource
                                asynchronousLoading: !!root.screenRoot
                                    && root.screenRoot.customizeMode === true
                                stateData: lane.dstState
                                stateYOverride: noteItem.localY
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

    Repeater {
        model: root.playfieldActive && root.laneCoverImageSource !== ""
            ? [1, 2]
            : []

        delegate: Item {
            id: laneCoverVisual

            required property int modelData

            readonly property int side: modelData
            readonly property var bounds: root.laneBounds(side)
            readonly property real coverHeight: root.laneCoverVisualHeight(side, bounds)

            x: bounds ? bounds.x * root.skinScale : 0
            y: bounds ? bounds.y * root.skinScale : 0
            width: bounds ? bounds.w * root.skinScale : 0
            height: coverHeight * root.skinScale
            visible: bounds
                && coverHeight > 0
                && root.laneCoverImageSource !== ""
            clip: true
            z: 10

            Image {
                source: Lr2SkinUtils.fileUrlForPath(root.laneCoverImageSource)
                width: laneCoverVisual.width
                height: laneCoverVisual.bounds
                    ? laneCoverVisual.bounds.h * root.skinScale
                    : 0
                fillMode: Image.Stretch
                cache: true
                asynchronous: false
                smooth: false
            }
        }
    }

}
