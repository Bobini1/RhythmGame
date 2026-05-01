#include "Lr2BarPositionedItem.h"

#include <QtMath>

Lr2BarPositionedItem::Lr2BarPositionedItem(QQuickItem* parent) : QQuickItem(parent) {}

Lr2BarPositionCache* Lr2BarPositionedItem::positionCache() const {
    return m_positionCache;
}

void Lr2BarPositionedItem::setPositionCache(Lr2BarPositionCache* cache) {
    if (m_positionCache == cache) {
        return;
    }

    if (m_revisionConnection) {
        disconnect(m_revisionConnection);
    }
    if (m_slotOffsetConnection) {
        disconnect(m_slotOffsetConnection);
    }
    if (m_slotCountConnection) {
        disconnect(m_slotCountConnection);
    }

    m_positionCache = cache;
    if (m_positionCache) {
        m_revisionConnection = connect(
            m_positionCache,
            &Lr2BarPositionCache::revisionChanged,
            this,
            &Lr2BarPositionedItem::updatePosition);
        m_slotOffsetConnection = connect(
            m_positionCache,
            &Lr2BarPositionCache::slotOffsetChanged,
            this,
            [this]() {
                if (m_useSlotRow) {
                    updatePosition();
                }
            });
        m_slotCountConnection = connect(
            m_positionCache,
            &Lr2BarPositionCache::slotCountChanged,
            this,
            [this]() {
                if (m_useSlotRow) {
                    updatePosition();
                }
            });
    } else {
        m_revisionConnection = {};
        m_slotOffsetConnection = {};
        m_slotCountConnection = {};
    }

    emit positionCacheChanged();
    updatePosition();
}

int Lr2BarPositionedItem::row() const {
    return m_row;
}

void Lr2BarPositionedItem::setRow(int row) {
    if (m_row == row) {
        return;
    }
    m_row = row;
    updatePosition();
}

bool Lr2BarPositionedItem::useSlotRow() const {
    return m_useSlotRow;
}

void Lr2BarPositionedItem::setUseSlotRow(bool useSlotRow) {
    if (m_useSlotRow == useSlotRow) {
        return;
    }
    m_useSlotRow = useSlotRow;
    emit useSlotRowChanged();
    updatePosition();
}

int Lr2BarPositionedItem::slot() const {
    return m_slot;
}

void Lr2BarPositionedItem::setSlot(int slot) {
    if (m_slot == slot) {
        return;
    }
    m_slot = slot;
    emit slotChanged();
    updatePosition();
}

int Lr2BarPositionedItem::effectiveRow() const {
    return m_effectiveRow;
}

bool Lr2BarPositionedItem::rowVisible() const {
    return m_rowVisible;
}

qreal Lr2BarPositionedItem::scaleOverride() const {
    return m_scaleOverride;
}

void Lr2BarPositionedItem::setScaleOverride(qreal scale) {
    if (sameReal(m_scaleOverride, scale)) {
        return;
    }
    m_scaleOverride = scale;
    updatePosition();
}

bool Lr2BarPositionedItem::usePositionCache() const {
    return m_usePositionCache;
}

void Lr2BarPositionedItem::setUsePositionCache(bool useCache) {
    if (m_usePositionCache == useCache) {
        return;
    }
    m_usePositionCache = useCache;
    updatePosition();
}

bool Lr2BarPositionedItem::hasOverride() const {
    return m_hasOverride;
}

void Lr2BarPositionedItem::setHasOverride(bool hasOverride) {
    if (m_hasOverride == hasOverride) {
        return;
    }
    m_hasOverride = hasOverride;
    updatePosition();
}

qreal Lr2BarPositionedItem::overrideX() const {
    return m_overrideX;
}

void Lr2BarPositionedItem::setOverrideX(qreal value) {
    if (sameReal(m_overrideX, value)) {
        return;
    }
    m_overrideX = value;
    updatePosition();
}

qreal Lr2BarPositionedItem::overrideY() const {
    return m_overrideY;
}

void Lr2BarPositionedItem::setOverrideY(qreal value) {
    if (sameReal(m_overrideY, value)) {
        return;
    }
    m_overrideY = value;
    updatePosition();
}

qreal Lr2BarPositionedItem::adjustX() const {
    return m_adjustX;
}

void Lr2BarPositionedItem::setAdjustX(qreal value) {
    if (sameReal(m_adjustX, value)) {
        return;
    }
    m_adjustX = value;
    updatePosition();
}

qreal Lr2BarPositionedItem::adjustY() const {
    return m_adjustY;
}

void Lr2BarPositionedItem::setAdjustY(qreal value) {
    if (sameReal(m_adjustY, value)) {
        return;
    }
    m_adjustY = value;
    updatePosition();
}

qreal Lr2BarPositionedItem::fallbackX() const {
    return m_fallbackX;
}

void Lr2BarPositionedItem::setFallbackX(qreal value) {
    if (sameReal(m_fallbackX, value)) {
        return;
    }
    m_fallbackX = value;
    updatePosition();
}

qreal Lr2BarPositionedItem::fallbackY() const {
    return m_fallbackY;
}

void Lr2BarPositionedItem::setFallbackY(qreal value) {
    if (sameReal(m_fallbackY, value)) {
        return;
    }
    m_fallbackY = value;
    updatePosition();
}

bool Lr2BarPositionedItem::sameReal(qreal lhs, qreal rhs) {
    return qAbs(lhs - rhs) <= 0.000001;
}

int Lr2BarPositionedItem::resolvedRow() const {
    if (m_useSlotRow && m_positionCache) {
        return m_positionCache->rowForSlot(m_slot);
    }
    return m_row;
}

void Lr2BarPositionedItem::updatePosition() {
    const int row = resolvedRow();
    if (m_effectiveRow != row) {
        m_effectiveRow = row;
        emit effectiveRowChanged();
    }

    const bool rowVisible = row > 0
        && (!m_positionCache || row < m_positionCache->count());
    if (m_rowVisible != rowVisible) {
        m_rowVisible = rowVisible;
        emit rowVisibleChanged();
    }

    qreal nextX = m_fallbackX;
    qreal nextY = m_fallbackY;

    if (m_hasOverride) {
        nextX = m_overrideX;
        nextY = m_overrideY;
    } else if (m_usePositionCache && m_positionCache && row >= 0 && row < m_positionCache->count()) {
        nextX = m_positionCache->xAt(row);
        nextY = m_positionCache->yAt(row);
    }

    const qreal scaledX = (nextX - m_adjustX) * m_scaleOverride;
    const qreal scaledY = (nextY - m_adjustY) * m_scaleOverride;
    if (!sameReal(x(), scaledX)) {
        setX(scaledX);
    }
    if (!sameReal(y(), scaledY)) {
        setY(scaledY);
    }
}
