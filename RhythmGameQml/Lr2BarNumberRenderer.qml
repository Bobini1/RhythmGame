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
    property bool colorKeyEnabled: false
    property color transColor: "black"
    readonly property int contextRevision: selectContext ? selectContext.listRevision + selectContext.barEntriesRevision : 0

    readonly property var numberRows: {
        if (!barRows || !selectContext || !srcData) {
            return [];
        }

        let rows = [];
        for (let row = 1; row < barRows.length; ++row) {
            rows.push(row);
        }
        return rows;
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

    function visibleFor(entry) {
        if (!entry || !srcData || !selectContext) {
            return false;
        }
        let ranking = selectContext.isRankingEntry(entry);
        if (!ranking && !selectContext.isChart(entry) && !selectContext.isEntry(entry)) {
            return false;
        }
        if (ranking) {
            return srcData.variant === 6;
        }
        if ((entry.keymode || 0) <= 0 || selectContext.entryPlayLevel(entry) <= 0) {
            return false;
        }
        let difficulty = selectContext.entryDifficulty(entry);
        if (difficulty <= 0) {
            return srcData.variant === 0;
        }
        return srcData.variant === difficulty;
    }

    Repeater {
        model: root.numberRows

        Lr2NumberRenderer {
            readonly property int row: modelData
            readonly property var entry: {
                let revision = root.contextRevision;
                return root.selectContext ? root.selectContext.visibleBarEntry(row, root.barCenter) : null;
            }
            readonly property var visibleBase: root.visibilityState(row)
            readonly property bool contentVisible: !!visibleBase && root.visibleFor(entry)
            visible: contentVisible
            dsts: root.dsts
            srcData: root.srcData ? root.srcData.source : null
            skinTime: root.skinTime
            activeOptions: root.activeOptions
            timers: root.timers
            scaleOverride: root.scaleOverride
            colorKeyEnabled: root.colorKeyEnabled
            transColor: root.transColor
            offsetX: root.drawOffset(row, 0)
            offsetY: root.drawOffset(row, 1)
            value: root.selectContext ? root.selectContext.entryPlayLevel(entry) : 0
        }
    }
}
