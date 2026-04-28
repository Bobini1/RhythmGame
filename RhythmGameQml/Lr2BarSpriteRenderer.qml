import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property int sourceSkinTime: skinTime
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property var chart
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barBaseStates: []
    property var barDrawXs: []
    property var barDrawYs: []
    property var barCells: []
    property real barScrollOffset: 0
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property int barCenter: 0
    property var barLampVariants: []
    property color transColor: "black"
    property bool colorKeyEnabled: false
    readonly property bool applyFastBarScroll: fastBarScrollActive && (!srcData || srcData.kind !== 2)
    x: applyFastBarScroll ? fastBarScrollX * scaleOverride : 0
    y: applyFastBarScroll ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property bool hasOverlayTimelineState: srcData && srcData.kind >= 2
    readonly property var overlayDsts: hasOverlayTimelineState ? dsts : []
    readonly property bool overlayHasStaticTimelineState: Lr2Timeline.canUseStaticState(overlayDsts)
    readonly property var overlayStaticTimelineState: overlayHasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(overlayDsts[0], overlayDsts[0])
        : null
    readonly property var overlayTimelineTimers: Lr2Timeline.dstsUseDynamicTimer(overlayDsts) ? timers : null
    readonly property var overlayTimelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(overlayDsts) ? activeOptions : []
    readonly property var overlayTimelineState: hasOverlayTimelineState
        ? (overlayStaticTimelineState
           || Lr2Timeline.getCurrentState(overlayDsts, skinTime, overlayTimelineTimers, overlayTimelineActiveOptions))
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

    readonly property var overlayRows: {
        if (!srcData || !selectContext || srcData.kind < 2) {
            return [];
        }

        if (srcData.kind === 4 || srcData.kind === 5 || srcData.kind === 7) {
            return [];
        }

        // OpenLR2 draws BAR_RANK only while the IR ranking list is active.
        if (srcData.kind === 6 && !selectContext.rankingMode) {
            return [];
        }

        let total = barRows ? barRows.length : 0;
        if (srcData.kind === 2) {
            return selectedRow > 0 && selectedRow < total ? [selectedRow] : [];
        }

        let rows = [];
        for (let row = 1; row < total; ++row) {
            rows.push(row);
        }
        return rows;
    }

    function rowData(row) {
        return barRows && row >= 0 && row < barRows.length ? barRows[row] : null;
    }

    function visibilityState(row) {
        return cachedBaseState(row);
    }

    function interpolateRowState(fromState, toState, progress) {
        if (!fromState || !toState) {
            return fromState;
        }
        let inv = 1.0 - progress;
        return {
            x: fromState.x * inv + toState.x * progress,
            y: fromState.y * inv + toState.y * progress,
            w: fromState.w * inv + toState.w * progress,
            h: fromState.h * inv + toState.h * progress,
            a: fromState.a * inv + toState.a * progress,
            r: fromState.r * inv + toState.r * progress,
            g: fromState.g * inv + toState.g * progress,
            b: fromState.b * inv + toState.b * progress,
            blend: fromState.blend,
            filter: fromState.filter,
            angle: fromState.angle * inv + toState.angle * progress,
            center: fromState.center,
            sortId: (fromState.sortId || 0) * inv + (toState.sortId || 0) * progress
        };
    }

    function rowDrawState(row) {
        if (srcData && srcData.kind === 2) {
            return cachedBaseState(row);
        }
        let fromState = cachedBaseState(row);
        let offset = barScrollOffset || 0;
        if (!fromState || offset <= 0.001 || applyFastBarScroll) {
            return fromState;
        }
        let toState = row > 0 ? cachedBaseState(row - 1) : null;
        return fromState && toState
            ? interpolateRowState(fromState, toState, offset)
            : fromState;
    }

    function rowDrawX(row) {
        if (srcData && srcData.kind === 2) {
            let state = cachedBaseState(row);
            return state ? state.x : 0;
        }
        return barDrawXs && row >= 0 && row < barDrawXs.length
            ? barDrawXs[row]
            : (cachedBaseState(row) ? cachedBaseState(row).x : 0);
    }

    function rowDrawY(row) {
        if (srcData && srcData.kind === 2) {
            let state = cachedBaseState(row);
            return state ? state.y : 0;
        }
        return barDrawYs && row >= 0 && row < barDrawYs.length
            ? barDrawYs[row]
            : (cachedBaseState(row) ? cachedBaseState(row).y : 0);
    }

    function cachedBaseState(row) {
        return barBaseStates && row >= 0 && row < barBaseStates.length
            ? barBaseStates[row]
            : null;
    }

    function cellData(row) {
        return barCells && row >= 0 && row < barCells.length ? barCells[row] : null;
    }

    function bodyDsts(row) {
        let data = rowData(row);
        if (!data) {
            return [];
        }
        return data.offDsts && data.offDsts.length > 0 ? data.offDsts : (data.onDsts || []);
    }

    function sourceForBody(row) {
        if (!selectContext || !srcData || !srcData.sources) {
            return srcData && srcData.source ? srcData.source : null;
        }
        let cell = cellData(row);
        let bodyType = cell ? (cell.bodyType || 0) : 0;
        return srcData.sources[bodyType] || srcData.sources[0] || srcData.source || null;
    }

    function overlayVisibleFor(cell) {
        if (!selectContext || !srcData) {
            return false;
        }
        if (srcData.kind === 2) {
            return true;
        }
        if (!cell) {
            return false;
        }
        switch (srcData.kind) {
        case 3:
            return (cell.lamp || 0) === srcData.variant;
        case 6:
            return !!cell.ranking && (cell.rank || 0) === srcData.variant;
        default:
            return false;
        }
    }

    function spriteSourceSkinTime(source) {
        return Lr2Timeline.srcCyclesContinuously(source)
            ? root.sourceSkinTime
            : root.skinTime;
    }

    Repeater {
        model: root.bodyRows

        Lr2SpriteRenderer {
            readonly property int row: modelData
            readonly property var bodySource: root.sourceForBody(row)
            readonly property var bodyState: root.rowDrawState(row)
            visible: !!bodyState
            anchors.fill: parent
            dsts: root.bodyDsts(row)
            srcData: bodySource
            stateOverride: bodyState
            skinTime: root.skinTime
            sourceSkinTime: root.spriteSourceSkinTime(bodySource)
            activeOptions: root.activeOptions
            timers: root.timers
            chart: root.chart
            scaleOverride: root.scaleOverride
            transColor: root.transColor
            colorKeyEnabled: false
        }
    }

    Repeater {
        model: root.overlayRows

        Item {
            readonly property int row: modelData
            readonly property real drawX: root.rowDrawX(row)
            readonly property real drawY: root.rowDrawY(row)
            readonly property var visibleBase: root.visibilityState(row)
            readonly property var cell: root.cellData(row)
            readonly property bool contentVisible: !!visibleBase && root.overlayVisibleFor(cell)
            x: drawX * root.scaleOverride
            y: drawY * root.scaleOverride
            width: root.width
            height: root.height
            visible: contentVisible

            Lr2SpriteRenderer {
                anchors.fill: parent
                dsts: root.dsts
                srcData: root.srcData ? root.srcData.source : null
                skinTime: root.skinTime
                sourceSkinTime: root.spriteSourceSkinTime(srcData)
                activeOptions: root.activeOptions
                timers: root.timers
                chart: root.chart
                scaleOverride: root.scaleOverride
                transColor: root.transColor
                colorKeyEnabled: root.colorKeyEnabled
                stateOverride: root.overlayTimelineState
            }
        }
    }
}
