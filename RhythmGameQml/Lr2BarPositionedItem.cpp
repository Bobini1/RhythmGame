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

    m_positionCache = cache;
    if (m_positionCache) {
        m_revisionConnection = connect(
            m_positionCache,
            &Lr2BarPositionCache::revisionChanged,
            this,
            &Lr2BarPositionedItem::updatePosition);
    } else {
        m_revisionConnection = {};
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
    emit rowChanged();
    updatePosition();
}

qreal Lr2BarPositionedItem::scaleOverride() const {
    return m_scaleOverride;
}

void Lr2BarPositionedItem::setScaleOverride(qreal scale) {
    if (sameReal(m_scaleOverride, scale)) {
        return;
    }
    m_scaleOverride = scale;
    emit scaleOverrideChanged();
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
    emit usePositionCacheChanged();
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
    emit hasOverrideChanged();
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
    emit overrideXChanged();
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
    emit overrideYChanged();
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
    emit adjustXChanged();
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
    emit adjustYChanged();
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
    emit fallbackXChanged();
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
    emit fallbackYChanged();
    updatePosition();
}

bool Lr2BarPositionedItem::sameReal(qreal lhs, qreal rhs) {
    return qAbs(lhs - rhs) <= 0.000001;
}

void Lr2BarPositionedItem::updatePosition() {
    qreal nextX = m_fallbackX;
    qreal nextY = m_fallbackY;

    if (m_hasOverride) {
        nextX = m_overrideX;
        nextY = m_overrideY;
    } else if (m_usePositionCache && m_positionCache && m_row >= 0 && m_row < m_positionCache->count()) {
        nextX = m_positionCache->xAt(m_row);
        nextY = m_positionCache->yAt(m_row);
    }

    setX((nextX - m_adjustX) * m_scaleOverride);
    setY((nextY - m_adjustY) * m_scaleOverride);
}
