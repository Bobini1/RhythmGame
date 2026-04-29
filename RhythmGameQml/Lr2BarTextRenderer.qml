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
    x: fastBarScrollActive ? fastBarScrollX * scaleOverride : 0
    y: fastBarScrollActive ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property int barCellCount: barCells ? barCells.length : 0
    readonly property int barCellSlotOffset: selectContext ? selectContext.visibleBarSlotOffset : 0
    readonly property var timelineDsts: {
        if (!dsts || dsts.length === 0 || !dsts[0]) {
            return dsts;
        }

        // OpenLR2's select bar title path applies the type-0/type-1 source
        // choice itself and does not gate #DST_BAR_TITLE by opt1/opt2/opt3.
        let result = dsts.slice();
        let first = {};
        for (let key in dsts[0]) {
            first[key] = dsts[0][key];
        }
        first.op1 = 0;
        first.op2 = 0;
        first.op3 = 0;
        result[0] = first;
        return result;
    }

    readonly property int textSlotCount: barRows && selectContext && srcData
        ? Math.max(0, barRows.length)
        : 0
    readonly property bool hasStaticTimelineState: Lr2Timeline.canUseStaticState(timelineDsts)
    readonly property var staticTimelineState: hasStaticTimelineState
        ? Lr2Timeline.copyDstAsState(timelineDsts[0], timelineDsts[0])
        : null
    readonly property var timelineTimers: Lr2Timeline.dstsUseDynamicTimer(timelineDsts) ? timers : null
    readonly property var timelineActiveOptions: Lr2Timeline.dstsUseActiveOptions(timelineDsts) ? activeOptions : []
    readonly property var textTimelineState: staticTimelineState
        || Lr2Timeline.getCurrentStateWithOptionalTimerFire(
            timelineDsts, skinTime, timelineTimers, timerFire, timelineActiveOptions)

    function visibleFor(cellEntry, cellTitleType) {
        return !!cellEntry && !!srcData && !!selectContext
            && cellTitleType === srcData.titleType;
    }

    function baseState(row) {
        return barBaseStates && row >= 0 && row < barBaseStates.length
            ? barBaseStates[row]
            : null;
    }

    function visibilityState(row) {
        return baseState(row);
    }

    function displayRowForSlot(slot) {
        let count = root.barCellCount;
        return count > 0 ? ((slot - root.barCellSlotOffset) % count + count) % count : -1;
    }

    function refreshTextCell(slot) {
        let item = textRepeater.itemAt(slot);
        if (item) {
            item.refreshCell();
        }
    }

    function refreshTextCells() {
        for (let slot = 0; slot < textRepeater.count; ++slot) {
            refreshTextCell(slot);
        }
    }

    function refreshTextCellRange(firstSlot, changedCount) {
        let count = textRepeater.count;
        if (count <= 0 || changedCount <= 0) {
            return;
        }
        if (count !== root.barCellCount || changedCount >= count) {
            root.refreshTextCells();
            return;
        }
        for (let i = 0; i < changedCount; ++i) {
            root.refreshTextCell((firstSlot + i) % count);
        }
    }

    Repeater {
        id: textRepeater

        model: root.textSlotCount

        Lr2BarPositionedItem {
            id: barTextDelegate

            readonly property int slot: modelData
            property var cellEntry: null
            property int cellTitleType: -1
            readonly property int displayRow: root.displayRowForSlot(slot)
            property string cellText: ""
            readonly property bool selectedRowContent: displayRow === root.selectedRow
            readonly property var visibleBase: root.visibilityState(displayRow)
            readonly property bool contentVisible: displayRow > 0
                && !!visibleBase
                && root.visibleFor(cellEntry, cellTitleType)
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

            function refreshCell() {
                let cell = cellAtSlot();
                let nextEntry = cell ? cell.entry : null;
                let nextTitleType = cell ? cell.titleType : -1;
                let nextText = cell ? (cell.text || "") : "";
                if (cellEntry !== nextEntry) {
                    cellEntry = nextEntry;
                }
                if (cellTitleType !== nextTitleType) {
                    cellTitleType = nextTitleType;
                }
                if (cellText !== nextText) {
                    cellText = nextText;
                }
            }

            Component.onCompleted: {
                refreshCell();
            }
            onSlotChanged: {
                refreshCell();
            }

            Connections {
                target: root
                function onSelectContextChanged() {
                    barTextDelegate.refreshCell();
                }
            }

            Lr2TextRenderer {
                anchors.fill: parent
                dsts: root.timelineDsts
                srcData: root.srcData
                skinTime: 0
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                scaleOverride: root.scaleOverride
                stateOverride: root.textTimelineState
                resolvedText: barTextDelegate.cellText
            }
        }
    }

    onBarCellsChanged: refreshTextCells()

    Connections {
        target: root.selectContext
        function onBarTextCellsInvalidated() {
            root.refreshTextCells();
        }
        function onBarTextCellRangeChanged(firstSlot, count) {
            root.refreshTextCellRange(firstSlot, count);
        }
    }
}
