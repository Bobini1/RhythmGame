pragma ValueTypeBehavior: Addressable

import QtQml

QtObject {
    id: root

    required property var context
    property var items: []
    property int logicalCount: 0
    property int listRevision: 0
    property int rowCount: 0
    property int barCenter: 0
    property int visualBaseIndex: 0

    property int slotOffset: 0
    property var entries: []
    property var cells: []

    property int cachedBaseIndex: -1
    property int cachedListRevision: -1
    property int cachedRowCount: -1
    property int cachedCenter: -1

    readonly property bool isCurrent: cachedBaseIndex === visualBaseIndex
        && cachedListRevision === listRevision
        && cachedRowCount === rowCount
        && cachedCenter === barCenter

    signal cellsInvalidated()
    signal cellRangeChanged(int firstSlot, int count)

    function invalidate() {
        cachedBaseIndex = -1;
    }

    function itemForRow(row) {
        if (logicalCount <= 0) {
            return null;
        }
        return items[context.normalizeIndex(visualBaseIndex + row - barCenter)];
    }

    function clearWindow(force) {
        if (!force && entries.length === 0 && cells.length === 0) {
            return;
        }
        entries = [];
        cells = [];
        slotOffset = 0;
        cachedBaseIndex = -1;
        cachedListRevision = listRevision;
        cachedRowCount = rowCount;
        cachedCenter = barCenter;
        cellsInvalidated();
    }

    function refresh(force) {
        let effectiveRowCount = Math.max(0, rowCount || 0);
        if (logicalCount === 0 || effectiveRowCount === 0) {
            clearWindow(!!force);
            return;
        }

        if (!force
                && cachedBaseIndex >= 0
                && cachedListRevision === listRevision
                && cachedRowCount === effectiveRowCount
                && cachedCenter === barCenter
                && entries.length === effectiveRowCount
                && cells.length === effectiveRowCount) {
            let forward = (visualBaseIndex - cachedBaseIndex + logicalCount) % logicalCount;
            let backward = (cachedBaseIndex - visualBaseIndex + logicalCount) % logicalCount;
            let delta = forward <= backward ? forward : -backward;
            if (delta !== 0 && Math.abs(delta) <= Math.min(effectiveRowCount, 4)) {
                shiftWindow(delta, effectiveRowCount);
                return;
            }
        }

        if (!force
                && cachedBaseIndex === visualBaseIndex
                && cachedListRevision === listRevision
                && cachedRowCount === effectiveRowCount
                && cachedCenter === barCenter) {
            return;
        }

        rebuild(effectiveRowCount);
    }

    function shiftWindow(delta, effectiveRowCount) {
        let nextOffset = ((slotOffset + delta) % effectiveRowCount + effectiveRowCount) % effectiveRowCount;
        let steps = Math.abs(delta);
        let firstChangedSlot = -1;

        for (let step = 0; step < steps; ++step) {
            let row = delta > 0 ? effectiveRowCount - steps + step : step;
            let slot = (row + nextOffset) % effectiveRowCount;
            if (firstChangedSlot < 0) {
                firstChangedSlot = slot;
            }

            entries[slot] = itemForRow(row);
            let cell = cells[slot];
            if (cell) {
                context.updateBarTextCellForRow(cell, row);
            } else {
                cells[slot] = context.barTextCellForRow(row);
            }
        }

        slotOffset = nextOffset;
        cachedBaseIndex = visualBaseIndex;
        cachedListRevision = listRevision;
        cachedRowCount = effectiveRowCount;
        cachedCenter = barCenter;
        cellRangeChanged(firstChangedSlot, steps);
    }

    function rebuild(effectiveRowCount) {
        let nextEntries = [];
        let nextCells = [];
        for (let row = 0; row < effectiveRowCount; ++row) {
            nextEntries.push(itemForRow(row));
            nextCells.push(context.barTextCellForRow(row));
        }

        entries = nextEntries;
        cells = nextCells;
        slotOffset = 0;
        cachedBaseIndex = visualBaseIndex;
        cachedListRevision = listRevision;
        cachedRowCount = effectiveRowCount;
        cachedCenter = barCenter;
        cellsInvalidated();
    }

    function slotForRow(row) {
        let effectiveRowCount = cells.length;
        if (effectiveRowCount <= 0 || row < 0 || row >= effectiveRowCount) {
            return -1;
        }
        return (row + slotOffset) % effectiveRowCount;
    }

    function entryAtRow(row, fallbackBarCenter) {
        let slot = slotForRow(row);
        if (slot >= 0 && slot < entries.length) {
            return entries[slot];
        }
        return context.barEntry(row, fallbackBarCenter);
    }

    function cellAtSlot(slot) {
        if (slot >= 0 && slot < cells.length) {
            return cells[slot];
        }
        return context.barTextCellForRow(slot);
    }
}
