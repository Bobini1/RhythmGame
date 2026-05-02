pragma ValueTypeBehavior: Addressable

import QtQml
import RhythmGameQml 1.0

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

    function createCell() {
        return Qt.createQmlObject("import RhythmGameQml 1.0\nLr2SelectBarCell {}", root);
    }

    function ensureCellList(count) {
        let current = cells || [];
        let next = current.length === count ? current : current.slice(0, count);
        let changed = next !== current;

        for (let i = 0; i < count; ++i) {
            if (!next[i]) {
                if (next === current) {
                    next = current.slice();
                }
                next[i] = createCell();
                changed = true;
            }
        }

        for (let old = count; old < current.length; ++old) {
            if (current[old] && current[old].destroy) {
                current[old].destroy();
            }
        }

        if (changed) {
            cells = next;
        }
        return next;
    }

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
        for (let i = 0; i < cells.length; ++i) {
            if (cells[i] && cells[i].destroy) {
                cells[i].destroy();
            }
        }
        cells = [];
        slotOffset = 0;
        cachedBaseIndex = -1;
        cachedListRevision = listRevision;
        cachedRowCount = rowCount;
        cachedCenter = barCenter;
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
            if (delta !== 0 && Math.abs(delta) < effectiveRowCount) {
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

            let entry = itemForRow(row);
            entries[slot] = entry;
            let cell = cells[slot];
            if (!cell) {
                cell = createCell();
                cells[slot] = cell;
            }
            context.updateBarTextCell(cell, row, entry);
        }

        slotOffset = nextOffset;
        cachedBaseIndex = visualBaseIndex;
        cachedListRevision = listRevision;
        cachedRowCount = effectiveRowCount;
        cachedCenter = barCenter;
    }

    function rebuild(effectiveRowCount) {
        let nextEntries = [];
        let nextCells = ensureCellList(effectiveRowCount);
        for (let row = 0; row < effectiveRowCount; ++row) {
            let entry = itemForRow(row);
            nextEntries.push(entry);
            context.updateBarTextCell(nextCells[row], row, entry);
        }

        entries = nextEntries;
        slotOffset = 0;
        cachedBaseIndex = visualBaseIndex;
        cachedListRevision = listRevision;
        cachedRowCount = effectiveRowCount;
        cachedCenter = barCenter;
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
