#pragma once

#include "Lr2SelectVisualState.h"

#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class Lr2BarPositionCache : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList baseStates READ baseStates WRITE setBaseStates NOTIFY baseStatesChanged)
    Q_PROPERTY(qreal scrollOffset READ scrollOffset WRITE setScrollOffset NOTIFY scrollOffsetChanged)
    Q_PROPERTY(int slotOffset READ slotOffset WRITE setSlotOffset NOTIFY slotOffsetChanged)
    Q_PROPERTY(int slotCount READ slotCount WRITE setSlotCount NOTIFY slotCountChanged)
    Q_PROPERTY(Lr2SelectVisualState* visualState READ visualState WRITE setVisualState NOTIFY visualStateChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(int count READ count NOTIFY revisionChanged)

public:
    explicit Lr2BarPositionCache(QObject* parent = nullptr);

    QVariantList baseStates() const;
    void setBaseStates(const QVariantList& states);

    qreal scrollOffset() const;
    void setScrollOffset(qreal offset);

    int slotOffset() const;
    void setSlotOffset(int offset);

    int slotCount() const;
    void setSlotCount(int count);

    Lr2SelectVisualState* visualState() const;
    void setVisualState(Lr2SelectVisualState* state);

    int revision() const;
    int count() const;

    Q_INVOKABLE qreal xAt(int row) const;
    Q_INVOKABLE qreal yAt(int row) const;
    Q_INVOKABLE int rowForSlot(int slot) const;

signals:
    void baseStatesChanged();
    void scrollOffsetChanged();
    void slotOffsetChanged();
    void slotCountChanged();
    void visualStateChanged();
    void revisionChanged();

private:
    void rebuildBaseCoordinates(const QVariantList& states);
    void rebuildDrawCoordinates();
    void bumpRevision();
    static bool extractCoordinates(const QVariant& state, qreal& x, qreal& y);

    QVector<qreal> m_baseXs;
    QVector<qreal> m_baseYs;
    QVector<qreal> m_drawXs;
    QVector<qreal> m_drawYs;
    QVector<bool> m_validRows;
    qreal m_scrollOffset = 0.0;
    int m_slotOffset = 0;
    int m_slotCount = 0;
    QPointer<Lr2SelectVisualState> m_visualState;
    QMetaObject::Connection m_visualStateOffsetConnection;
    int m_revision = 0;
};
