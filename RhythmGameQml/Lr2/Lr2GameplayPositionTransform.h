#pragma once

#include <QMetaObject>
#include <QPointer>
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

class Lr2GameplayFrameState;

class Lr2GameplayPositionTransform : public QQuickTransform
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2GameplayFrameState* frameState READ frameState WRITE
                 setFrameState NOTIFY frameStateChanged)
    Q_PROPERTY(QObject* player READ player WRITE setPlayer NOTIFY playerChanged)
    Q_PROPERTY(int side READ side WRITE setSide NOTIFY sideChanged)
    Q_PROPERTY(qreal multiplier READ multiplier WRITE setMultiplier NOTIFY
                 multiplierChanged)

  public:
    explicit Lr2GameplayPositionTransform(QObject* parent = nullptr);

    Lr2GameplayFrameState* frameState() const;
    void setFrameState(Lr2GameplayFrameState* state);

    QObject* player() const;
    void setPlayer(QObject* player);

    int side() const;
    void setSide(int side);

    qreal multiplier() const;
    void setMultiplier(qreal multiplier);

    void applyTo(QMatrix4x4* matrix) const override;

  signals:
    void frameStateChanged();
    void playerChanged();
    void sideChanged();
    void multiplierChanged();

  private:
    void reconnectPositionSignal();

    QPointer<Lr2GameplayFrameState> m_frameState;
    QPointer<QObject> m_player;
    QMetaObject::Connection m_positionConnection;
    int m_side = 1;
    qreal m_multiplier = 0.0;
};
