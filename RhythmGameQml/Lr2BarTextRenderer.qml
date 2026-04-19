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
    readonly property int contextRevision: selectContext ? selectContext.listRevision : 0
    readonly property int visualBaseIndex: selectContext ? selectContext.visualBaseIndex : 0

    readonly property var textRows: {
        if (!barRows || !selectContext || !srcData) {
            return [];
        }

        let rows = [];
        for (let row = 1; row < barRows.length; ++row) {
            rows.push(row);
        }
        return rows;
    }

    function visibleFor(entry) {
        return !!entry && !!srcData && !!selectContext
            && selectContext.entryTitleType(entry) === srcData.titleType;
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
        model: root.textRows

        Lr2TextRenderer {
            readonly property int row: modelData
            readonly property var entry: {
                let revision = root.contextRevision;
                let base = root.visualBaseIndex;
                return root.selectContext ? root.selectContext.barEntry(row, root.barCenter) : null;
            }
            readonly property var visibleBase: root.visibilityState(row)
            readonly property bool contentVisible: !!visibleBase && root.visibleFor(entry)
            visible: contentVisible
            dsts: root.dsts
            srcData: root.srcData
            skinTime: root.skinTime
            activeOptions: root.activeOptions
            timers: root.timers
            scaleOverride: root.scaleOverride
            offsetX: root.drawOffset(row, 0)
            offsetY: root.drawOffset(row, 1)
            resolvedText: root.selectContext ? root.selectContext.entryDisplayName(entry, true) : ""
        }
    }
}
