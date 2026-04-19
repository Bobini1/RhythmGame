import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property var chart
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property int barCenter: 0
    readonly property int contextRevision: selectContext ? selectContext.revision : 0
    readonly property int visualBaseIndex: selectContext ? selectContext.visualBaseIndex : 0
    readonly property real scrollOffset: selectContext ? selectContext.scrollOffset : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter

    readonly property var bodyRows: {
        if (!srcData || !selectContext || srcData.kind !== 0 || srcData.row !== 0) {
            return [];
        }

        let rows = [];
        let total = barRows ? barRows.length : 0;
        for (let row = 1; row < total; ++row) {
            if (rowData(row) && rowData(row - 1)) {
                rows.push(row);
            }
        }
        return rows;
    }

    readonly property var overlayRows: {
        if (!srcData || !selectContext || srcData.kind < 2) {
            return [];
        }

        // These LR2 overlay kinds are not supported yet and would otherwise
        // create hidden delegates for every bar row.
        if (srcData.kind === 4 || srcData.kind === 5 || srcData.kind === 7) {
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

    function baseState(row) {
        if (srcData && srcData.kind === 2) {
            return rawBaseState(row, true);
        }
        return rowDrawState(row);
    }

    function rowDrawState(row) {
        let fromState = rawBaseState(row, row === selectedRow);
        if (scrollOffset <= 0.001 || !fromState) {
            return fromState;
        }
        return interpolateState(fromState,
                                rawBaseState(row - 1, row - 1 === selectedRow),
                                scrollOffset);
    }

    function rawBaseState(row, useOn) {
        let data = rowData(row);
        if (!data) {
            return null;
        }
        let dstList = useOn && data.onDsts && data.onDsts.length > 0
            ? data.onDsts
            : data.offDsts;
        if (!dstList || dstList.length === 0) {
            dstList = data.onDsts || [];
        }
        return Lr2Timeline.getCurrentState(dstList, skinTime, timers, activeOptions);
    }

    function interpolateState(fromState, toState, progress) {
        if (!toState) {
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
            center: fromState.center
        };
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
        let entry = selectContext.barEntry(row, barCenter);
        let bodyType = selectContext.entryBodyType(entry);
        return srcData.sources[bodyType] || srcData.sources[0] || srcData.source || null;
    }

    function overlayVisibleFor(entry) {
        if (!selectContext || !srcData) {
            return false;
        }
        if (srcData.kind === 2) {
            return true;
        }
        if (!entry) {
            return false;
        }
        switch (srcData.kind) {
        case 3:
            return selectContext.entryLamp(entry) === srcData.variant;
        case 6:
            return selectContext.entryRank(entry) === srcData.variant;
        default:
            return false;
        }
    }

    Repeater {
        model: root.bodyRows

        Lr2SpriteRenderer {
            readonly property int row: modelData
            readonly property var entry: {
                let revision = root.contextRevision;
                let base = root.visualBaseIndex;
                return root.selectContext ? root.selectContext.barEntry(row, root.barCenter) : null;
            }
            readonly property var bodySource: {
                let revision = root.contextRevision;
                let base = root.visualBaseIndex;
                return root.sourceForBody(row);
            }
            readonly property var bodyState: root.rowDrawState(row)
            visible: !!bodyState
            anchors.fill: parent
            dsts: root.bodyDsts(row)
            srcData: bodySource
            stateOverride: bodyState
            skinTime: root.skinTime
            activeOptions: root.activeOptions
            timers: root.timers
            chart: root.chart
            scaleOverride: root.scaleOverride
            colorKeyEnabled: false
        }
    }

    Repeater {
        model: root.overlayRows

        Lr2SpriteRenderer {
            readonly property int row: modelData
            readonly property var base: root.baseState(row)
            readonly property var entry: {
                let revision = root.contextRevision;
                let baseIndex = root.visualBaseIndex;
                return root.selectContext ? root.selectContext.barEntry(row, root.barCenter) : null;
            }
            visible: !!base && root.overlayVisibleFor(entry)
            dsts: root.dsts
            srcData: root.srcData ? root.srcData.source : null
            skinTime: root.skinTime
            activeOptions: root.activeOptions
            timers: root.timers
            chart: root.chart
            scaleOverride: root.scaleOverride
            offsetX: base ? base.x : 0
            offsetY: base ? base.y : 0
        }
    }
}
