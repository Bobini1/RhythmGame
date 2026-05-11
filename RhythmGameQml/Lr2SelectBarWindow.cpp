#include "Lr2SelectBarWindow.h"

#include "Lr2SelectBarCell.h"
#include "Lr2SelectStateCache.h"
#include "Lr2SelectVisualState.h"

#include <QJSValue>
#include <QQmlEngine>

#include <algorithm>

namespace {

QVariantList toList(const QVariant& value) {
    if (!value.isValid() || value.isNull()) {
        return {};
    }
    if (value.canConvert<QVariantList>()) {
        return value.toList();
    }
    if (value.canConvert<QStringList>()) {
        const QStringList strings = value.toStringList();
        QVariantList result;
        result.reserve(strings.size());
        for (const QString& string : strings) {
            result.append(string);
        }
        return result;
    }
    if (value.canConvert<QJSValue>()) {
        return value.value<QJSValue>().toVariant().toList();
    }
    return {};
}

int positiveModulo(int value, int modulus) {
    if (modulus <= 0) {
        return 0;
    }
    const int remainder = value % modulus;
    return remainder < 0 ? remainder + modulus : remainder;
}

} // namespace

Lr2SelectBarWindow::Lr2SelectBarWindow(QObject* parent) : QObject(parent) {}

QObject* Lr2SelectBarWindow::context() const { return m_context; }
void Lr2SelectBarWindow::setContext(QObject* value) {
    if (m_context == value) {
        return;
    }
    m_context = value;
    emit contextChanged();
}

QObject* Lr2SelectBarWindow::nativeState() const { return m_nativeState; }
void Lr2SelectBarWindow::setNativeState(QObject* value) {
    auto* cache = qobject_cast<Lr2SelectStateCache*>(value);
    if (m_nativeState == cache) {
        return;
    }
    for (const QMetaObject::Connection& connection : m_nativeStateConnections) {
        disconnect(connection);
    }
    m_nativeStateConnections.clear();

    m_nativeState = cache;
    if (m_nativeState) {
        m_nativeStateConnections.append(connect(
            m_nativeState,
            &Lr2SelectStateCache::folderLampByKeyChanged,
            this,
            &Lr2SelectBarWindow::refreshCellsInPlace));
        m_nativeStateConnections.append(connect(
            m_nativeState,
            &Lr2SelectStateCache::folderDistributionByKeyChanged,
            this,
            &Lr2SelectBarWindow::refreshCellsInPlace));
    }

    invalidate();
    refreshIfActive(true);
    emit nativeStateChanged();
}

QVariant Lr2SelectBarWindow::items() const { return m_items; }
void Lr2SelectBarWindow::setItems(const QVariant& value) {
    m_items = value;
    m_itemList = toList(value);
    invalidate();
    refreshIfActive(true);
    emit itemsChanged();
}

int Lr2SelectBarWindow::logicalCount() const { return m_logicalCount; }
void Lr2SelectBarWindow::setLogicalCount(int value) {
    const int next = std::max(0, value);
    if (m_logicalCount == next) {
        return;
    }
    const bool wasCurrent = isCurrent();
    m_logicalCount = next;
    emit logicalCountChanged();
    maybeEmitIsCurrentChanged(wasCurrent);
    refreshIfActive(true);
}

int Lr2SelectBarWindow::listRevision() const { return m_listRevision; }
void Lr2SelectBarWindow::setListRevision(int value) {
    if (m_listRevision == value) {
        return;
    }
    const bool wasCurrent = isCurrent();
    m_listRevision = value;
    emit listRevisionChanged();
    maybeEmitIsCurrentChanged(wasCurrent);
    refreshIfActive(true);
}

int Lr2SelectBarWindow::rowCount() const { return m_rowCount; }
void Lr2SelectBarWindow::setRowCount(int value) {
    const int next = std::max(0, value);
    if (m_rowCount == next) {
        return;
    }
    const bool wasCurrent = isCurrent();
    m_rowCount = next;
    emit rowCountChanged();
    maybeEmitIsCurrentChanged(wasCurrent);
    refreshIfActive(true);
}

int Lr2SelectBarWindow::barCenter() const { return m_barCenter; }
void Lr2SelectBarWindow::setBarCenter(int value) {
    if (m_barCenter == value) {
        return;
    }
    const bool wasCurrent = isCurrent();
    m_barCenter = value;
    emit barCenterChanged();
    maybeEmitIsCurrentChanged(wasCurrent);
    refreshIfActive(true);
}

Lr2SelectVisualState* Lr2SelectBarWindow::visualState() const { return m_visualState; }
void Lr2SelectBarWindow::setVisualState(Lr2SelectVisualState* value) {
    if (m_visualState == value) {
        return;
    }
    if (m_visualStateBaseIndexConnection) {
        disconnect(m_visualStateBaseIndexConnection);
        m_visualStateBaseIndexConnection = {};
    }

    m_visualState = value;
    if (m_visualState) {
        m_visualStateBaseIndexConnection = connect(
            m_visualState,
            &Lr2SelectVisualState::baseIndexChanged,
            this,
            &Lr2SelectBarWindow::updateVisualBaseIndexFromState);
        updateVisualBaseIndexFromState();
    }
    emit visualStateChanged();
}

int Lr2SelectBarWindow::visualBaseIndex() const { return m_visualBaseIndex; }
void Lr2SelectBarWindow::setVisualBaseIndex(int value) {
    if (m_visualBaseIndex == value) {
        return;
    }
    const bool wasCurrent = isCurrent();
    m_visualBaseIndex = value;
    emit visualBaseIndexChanged();
    maybeEmitIsCurrentChanged(wasCurrent);
    refreshIfActive(false);
}

QVariant Lr2SelectBarWindow::barTitleTypes() const { return m_barTitleTypes; }
void Lr2SelectBarWindow::setBarTitleTypes(const QVariant& value) {
    m_barTitleTypes = value;
    invalidate();
    refreshIfActive(true);
    emit barTitleTypesChanged();
}

int Lr2SelectBarWindow::scoreRevision() const { return m_scoreRevision; }
void Lr2SelectBarWindow::setScoreRevision(int value) {
    if (m_scoreRevision == value) {
        return;
    }
    m_scoreRevision = value;
    invalidate();
    refreshIfActive(true);
    emit scoreRevisionChanged();
}

int Lr2SelectBarWindow::folderLampRevision() const { return m_folderLampRevision; }
void Lr2SelectBarWindow::setFolderLampRevision(int value) {
    if (m_folderLampRevision == value) {
        return;
    }
    m_folderLampRevision = value;
    emit folderLampRevisionChanged();
    refreshCellsInPlace();
}

bool Lr2SelectBarWindow::isActive() const { return m_active; }
void Lr2SelectBarWindow::setActive(bool value) {
    if (m_active == value) {
        return;
    }
    m_active = value;
    emit activeChanged();
    if (m_active) {
        refresh(true);
    }
}

int Lr2SelectBarWindow::slotOffset() const { return m_slotOffset; }
QVariantList Lr2SelectBarWindow::entries() const { return m_entries; }
QVariantList Lr2SelectBarWindow::cells() const { return m_cellValues; }

bool Lr2SelectBarWindow::isCurrent() const {
    return m_cachedBaseIndex == m_visualBaseIndex
        && m_cachedListRevision == m_listRevision
        && m_cachedRowCount == std::max(0, m_rowCount)
        && m_cachedCenter == m_barCenter;
}

void Lr2SelectBarWindow::invalidate() {
    const bool wasCurrent = isCurrent();
    m_cachedBaseIndex = -1;
    maybeEmitIsCurrentChanged(wasCurrent);
}

void Lr2SelectBarWindow::refreshIfActive(bool force) {
    if (m_active) {
        refresh(force);
    }
}

void Lr2SelectBarWindow::refresh(bool force) {
    if (!m_active) {
        return;
    }

    const int effectiveRowCount = std::max(0, m_rowCount);
    if (m_logicalCount == 0 || effectiveRowCount == 0) {
        clearWindow(force);
        return;
    }

    if (!force
            && m_cachedBaseIndex >= 0
            && m_cachedListRevision == m_listRevision
            && m_cachedRowCount == effectiveRowCount
            && m_cachedCenter == m_barCenter
            && m_entries.size() == effectiveRowCount
            && m_cellObjects.size() == effectiveRowCount) {
        const int forward = positiveModulo(m_visualBaseIndex - m_cachedBaseIndex, m_logicalCount);
        const int backward = positiveModulo(m_cachedBaseIndex - m_visualBaseIndex, m_logicalCount);
        const int delta = forward <= backward ? forward : -backward;
        if (delta != 0 && std::abs(delta) < effectiveRowCount) {
            shiftWindow(delta, effectiveRowCount);
            return;
        }
    }

    if (!force && isCurrent()) {
        return;
    }

    rebuild(effectiveRowCount);
}

int Lr2SelectBarWindow::slotForRow(int row) const {
    const int effectiveRowCount = m_cellObjects.size();
    if (effectiveRowCount <= 0 || row < 0 || row >= effectiveRowCount) {
        return -1;
    }
    return positiveModulo(row + m_slotOffset, effectiveRowCount);
}

QVariant Lr2SelectBarWindow::entryAtRow(int row, int fallbackBarCenter) const {
    const int slot = slotForRow(row);
    if (slot >= 0 && slot < m_entries.size()) {
        return m_entries.at(slot);
    }
    return itemForRow(row, fallbackBarCenter);
}

QObject* Lr2SelectBarWindow::cellAtSlot(int slot) const {
    if (slot >= 0 && slot < m_cellObjects.size()) {
        return m_cellObjects.at(slot);
    }
    return nullptr;
}

QVariant Lr2SelectBarWindow::itemForRow(int row, int center) const {
    if (m_logicalCount <= 0 || m_itemList.isEmpty()) {
        return {};
    }
    const int index = positiveModulo(m_visualBaseIndex + row - center, m_logicalCount);
    return index >= 0 && index < m_itemList.size() ? m_itemList.at(index) : QVariant();
}

void Lr2SelectBarWindow::clearWindow(bool force) {
    if (!force && m_entries.isEmpty() && m_cellObjects.isEmpty()) {
        return;
    }

    const bool wasCurrent = isCurrent();
    qDeleteAll(m_cellObjects);
    m_cellObjects.clear();
    m_cellValues.clear();
    m_entries.clear();
    if (m_slotOffset != 0) {
        m_slotOffset = 0;
        emit slotOffsetChanged();
    }
    setCachedState(-1, m_listRevision, m_rowCount, m_barCenter);
    emit cellsChanged();
    emit entriesChanged();
    maybeEmitIsCurrentChanged(wasCurrent);
}

void Lr2SelectBarWindow::shiftWindow(int delta, int effectiveRowCount) {
    const int nextOffset = positiveModulo(m_slotOffset + delta, effectiveRowCount);
    const int steps = std::abs(delta);
    bool cellListChanged = false;

    for (int step = 0; step < steps; ++step) {
        const int row = delta > 0 ? effectiveRowCount - steps + step : step;
        const int slot = positiveModulo(row + nextOffset, effectiveRowCount);
        const QVariant entry = itemForRow(row, m_barCenter);
        m_entries[slot] = entry;
        const int previousCellCount = m_cellObjects.size();
        Lr2SelectBarCell* cell = ensureCellAtSlot(slot);
        cellListChanged = cellListChanged || m_cellObjects.size() != previousCellCount;
        updateCell(cell, row, entry);
    }

    const bool slotChanged = m_slotOffset != nextOffset;
    const bool wasCurrent = isCurrent();
    m_slotOffset = nextOffset;
    setCachedState(m_visualBaseIndex, m_listRevision, effectiveRowCount, m_barCenter);
    if (slotChanged) {
        emit slotOffsetChanged();
    }
    if (cellListChanged) {
        emit cellsChanged();
    }
    maybeEmitIsCurrentChanged(wasCurrent);
}

void Lr2SelectBarWindow::rebuild(int effectiveRowCount) {
    const bool wasCurrent = isCurrent();
    ensureCellList(effectiveRowCount);

    QVariantList nextEntries;
    nextEntries.reserve(effectiveRowCount);
    for (int row = 0; row < effectiveRowCount; ++row) {
        const QVariant entry = itemForRow(row, m_barCenter);
        nextEntries.append(entry);
        updateCell(m_cellObjects.at(row), row, entry);
    }

    m_entries = nextEntries;
    const bool slotChanged = m_slotOffset != 0;
    m_slotOffset = 0;
    setCachedState(m_visualBaseIndex, m_listRevision, effectiveRowCount, m_barCenter);
    emit entriesChanged();
    emit cellsChanged();
    if (slotChanged) {
        emit slotOffsetChanged();
    }
    maybeEmitIsCurrentChanged(wasCurrent);
}

void Lr2SelectBarWindow::ensureCellList(int count) {
    while (m_cellObjects.size() > count) {
        delete m_cellObjects.takeLast();
        m_cellValues.removeLast();
    }

    while (m_cellObjects.size() < count) {
        auto* cell = new Lr2SelectBarCell(this);
        QQmlEngine::setObjectOwnership(cell, QQmlEngine::CppOwnership);
        m_cellObjects.append(cell);
        m_cellValues.append(QVariant::fromValue(static_cast<QObject*>(cell)));
    }
}

Lr2SelectBarCell* Lr2SelectBarWindow::ensureCellAtSlot(int slot) {
    if (slot < 0) {
        return nullptr;
    }
    while (m_cellObjects.size() <= slot) {
        auto* cell = new Lr2SelectBarCell(this);
        QQmlEngine::setObjectOwnership(cell, QQmlEngine::CppOwnership);
        m_cellObjects.append(cell);
        m_cellValues.append(QVariant::fromValue(static_cast<QObject*>(cell)));
    }
    return m_cellObjects.at(slot);
}

void Lr2SelectBarWindow::updateCell(Lr2SelectBarCell* cell, int row, const QVariant& entry) {
    if (!cell) {
        return;
    }
    if (m_nativeState) {
        m_nativeState->updateBarCell(cell, row, entry, m_barTitleTypes, m_scoreRevision, m_listRevision, m_folderLampRevision);
        return;
    }

    cell->setCore(row, entry.isValid() && !entry.isNull(), {}, 0, 1, 0, 0, 0,
                  false, false, false, false, 0, 0, 0, {}, {});
}

void Lr2SelectBarWindow::refreshCellsInPlace() {
    if (!m_active || m_cellObjects.isEmpty()) {
        return;
    }

    for (int slot = 0; slot < m_cellObjects.size(); ++slot) {
        const int row = positiveModulo(slot - m_slotOffset, m_cellObjects.size());
        const QVariant entry = slot >= 0 && slot < m_entries.size()
            ? m_entries.at(slot)
            : itemForRow(row, m_barCenter);
        updateCell(m_cellObjects.at(slot), row, entry);
    }
}

void Lr2SelectBarWindow::setCachedState(int baseIndex, int listRevision, int rowCount, int center) {
    m_cachedBaseIndex = baseIndex;
    m_cachedListRevision = listRevision;
    m_cachedRowCount = rowCount;
    m_cachedCenter = center;
}

void Lr2SelectBarWindow::maybeEmitIsCurrentChanged(bool previous) {
    if (previous != isCurrent()) {
        emit isCurrentChanged();
    }
}

void Lr2SelectBarWindow::updateVisualBaseIndexFromState() {
    setVisualBaseIndex(m_visualState ? m_visualState->baseIndex() : 0);
}
