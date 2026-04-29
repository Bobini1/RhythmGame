import QtQuick
import RhythmGameQml 1.0

import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property int sourceTimerFire: -2147483648
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barBaseStates: []
    property var barPositionCache
    property var barCells: []
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property real selectedFastBarDrawX: 0
    property real selectedFastBarDrawY: 0
    property int barCenter: 0
    property bool colorKeyEnabled: false
    property color transColor: "black"
    x: fastBarScrollActive ? fastBarScrollX * scaleOverride : 0
    y: fastBarScrollActive ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property int barCellCount: barCells ? barCells.length : 0
    readonly property int barCellSlotOffset: selectContext ? selectContext.visibleBarSlotOffset : 0

    readonly property int numberSlotCount: barRows && selectContext && srcData
        ? Math.max(0, barRows.length)
        : 0
    readonly property var numberDsts: srcData && srcData.source ? dsts : []
    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(numberDsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(numberDsts[0], numberDsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(numberDsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(numberDsts) ? activeOptions : []
    readonly property var numberTimelineState: staticTimelineState
        || Lr2Timeline.getCurrentStateWithOptionalTimerFire(
            numberDsts, skinTime, timelineTimers, timerFire, timelineActiveOptions)
    readonly property bool numberSourceAnimates: srcData && srcData.source
        && (srcData.source.cycle || 0) > 0

    function baseState(row) {
        return barBaseStates && row >= 0 && row < barBaseStates.length
            ? barBaseStates[row]
            : null;
    }

    function visibilityState(row) {
        return baseState(row);
    }

    function cellData(row) {
        return barCells && row >= 0 && row < barCells.length ? barCells[row] : null;
    }

    function visibleForCell(cell) {
        if (!cell || !srcData || !selectContext) {
            return false;
        }
        if (!cell.ranking && !cell.chartLike && !cell.entryLike) {
            return false;
        }
        if (cell.ranking) {
            return srcData.variant === 6;
        }
        if ((cell.keymode || 0) <= 0 || (cell.playLevel || 0) <= 0) {
            return false;
        }
        let difficulty = cell.difficulty || 0;
        if (difficulty <= 0) {
            return srcData.variant === 0;
        }
        return srcData.variant === difficulty;
    }

    function displayRowForSlot(slot) {
        let count = root.barCellCount;
        return count > 0 ? ((slot - root.barCellSlotOffset) % count + count) % count : -1;
    }

    function refreshNumberCellRange(firstSlot, changedCount, force) {
        let count = numberRepeater.count;
        if (count <= 0 || changedCount <= 0) {
            return;
        }
        if (count !== root.barCellCount || changedCount >= count) {
            root.refreshNumberCells(force);
            return;
        }
        for (let i = 0; i < changedCount; ++i) {
            root.refreshNumberCell((firstSlot + i) % count, force);
        }
    }

    function refreshNumberCells(force) {
        for (let i = 0; i < numberRepeater.count; ++i) {
            refreshNumberCell(i, force);
        }
    }

    function refreshNumberCell(slot, force) {
        let item = numberRepeater.itemAt(slot);
        if (item) {
            item.refreshCell(force);
        }
    }

    Repeater {
        id: numberRepeater

        model: root.numberSlotCount

        Lr2BarPositionedItem {
            id: barNumberDelegate

            readonly property int slot: modelData
            property var cell: null
            readonly property int displayRow: root.displayRowForSlot(slot)
            readonly property var visibleBase: root.visibilityState(displayRow)
            readonly property bool selectedRowContent: displayRow === root.selectedRow
            readonly property bool contentVisible: displayRow > 0
                && !!visibleBase
                && root.visibleForCell(cell)
            positionCache: root.barPositionCache
            row: displayRow
            scaleOverride: root.scaleOverride
            usePositionCache: true
            hasOverride: root.fastBarScrollActive && selectedRowContent
            overrideX: root.selectedFastBarDrawX
            overrideY: root.selectedFastBarDrawY
            adjustX: hasOverride ? root.fastBarScrollX : 0
            adjustY: hasOverride ? root.fastBarScrollY : 0
            fallbackX: visibleBase ? visibleBase.x : 0
            fallbackY: visibleBase ? visibleBase.y : 0
            width: root.width
            height: root.height
            visible: contentVisible

            function cellAtSlot() {
                return root.barCells && slot >= 0 && slot < root.barCells.length
                    ? root.barCells[slot]
                    : null;
            }

            function refreshCell(force) {
                let next = cellAtSlot();
                if (force && cell === next) {
                    cell = null;
                }
                if (cell !== next) {
                    cell = next;
                }
            }

            Component.onCompleted: {
                refreshCell(false);
            }
            onSlotChanged: {
                refreshCell(false);
            }

            Lr2NumberRenderer {
                anchors.fill: parent
                dsts: root.dsts
                srcData: root.srcData ? root.srcData.source : null
                skinTime: root.numberSourceAnimates ? root.skinTime : 0
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                sourceTimerFire: root.numberSourceAnimates ? root.sourceTimerFire : -2147483648
                scaleOverride: root.scaleOverride
                colorKeyEnabled: root.colorKeyEnabled
                transColor: root.transColor
                stateOverride: root.numberTimelineState
                value: parent.cell ? (parent.cell.playLevel || 0) : 0
            }
        }
    }

    onBarCellsChanged: refreshNumberCells(false)

    Connections {
        target: root.selectContext
        function onBarTextCellsInvalidated() {
            root.refreshNumberCells(false);
        }
        function onBarTextCellRangeChanged(firstSlot, count) {
            root.refreshNumberCellRange(firstSlot, count, true);
        }
    }
}
