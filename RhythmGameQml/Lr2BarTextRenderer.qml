import QtQuick

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

    function drawOffset(row, axis) {
        let fromState = baseState(row);
        if (!fromState) {
            return 0;
        }

        let offset = barScrollOffset || 0;
        if (offset <= 0.001 || row <= 0) {
            return axis === 0 ? fromState.x : fromState.y;
        }

        let toState = baseState(row - 1);
        if (!toState) {
            return axis === 0 ? fromState.x : fromState.y;
        }

        let fromValue = axis === 0 ? fromState.x : fromState.y;
        let toValue = axis === 0 ? toState.x : toState.y;
        return fromValue + (toValue - fromValue) * offset;
    }

    Repeater {
        model: root.textSlotCount

        Lr2TextRenderer {
            id: barTextDelegate

            readonly property int slot: modelData
            property var cellEntry: null
            property int cellTitleType: -1
            property int displayRow: -1
            property string cellText: ""
            readonly property var visibleBase: root.visibilityState(displayRow)
            readonly property bool contentVisible: displayRow > 0
                && !!visibleBase
                && root.visibleFor(cellEntry, cellTitleType)

            function refreshCell() {
                let cell = root.selectContext ? root.selectContext.visibleBarTextCell(slot) : null;
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

            visible: contentVisible
            dsts: root.timelineDsts
            srcData: root.srcData
            skinTime: root.skinTime
            activeOptions: root.activeOptions
            timers: root.timers
            scaleOverride: root.scaleOverride
            offsetX: root.drawOffset(displayRow, 0)
            offsetY: root.drawOffset(displayRow, 1)
            resolvedText: cellText
        }
    }
}
