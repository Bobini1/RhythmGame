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
    property real barScrollOffset: 0
    property int barCenter: 0
    property color transColor: "black"
    readonly property int contextRevision: selectContext ? selectContext.listRevision + selectContext.scoreRevision + selectContext.folderLampRevision : 0
    readonly property int visualBaseIndex: selectContext ? selectContext.visualBaseIndex : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter

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

    function rowDrawState(row) {
        if (srcData && srcData.kind === 2) {
            return cachedBaseState(row);
        }
        return interpolatedState(row);
    }

    function cachedBaseState(row) {
        return barBaseStates && row >= 0 && row < barBaseStates.length
            ? barBaseStates[row]
            : null;
    }

    function interpolatedState(row) {
        let fromState = cachedBaseState(row);
        let offset = barScrollOffset || 0;
        if (!fromState || offset <= 0.001) {
            return fromState;
        }

        let toState = row > 0 ? cachedBaseState(row - 1) : null;
        if (!toState) {
            return fromState;
        }

        let inv = 1.0 - offset;
        return {
            x: fromState.x * inv + toState.x * offset,
            y: fromState.y * inv + toState.y * offset,
            w: fromState.w * inv + toState.w * offset,
            h: fromState.h * inv + toState.h * offset,
            a: fromState.a * inv + toState.a * offset,
            r: fromState.r * inv + toState.r * offset,
            g: fromState.g * inv + toState.g * offset,
            b: fromState.b * inv + toState.b * offset,
            blend: fromState.blend,
            filter: fromState.filter,
            angle: fromState.angle * inv + toState.angle * offset,
            center: fromState.center,
            sortId: fromState.sortId || 0
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
            return selectContext.isRankingEntry(entry)
                && selectContext.entryRank(entry) === srcData.variant;
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

        Lr2SpriteRenderer {
            readonly property int row: modelData
            readonly property var drawBase: root.rowDrawState(row)
            readonly property var visibleBase: root.visibilityState(row)
            readonly property var entry: {
                let revision = root.contextRevision;
                let baseIndex = root.visualBaseIndex;
                return root.selectContext ? root.selectContext.barEntry(row, root.barCenter) : null;
            }
            readonly property bool contentVisible: !!visibleBase && root.overlayVisibleFor(entry)
            visible: contentVisible && !!drawBase
            dsts: root.dsts
            srcData: root.srcData ? root.srcData.source : null
            skinTime: root.skinTime
            sourceSkinTime: root.spriteSourceSkinTime(srcData)
            activeOptions: root.activeOptions
            timers: root.timers
            chart: root.chart
            scaleOverride: root.scaleOverride
            transColor: root.transColor
            offsetX: drawBase ? drawBase.x : 0
            offsetY: drawBase ? drawBase.y : 0
        }
    }
}
