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
    property int barCenter: 0
    readonly property int contextRevision: selectContext ? selectContext.revision : 0
    readonly property int visualBaseIndex: selectContext ? selectContext.visualBaseIndex : 0
    readonly property real scrollOffset: selectContext ? selectContext.scrollOffset : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter

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
        let fromState = rawBaseState(row, row === selectedRow);
        if (scrollOffset <= 0.001 || !fromState) {
            return fromState;
        }
        return interpolateState(fromState,
                                rawBaseState(row - 1, row - 1 === selectedRow),
                                scrollOffset);
    }

    function rawBaseState(row, useOn) {
        if (!barRows || row < 0 || row >= barRows.length) {
            return null;
        }
        let data = barRows[row];
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

    Repeater {
        model: root.textRows

        Lr2TextRenderer {
            readonly property int row: modelData
            readonly property var entry: {
                let revision = root.contextRevision;
                let base = root.visualBaseIndex;
                return root.selectContext ? root.selectContext.barEntry(row, root.barCenter) : null;
            }
            readonly property var base: root.baseState(row)
            visible: !!base && root.visibleFor(entry)
            dsts: root.dsts
            srcData: root.srcData
            skinTime: root.skinTime
            activeOptions: root.activeOptions
            timers: root.timers
            scaleOverride: root.scaleOverride
            offsetX: base ? base.x : 0
            offsetY: base ? base.y : 0
            resolvedText: root.selectContext ? root.selectContext.entryDisplayName(entry, true) : ""
        }
    }
}
