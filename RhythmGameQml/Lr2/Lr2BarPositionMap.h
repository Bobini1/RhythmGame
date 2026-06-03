#pragma once

#include "Lr2BarBaseStateResolver.h"
#include "Lr2SelectVisualState.h"

#include <QObject>
#include <QPointer>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class Lr2BarPositionMap : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2BarBaseStateResolver* baseStateResolver READ baseStateResolver WRITE setBaseStateResolver NOTIFY baseStateResolverChanged)
    Q_PROPERTY(qreal scrollOffset READ scrollOffset WRITE setScrollOffset NOTIFY scrollOffsetChanged)
    Q_PROPERTY(int slotOffset READ slotOffset WRITE setSlotOffset NOTIFY slotOffsetChanged)
    Q_PROPERTY(int slotCount READ slotCount WRITE setSlotCount NOTIFY slotCountChanged)
    Q_PROPERTY(Lr2SelectVisualState* visualState READ visualState WRITE setVisualState NOTIFY visualStateChanged)
    Q_PROPERTY(int count READ count NOTIFY coordinatesChanged)

public:
    explicit Lr2BarPositionMap(QObject* parent = nullptr);

    Lr2BarBaseStateResolver* baseStateResolver() const;
    void setBaseStateResolver(Lr2BarBaseStateResolver* resolver);

    qreal scrollOffset() const;
    void setScrollOffset(qreal offset);

    int slotOffset() const;
    void setSlotOffset(int offset);

    int slotCount() const;
    void setSlotCount(int count);

    Lr2SelectVisualState* visualState() const;
    void setVisualState(Lr2SelectVisualState* state);

    int count() const;

    Q_INVOKABLE qreal xAt(int row) const;
    Q_INVOKABLE qreal yAt(int row) const;
    Q_INVOKABLE int slotForRow(int row) const;
    Q_INVOKABLE int rowForSlot(int slot) const;

signals:
    void baseStateResolverChanged();
    void scrollOffsetChanged();
    void slotOffsetChanged();
    void slotCountChanged();
    void visualStateChanged();
    void coordinatesChanged();

private:
    void rebuildBaseCoordinatesFromResolver();
    void rebuildDrawCoordinates();
    void notifyCoordinatesChanged();

    QVector<qreal> m_baseXs;
    QVector<qreal> m_baseYs;
    QVector<qreal> m_drawXs;
    QVector<qreal> m_drawYs;
    QVector<bool> m_validRows;
    QPointer<Lr2BarBaseStateResolver> m_baseStateResolver;
    QMetaObject::Connection m_baseStateResolverConnection;
    qreal m_scrollOffset = 0.0;
    int m_slotOffset = 0;
    int m_slotCount = 0;
    QPointer<Lr2SelectVisualState> m_visualState;
    QMetaObject::Connection m_visualStateOffsetConnection;
};
