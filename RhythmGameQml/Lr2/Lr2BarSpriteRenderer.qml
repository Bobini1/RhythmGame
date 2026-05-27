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
    readonly property int barBaseStateRevision: barBaseStateResolver
        ? barBaseStateResolver.baseStatesRevision
        : 0
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
    readonly property bool hasOverlayTimelineState: srcData && srcData.kind >= 2
    readonly property var overlayDsts: hasOverlayTimelineState ? dsts : []
    readonly property bool overlayHasStaticTimelineState: overlayTimelineTracker.canUseStaticState
    readonly property var overlayStaticTimelineState: overlayHasStaticTimelineState
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
           || overlayTimelineTracker.state)
        : null

    readonly property var bodyRows: {
        if (!srcData || !selectContext || srcData.kind !== 0) {
            return [];
        }
        let total = barRows ? barRows.length : 0;
        let row = srcData.row;
        if (row <= 0 || row >= total || !rowData(row) || !rowData(row - 1)) {
            return [];
        }
        return [row];
    }
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

    function rowData(row: var) : var {
        return barRows && row >= 0 && row < barRows.length ? barRows[row] : null;
    }

    function sourceAt(sources: var, sourceIndex: var) : var {
        if (!sources || sourceIndex < 0) {
            return null;
        }
        return sourceIndex < sources.length ? sources[sourceIndex] : null;
    }

    function chartAssetSourceFor(source: var) : var {
        return Lr2SkinUtils.chartAssetSourceFor(
            source,
            stageFileSource,
            backBmpSource,
            bannerSource);
    }

    function visibilityState(row: var) : var {
        return baseStateAt(row);
    }

    function bodyNeedsStateInterpolation(row: var) : var {
        if (applyFastBarScroll) {
            return false;
        }
        root.barBaseStateRevision;
        return barBaseStateResolver ? barBaseStateResolver.stateNeedsInterpolationAt(row) : false;
    }

    function positionlessStateAt(row: var) : var {
        root.barBaseStateRevision;
        return barBaseStateResolver ? barBaseStateResolver.positionlessStateAt(row) : null;
    }

    function baseStateXAt(row: var) : var {
        root.barBaseStateRevision;
        return barBaseStateResolver ? barBaseStateResolver.stateXAt(row) : 0;
    }

    function baseStateYAt(row: var) : var {
        root.barBaseStateRevision;
        return barBaseStateResolver ? barBaseStateResolver.stateYAt(row) : 0;
    }

    function baseStateAt(row: var) : var {
        root.barBaseStateRevision;
        return barBaseStateResolver && row >= 0 && row < barBaseStateResolver.stateCount()
            ? barBaseStateResolver.stateAt(row)
            : null;
    }

    function slotForDisplayRow(row: var) : var {
        let count = root.barCellCount;
        if (count <= 0 || row < 0 || row >= count) {
            return -1;
        }
        let offset = root.barPositionMap ? root.barPositionMap.slotOffset : 0;
        return root.barPositionMap
            ? root.barPositionMap.slotForRow(row)
            : (row + offset) % count;
    }

    function cellData(row: var) : var {
        let cells = barTextCells && barTextCells.length > 0 ? barTextCells : barCells;
        let slot = slotForDisplayRow(row);
        return cells && slot >= 0 && slot < cells.length ? cells[slot] : null;
    }

    function cellDataForSlot(slot: var) : var {
        let cells = barTextCells && barTextCells.length > 0 ? barTextCells : barCells;
        return cells && slot >= 0 && slot < cells.length ? cells[slot] : null;
    }

    function bodyDsts(row: var) : var {
        let data = rowData(row);
        if (!data) {
            return [];
        }
        return data.offDsts && data.offDsts.length > 0 ? data.offDsts : (data.onDsts || []);
    }

    function overlayVisibleForValues(cellValid: var, lamp: var, ranking: var, rank: var) : var {
        if (!selectContext || !srcData) {
            return false;
        }
        if (srcData.kind === 2) {
            return true;
        }
        if (!cellValid) {
            return false;
        }
        switch (srcData.kind) {
        case 3:
            return (lamp || 0) === srcData.variant;
        case 6:
            return !!ranking && (rank || 0) === srcData.variant;
        default:
            return false;
        }
    }

    Repeater {
        id: bodyRepeater

        model: root.bodyRows

        Lr2BarPositionedItem {
            id: bodyDelegate

            readonly property int bodyRow: modelData
            readonly property bool needsStateInterpolation: root.bodyNeedsStateInterpolation(bodyRow)
            readonly property var bodyCell: root.cellData(bodyRow)
            readonly property var bodySource: {
                let fallback = root.srcData && root.srcData.source ? root.srcData.source : null;
                if (!bodyCell || !root.srcData || !root.srcData.sources) {
                    return fallback;
                }
                return root.sourceAt(root.srcData.sources, bodyCell.bodyType)
                    || root.sourceAt(root.srcData.sources, 0)
                    || fallback;
            }
            readonly property bool bodySourceAnimates: Lr2SkinUtils.sourceCyclesContinuously(bodySource)
            readonly property var staticBodyState: root.positionlessStateAt(bodyRow)
            readonly property var effectiveBodyState: needsStateInterpolation ? bodyInterpolatedState : staticBodyState
            positionMap: root.barPositionMap
            row: bodyDelegate.bodyRow
            scaleOverride: root.scaleOverride
            usePositionMap: !needsStateInterpolation && !root.applyFastBarScroll
            fallbackX: !needsStateInterpolation ? root.baseStateXAt(bodyRow) : 0
            fallbackY: !needsStateInterpolation ? root.baseStateYAt(bodyRow) : 0
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
                dsts: root.bodyDsts(bodyDelegate.bodyRow)
                srcData: bodyDelegate.bodySource
                stateOverride: bodyDelegate.effectiveBodyState
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
                chartAssetSource: root.chartAssetSourceFor(bodyDelegate.bodySource)
                scaleOverride: root.scaleOverride
                transColor: root.transColor
                colorKeyEnabled: false
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
            readonly property bool overlaySourceAnimates: Lr2SkinUtils.sourceCyclesContinuously(overlaySource)
            readonly property bool usesRowPositionMap: !selectedOverlaySource
            readonly property int selectedDisplayRow: selectedOverlaySource ? slot : -1
            readonly property var visibleBase: selectedOverlaySource ? root.visibilityState(selectedDisplayRow) : null
            readonly property var cell: selectedOverlaySource
                ? null
                : root.cellDataForSlot(slot)
            readonly property bool cellValid: selectedOverlaySource ? true : !!cell
            readonly property bool cellOverlayVisible: !selectedOverlaySource
                && cell
                && cell.valid
                && (sourceKind === 3
                    ? cell.lamp === sourceVariant
                    : (sourceKind === 6
                        ? cell.ranking && cell.rank === sourceVariant
                        : (sourceKind === 8
                            ? sourceVariant >= 0
                                && sourceVariant < 31
                                && (cell.labelMask & (1 << sourceVariant)) !== 0
                            : false)))
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
                chartAssetSource: root.chartAssetSourceFor(overlayDelegate.overlaySource)
                scaleOverride: root.scaleOverride
                transColor: root.transColor
                colorKeyEnabled: root.colorKeyEnabled
                stateOverride: root.overlayTimelineState
            }
        }
    }
}

