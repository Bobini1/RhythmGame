import QtQuick
import RhythmGameQml

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property int sourceSkinTime: skinTime
    property var skinClock: null
    property int sourceSkinClockMode: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property var chart
    property string stageFileSource: ""
    property string backBmpSource: ""
    property string bannerSource: ""
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barBaseStateResolver
    property var barPositionMap
    property var barCells: []
    property var barTextCells: []
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property real selectedFastBarDrawX: 0
    property real selectedFastBarDrawY: 0
    property int barCenter: 0
    property var barLampVariants: []
    property color transColor: "black"
    property bool colorKeyEnabled: false
    property var loggedBodyDebugKeys: ({})
    property int loggedBodyDebugCount: 0
    property var loggedBodyPlacementKeys: ({})
    property int loggedBodyPlacementCount: 0
    readonly property var barBaseStates: barBaseStateResolver ? barBaseStateResolver.baseStates : []
    readonly property var positionlessBarBaseStates: barBaseStateResolver
        ? barBaseStateResolver.positionlessBaseStates
        : []
    readonly property var barStateInterpolationNeeded: barBaseStateResolver
        ? barBaseStateResolver.interpolationNeededByRow
        : []
    readonly property bool selectedBodySource: !!srcData
        && srcData.kind === 0
        && srcData.row === selectedRow
    readonly property bool applyFastBarScroll: fastBarScrollActive
        && !selectedBodySource
        && (!srcData || srcData.kind !== 2)
    readonly property bool useDirectSourceSkinClock: !!skinClock && sourceSkinClockMode !== 0
    x: applyFastBarScroll ? fastBarScrollX * scaleOverride : 0
    y: applyFastBarScroll ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property int barCellCount: barTextCells && barTextCells.length > 0
        ? barTextCells.length
        : (barCells ? barCells.length : 0)
    readonly property var displayCells: barTextCells && barTextCells.length > 0 ? barTextCells : barCells
    readonly property int positionSlotCount: barPositionMap
        ? (barPositionMap.slotCount > 0 ? barPositionMap.slotCount : barPositionMap.count)
        : barCellCount
    readonly property int positionSlotOffset: barPositionMap ? barPositionMap.slotOffset : 0
    readonly property bool hasOverlayTimelineState: srcData && srcData.kind >= 2
    readonly property var overlayDsts: hasOverlayTimelineState ? dsts : []
    readonly property bool overlayHasStaticTimelineState: overlayTimelineTracker.canUseStaticState
    readonly property var overlayStaticTimelineState: overlayHasStaticTimelineState
        && overlayTimelineTracker.staticState.valid
        ? overlayTimelineTracker.staticState
        : null
    readonly property var overlayTimelineTimers: overlayTimelineTracker.usesDynamicTimer ? timers : null
    property Lr2TimelineState overlayTimelineTracker: Lr2TimelineState {
        enabled: root.hasOverlayTimelineState && !root.overlayHasStaticTimelineState
        dsts: root.overlayDsts
        skinTime: root.skinTime
        timers: root.overlayTimelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }
    readonly property var overlayTimelineState: hasOverlayTimelineState
        ? (overlayStaticTimelineState
           || (overlayTimelineTracker.hasState ? overlayTimelineTracker.state : null))
        : null

    function dstSummary(dst) {
        if (!dst) {
            return "null";
        }
        return "t=" + (dst.time || 0)
            + ",xywh=" + [dst.x || 0, dst.y || 0, dst.w || 0, dst.h || 0].join("/")
            + ",a=" + (dst.a === undefined ? "u" : dst.a)
            + ",rgb=" + [dst.r === undefined ? "u" : dst.r, dst.g === undefined ? "u" : dst.g, dst.b === undefined ? "u" : dst.b].join("/")
            + ",blend=" + (dst.blend === undefined ? "u" : dst.blend)
            + ",filter=" + (dst.filter === undefined ? "u" : dst.filter)
            + ",loop=" + (dst.loop === undefined ? "u" : dst.loop)
            + ",timer=" + (dst.timer === undefined ? "u" : dst.timer);
    }

    function logBodyDebug(row, slot, bodyType, source, state, bodyDsts, cell, rowData) {
        if (!source || !source.source || String(source.source).indexOf("song_bar") === -1) {
            return;
        }
        const dstCount = bodyDsts ? bodyDsts.length : 0;
        const stateValid = !!state && state.valid === true;
        const timeBucket = root.skinTime < 1100 ? "early" : (root.skinTime < 1500 ? "fade" : "final");
        const key = row + "|" + slot + "|" + bodyType + "|" + source.source
            + "|" + (stateValid ? 1 : 0)
            + "|" + dstCount
            + "|" + timeBucket
            + "|" + root.selectedRow
            + "|" + (cell ? cell.text : "");
        if (loggedBodyDebugKeys[key] || loggedBodyDebugCount >= 90) {
            return;
        }
        loggedBodyDebugKeys[key] = true;
        loggedBodyDebugCount += 1;
        console.warn("[LR2] BAR_BODY_DEBUG"
            + " row=" + row
            + " slot=" + slot
            + " selectedRow=" + root.selectedRow
            + " slotOffset=" + root.positionSlotOffset
            + " skinTime=" + root.skinTime
            + " timeBucket=" + timeBucket
            + " sourceSkinTime=" + root.sourceSkinTime
            + " bodyType=" + bodyType
            + " cellValid=" + (cell ? cell.valid : "null")
            + " cellText=\"" + (cell ? cell.text : "") + "\""
            + " cellBodyType=" + (cell ? cell.bodyType : "null")
            + " cellTitleType=" + (cell ? cell.titleType : "null")
            + " src=" + source.source
            + " srcRect=" + [source.x, source.y, source.w, source.h].join(",")
            + " stateValid=" + stateValid
            + " rawStateValid=" + (state ? state.valid : "null")
            + " state=" + (state ? [state.x, state.y, state.w, state.h, state.a, state.blend, state.filter].join(",") : "null")
            + " dstCount=" + dstCount
            + " firstDst={" + root.dstSummary(dstCount > 0 ? bodyDsts[0] : null) + "}"
            + " lastDst={" + root.dstSummary(dstCount > 0 ? bodyDsts[dstCount - 1] : null) + "}"
            + " offCount=" + (rowData && rowData.offDsts ? rowData.offDsts.length : "null")
            + " onCount=" + (rowData && rowData.onDsts ? rowData.onDsts.length : "null")
            + " scale=" + root.scaleOverride
            + " rootSize=" + root.width + "x" + root.height);
    }

    function logBodyPlacementDebug(row, slot, bodyType, source, state, cell, parentItem) {
        if (!source || !source.source || String(source.source).indexOf("song_bar") === -1) {
            return;
        }
        if (!state || state.valid !== true) {
            return;
        }
        const key = row + "|" + slot + "|" + bodyType + "|" + root.selectedRow
            + "|" + (cell ? cell.text : "")
            + "|" + Math.round(parentItem.x)
            + "|" + Math.round(parentItem.y);
        if (loggedBodyPlacementKeys[key] || loggedBodyPlacementCount >= 60) {
            return;
        }
        loggedBodyPlacementKeys[key] = true;
        loggedBodyPlacementCount += 1;
        console.warn("[LR2] BAR_BODY_PLACE_DEBUG"
            + " row=" + row
            + " slot=" + slot
            + " selectedRow=" + root.selectedRow
            + " bodyType=" + bodyType
            + " cellText=\"" + (cell ? cell.text : "") + "\""
            + " rootXY=" + root.x + "," + root.y
            + " rootSize=" + root.width + "x" + root.height
            + " parentXY=" + parentItem.x + "," + parentItem.y
            + " parentVisible=" + parentItem.visible
            + " parentRowVisible=" + parentItem.rowVisible
            + " effectiveRow=" + parentItem.effectiveRow
            + " state=" + [state.x, state.y, state.w, state.h, state.a, state.blend, state.filter].join(","));
    }

    readonly property int bodyRowIndex: {
        if (!srcData || !selectContext || srcData.kind !== 0) {
            return -1;
        }
        let total = barRows ? barRows.length : 0;
        let row = srcData.row;
        if (row <= 0 || row >= total || !barRows[row] || !barRows[row - 1]) {
            return -1;
        }
        return row;
    }
    readonly property bool bodyRowActive: bodyRowIndex >= 0
    readonly property int overlaySlotCount: {
        if (!srcData || !selectContext || srcData.kind < 2) {
            return 0;
        }

        if (srcData.kind === 4 || srcData.kind === 5 || srcData.kind === 7) {
            return 0;
        }

        // OpenLR2 draws BAR_RANK only while the IR ranking list is active.
        if (srcData.kind === 6 && !selectContext.rankingMode) {
            return 0;
        }

        let total = barRows ? barRows.length : 0;
        if (srcData.kind === 2) {
            return selectedRow > 0 && selectedRow < total ? 1 : 0;
        }
        return total;
    }

    Repeater {
        id: bodyRepeater

        model: root.bodyRowActive ? 1 : 0

        Lr2BarPositionedItem {
            id: bodyDelegate

            readonly property int bodyRow: root.bodyRowIndex
            readonly property int bodySlot: root.positionSlotCount > 0
                && bodyRow >= 0
                && bodyRow < root.positionSlotCount
                ? (bodyRow + root.positionSlotOffset) % root.positionSlotCount
                : -1
            readonly property var bodyRowData: root.barRows
                && bodyRow >= 0
                && bodyRow < root.barRows.length
                ? root.barRows[bodyRow]
                : null
            readonly property var bodyCell: root.displayCells
                && bodySlot >= 0
                && bodySlot < root.displayCells.length
                ? root.displayCells[bodySlot]
                : null
            readonly property var bodySources: root.srcData ? root.srcData.sources : null
            readonly property var fallbackBodySource: root.srcData && root.srcData.source
                ? root.srcData.source
                : null
            readonly property int bodyType: bodyCell ? bodyCell.bodyType : -1
            readonly property var typedBodySource: bodySources
                && bodyType >= 0
                && bodyType < bodySources.length
                ? bodySources[bodyType]
                : null
            readonly property var firstBodySource: bodySources && bodySources.length > 0
                ? bodySources[0]
                : null
            readonly property var bodySource: typedBodySource || firstBodySource || fallbackBodySource
            readonly property int bodySourceFrameCount: bodySource
                ? Math.max(1, bodySource.div_x || 1) * Math.max(1, bodySource.div_y || 1)
                : 1
            readonly property bool bodySourceAnimates: !!bodySource
                && (bodySource.cycle || 0) > 0
                && bodySourceFrameCount > 1
            readonly property string bodyChartAssetSource: {
                if (!bodySource) {
                    return "";
                }
                if (bodySource.specialType === 1) {
                    return root.stageFileSource;
                }
                if (bodySource.specialType === 3) {
                    return root.backBmpSource;
                }
                if (bodySource.specialType === 4) {
                    return root.bannerSource;
                }
                return "";
            }
            readonly property var bodyBaseState: root.barBaseStates
                && bodyRow >= 0
                && bodyRow < root.barBaseStates.length
                && root.barBaseStates[bodyRow].valid
                ? root.barBaseStates[bodyRow]
                : null
            readonly property bool needsStateInterpolation: !root.applyFastBarScroll
                && bodyRow >= 0
                && bodyRow < root.barStateInterpolationNeeded.length
                && !!root.barStateInterpolationNeeded[bodyRow]
            readonly property var staticBodyState: root.positionlessBarBaseStates
                && bodyRow >= 0
                && bodyRow < root.positionlessBarBaseStates.length
                && root.positionlessBarBaseStates[bodyRow].valid
                ? root.positionlessBarBaseStates[bodyRow]
                : null
            readonly property var effectiveBodyState: needsStateInterpolation ? bodyInterpolatedState : staticBodyState
            readonly property var localBodyState: {
                if (!effectiveBodyState) {
                    return null;
                }
                if (!bodyDelegate.usePositionMap) {
                    return effectiveBodyState;
                }
                return {
                    valid: effectiveBodyState.valid,
                    x: 0,
                    y: 0,
                    w: effectiveBodyState.w,
                    h: effectiveBodyState.h,
                    a: effectiveBodyState.a,
                    r: effectiveBodyState.r,
                    g: effectiveBodyState.g,
                    b: effectiveBodyState.b,
                    angle: effectiveBodyState.angle,
                    center: effectiveBodyState.center,
                    blend: effectiveBodyState.blend,
                    filter: effectiveBodyState.filter,
                    op4: effectiveBodyState.op4
                };
            }
            readonly property var bodyDsts: bodyRowData
                ? (bodyRowData.offDsts && bodyRowData.offDsts.length > 0
                   ? bodyRowData.offDsts
                   : (bodyRowData.onDsts || []))
                : []
            onBodySourceChanged: root.logBodyDebug(bodyRow, bodySlot, bodyType, bodySource, effectiveBodyState, bodyDsts, bodyCell, bodyRowData)
            onEffectiveBodyStateChanged: {
                root.logBodyDebug(bodyRow, bodySlot, bodyType, bodySource, effectiveBodyState, bodyDsts, bodyCell, bodyRowData);
                root.logBodyPlacementDebug(bodyRow, bodySlot, bodyType, bodySource, effectiveBodyState, bodyCell, bodyDelegate);
            }
            Component.onCompleted: root.logBodyDebug(bodyRow, bodySlot, bodyType, bodySource, effectiveBodyState, bodyDsts, bodyCell, bodyRowData)
            onXChanged: root.logBodyPlacementDebug(bodyRow, bodySlot, bodyType, bodySource, effectiveBodyState, bodyCell, bodyDelegate)
            onYChanged: root.logBodyPlacementDebug(bodyRow, bodySlot, bodyType, bodySource, effectiveBodyState, bodyCell, bodyDelegate)
            positionMap: root.barPositionMap
            row: bodyDelegate.bodyRow
            scaleOverride: root.scaleOverride
            usePositionMap: !needsStateInterpolation && !root.applyFastBarScroll
            fallbackX: !needsStateInterpolation && bodyBaseState ? (bodyBaseState.x || 0) : 0
            fallbackY: !needsStateInterpolation && bodyBaseState ? (bodyBaseState.y || 0) : 0
            visible: !!effectiveBodyState
            width: root.width
            height: root.height

            Lr2BarInterpolatedState {
                id: bodyInterpolatedState
                enabled: bodyDelegate.needsStateInterpolation
                positionMap: root.barPositionMap
                baseStateResolver: root.barBaseStateResolver
                row: bodyDelegate.bodyRow
            }

            Lr2SpriteRenderer {
                anchors.fill: parent
                dsts: bodyDelegate.bodyDsts
                srcData: bodyDelegate.bodySource
                sourceHasFrameAnimation: bodyDelegate.bodySourceAnimates
                stateOverride: bodyDelegate.localBodyState
                forceHidden: !bodyDelegate.effectiveBodyState
                skinTime: 0
                sourceSkinTime: bodyDelegate.bodySourceAnimates && !root.useDirectSourceSkinClock
                    ? root.sourceSkinTime
                    : 0
                skinClock: bodyDelegate.bodySourceAnimates && root.useDirectSourceSkinClock
                    ? root.skinClock
                    : null
                sourceSkinClockMode: bodyDelegate.bodySourceAnimates
                    ? root.sourceSkinClockMode
                    : 0
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                sourceTimerFire: bodyDelegate.bodySourceAnimates ? root.sourceTimerFire : -2147483648
                chartAssetSource: bodyDelegate.bodyChartAssetSource
                scaleOverride: root.scaleOverride
                transColor: root.transColor
                colorKeyEnabled: root.colorKeyEnabled
            }
        }
    }

    Repeater {
        id: overlayRepeater

        model: root.overlaySlotCount

        Lr2BarPositionedItem {
            id: overlayDelegate

            readonly property int sourceKind: root.srcData ? (root.srcData.kind || 0) : 0
            readonly property int sourceVariant: root.srcData ? (root.srcData.variant || 0) : 0
            readonly property bool selectedOverlaySource: sourceKind === 2
            readonly property var overlaySource: root.srcData ? root.srcData.source : null
            readonly property int overlaySourceFrameCount: overlaySource
                ? Math.max(1, overlaySource.div_x || 1) * Math.max(1, overlaySource.div_y || 1)
                : 1
            readonly property bool overlaySourceAnimates: !!overlaySource
                && (overlaySource.cycle || 0) > 0
                && overlaySourceFrameCount > 1
            readonly property string overlayChartAssetSource: {
                if (!overlaySource) {
                    return "";
                }
                if (overlaySource.specialType === 1) {
                    return root.stageFileSource;
                }
                if (overlaySource.specialType === 3) {
                    return root.backBmpSource;
                }
                if (overlaySource.specialType === 4) {
                    return root.bannerSource;
                }
                return "";
            }
            readonly property bool usesRowPositionMap: !selectedOverlaySource
            readonly property int selectedDisplayRow: selectedOverlaySource ? slot : -1
            readonly property var visibleBase: selectedOverlaySource
                && root.barBaseStates
                && selectedDisplayRow >= 0
                && selectedDisplayRow < root.barBaseStates.length
                && root.barBaseStates[selectedDisplayRow].valid
                ? root.barBaseStates[selectedDisplayRow]
                : null
            readonly property var cell: selectedOverlaySource
                ? null
                : (root.displayCells && slot >= 0 && slot < root.displayCells.length
                   ? root.displayCells[slot]
                   : null)
            readonly property bool cellOverlayVisible: {
                if (selectedOverlaySource || !root.selectContext || !root.srcData || !cell || !cell.valid) {
                    return false;
                }
                if (sourceKind === 3) {
                    return cell.lamp === sourceVariant;
                }
                if (sourceKind === 6) {
                    return cell.ranking && cell.rank === sourceVariant;
                }
                if (sourceKind === 8) {
                    return sourceVariant >= 0
                        && sourceVariant < 31
                        && (cell.labelMask & (1 << sourceVariant)) !== 0;
                }
                return false;
            }
            readonly property bool contentVisible: (selectedOverlaySource
                    ? selectedDisplayRow > 0 && !!visibleBase
                    : rowVisible)
                && (selectedOverlaySource || cellOverlayVisible)
            positionMap: root.barPositionMap
            row: selectedOverlaySource ? selectedDisplayRow : -1
            slot: selectedOverlaySource ? root.selectedRow : modelData
            scaleOverride: root.scaleOverride
            useSlotRow: usesRowPositionMap
            usePositionMap: usesRowPositionMap
            hasOverride: false
            fallbackX: selectedOverlaySource && visibleBase ? visibleBase.x : 0
            fallbackY: selectedOverlaySource && visibleBase ? visibleBase.y : 0
            width: root.width
            height: root.height
            visible: contentVisible

            Lr2SpriteRenderer {
                anchors.fill: parent
                dsts: root.dsts
                srcData: overlayDelegate.overlaySource
                sourceHasFrameAnimation: overlayDelegate.overlaySourceAnimates
                forceHidden: !overlayDelegate.contentVisible
                skinTime: 0
                sourceSkinTime: overlayDelegate.overlaySourceAnimates && !root.useDirectSourceSkinClock
                    ? root.sourceSkinTime
                    : 0
                skinClock: overlayDelegate.overlaySourceAnimates && root.useDirectSourceSkinClock
                    ? root.skinClock
                    : null
                sourceSkinClockMode: overlayDelegate.overlaySourceAnimates
                    ? root.sourceSkinClockMode
                    : 0
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                sourceTimerFire: overlayDelegate.overlaySourceAnimates ? root.sourceTimerFire : -2147483648
                chartAssetSource: overlayDelegate.overlayChartAssetSource
                scaleOverride: root.scaleOverride
                transColor: root.transColor
                colorKeyEnabled: root.colorKeyEnabled
                stateOverride: root.overlayTimelineState
            }
        }
    }
}
