import QtQuick

import "Lr2Timeline.js" as Lr2Timeline

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barBaseStates: []
    property var barDrawStates: []
    property var barCells: []
    property real barScrollOffset: 0
    property int barCenter: 0
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
        || Lr2Timeline.getCurrentState(timelineDsts, skinTime, timelineTimers, timelineActiveOptions)

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

    Repeater {
        model: root.textSlotCount

        Item {
            id: barTextDelegate

            readonly property int slot: modelData
            property var cellEntry: null
            property int cellTitleType: -1
            property int displayRow: -1
            property string cellText: ""
            readonly property var drawState: root.barDrawStates && displayRow >= 0 && displayRow < root.barDrawStates.length
                ? root.barDrawStates[displayRow]
                : root.baseState(displayRow)
            readonly property var visibleBase: root.visibilityState(displayRow)
            readonly property bool contentVisible: displayRow > 0
                && !!visibleBase
                && root.visibleFor(cellEntry, cellTitleType)
            x: drawState ? drawState.x * root.scaleOverride : 0
            y: drawState ? drawState.y * root.scaleOverride : 0
            width: root.width
            height: root.height
            visible: contentVisible

            function refreshCell() {
                let cell = root.barCells && slot >= 0 && slot < root.barCells.length
                    ? root.barCells[slot]
                    : null;
                let nextRow = cell ? cell.row : -1;
                let nextEntry = cell ? cell.entry : null;
                let nextTitleType = cell ? cell.titleType : -1;
                let nextText = cell ? (cell.text || "") : "";
                if (displayRow !== nextRow) {
                    displayRow = nextRow;
                }
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

            Component.onCompleted: refreshCell()
            onSlotChanged: refreshCell()

            Connections {
                target: root.selectContext
                function onBarTextCellsInvalidated() {
                    barTextDelegate.refreshCell();
                }
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
                skinTime: root.skinTime
                activeOptions: root.activeOptions
                timers: root.timers
                scaleOverride: root.scaleOverride
                stateOverride: root.textTimelineState
                resolvedText: barTextDelegate.cellText
            }
        }
    }
}
