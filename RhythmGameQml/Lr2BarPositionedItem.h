#pragma once

#include "Lr2BarPositionCache.h"

#include <QPointer>
#include <QQuickItem>
#include <QMetaObject>
#include <QtQml/qqmlregistration.h>

class Lr2BarPositionedItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2BarPositionCache* positionCache READ positionCache WRITE setPositionCache NOTIFY positionCacheChanged)
    Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged)
    Q_PROPERTY(qreal scaleOverride READ scaleOverride WRITE setScaleOverride NOTIFY scaleOverrideChanged)
    Q_PROPERTY(bool usePositionCache READ usePositionCache WRITE setUsePositionCache NOTIFY usePositionCacheChanged)
    Q_PROPERTY(bool hasOverride READ hasOverride WRITE setHasOverride NOTIFY hasOverrideChanged)
    Q_PROPERTY(qreal overrideX READ overrideX WRITE setOverrideX NOTIFY overrideXChanged)
    Q_PROPERTY(qreal overrideY READ overrideY WRITE setOverrideY NOTIFY overrideYChanged)
    Q_PROPERTY(qreal adjustX READ adjustX WRITE setAdjustX NOTIFY adjustXChanged)
    Q_PROPERTY(qreal adjustY READ adjustY WRITE setAdjustY NOTIFY adjustYChanged)
    Q_PROPERTY(qreal fallbackX READ fallbackX WRITE setFallbackX NOTIFY fallbackXChanged)
    Q_PROPERTY(qreal fallbackY READ fallbackY WRITE setFallbackY NOTIFY fallbackYChanged)

public:
    explicit Lr2BarPositionedItem(QQuickItem* parent = nullptr);

    Lr2BarPositionCache* positionCache() const;
    void setPositionCache(Lr2BarPositionCache* cache);

    int row() const;
    void setRow(int row);

    qreal scaleOverride() const;
    void setScaleOverride(qreal scale);

    bool usePositionCache() const;
    void setUsePositionCache(bool useCache);

    bool hasOverride() const;
    void setHasOverride(bool hasOverride);

    qreal overrideX() const;
    void setOverrideX(qreal value);

    qreal overrideY() const;
    void setOverrideY(qreal value);

    qreal adjustX() const;
    void setAdjustX(qreal value);

    qreal adjustY() const;
    void setAdjustY(qreal value);

    qreal fallbackX() const;
    void setFallbackX(qreal value);

    qreal fallbackY() const;
    void setFallbackY(qreal value);

signals:
    void positionCacheChanged();
    void rowChanged();
    void scaleOverrideChanged();
    void usePositionCacheChanged();
    void hasOverrideChanged();
    void overrideXChanged();
    void overrideYChanged();
    void adjustXChanged();
    void adjustYChanged();
    void fallbackXChanged();
    void fallbackYChanged();

private:
    static bool sameReal(qreal lhs, qreal rhs);
    void updatePosition();

    QPointer<Lr2BarPositionCache> m_positionCache;
    QMetaObject::Connection m_revisionConnection;
    int m_row = -1;
    qreal m_scaleOverride = 1.0;
    bool m_usePositionCache = true;
    bool m_hasOverride = false;
    qreal m_overrideX = 0.0;
    qreal m_overrideY = 0.0;
    qreal m_adjustX = 0.0;
    qreal m_adjustY = 0.0;
    qreal m_fallbackX = 0.0;
    qreal m_fallbackY = 0.0;
};
