#pragma once

#include "Lr2BarBaseStateResolver.h"
#include "Lr2BarPositionMap.h"
#include "Lr2TimelineStateValue.h"

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class Lr2BarInterpolatedState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2BarBaseStateResolver* baseStateResolver READ baseStateResolver WRITE setBaseStateResolver NOTIFY baseStateResolverChanged)
    Q_PROPERTY(Lr2BarPositionMap* positionMap READ positionMap WRITE setPositionMap NOTIFY positionMapChanged)
    Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(qreal x READ x NOTIFY stateChanged)
    Q_PROPERTY(qreal y READ y NOTIFY stateChanged)
    Q_PROPERTY(qreal w READ w NOTIFY stateChanged)
    Q_PROPERTY(qreal h READ h NOTIFY stateChanged)
    Q_PROPERTY(qreal a READ a NOTIFY stateChanged)
    Q_PROPERTY(qreal r READ r NOTIFY stateChanged)
    Q_PROPERTY(qreal g READ g NOTIFY stateChanged)
    Q_PROPERTY(qreal b READ b NOTIFY stateChanged)
    Q_PROPERTY(qreal angle READ angle NOTIFY stateChanged)
    Q_PROPERTY(qreal center READ center NOTIFY stateChanged)
    Q_PROPERTY(qreal sortId READ sortId NOTIFY stateChanged)
    Q_PROPERTY(qreal blend READ blend NOTIFY stateChanged)
    Q_PROPERTY(qreal filter READ filter NOTIFY stateChanged)
    Q_PROPERTY(qreal op1 READ op1 NOTIFY stateChanged)
    Q_PROPERTY(qreal op2 READ op2 NOTIFY stateChanged)
    Q_PROPERTY(qreal op3 READ op3 NOTIFY stateChanged)
    Q_PROPERTY(qreal op4 READ op4 NOTIFY stateChanged)

public:
    explicit Lr2BarInterpolatedState(QObject* parent = nullptr);

    Lr2BarBaseStateResolver* baseStateResolver() const;
    void setBaseStateResolver(Lr2BarBaseStateResolver* resolver);

    Lr2BarPositionMap* positionMap() const;
    void setPositionMap(Lr2BarPositionMap* map);

    int row() const;
    void setRow(int row);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    qreal x() const;
    qreal y() const;
    qreal w() const;
    qreal h() const;
    qreal a() const;
    qreal r() const;
    qreal g() const;
    qreal b() const;
    qreal angle() const;
    qreal center() const;
    qreal sortId() const;
    qreal blend() const;
    qreal filter() const;
    qreal op1() const;
    qreal op2() const;
    qreal op3() const;
    qreal op4() const;

signals:
    void baseStateResolverChanged();
    void positionMapChanged();
    void rowChanged();
    void enabledChanged();
    void stateChanged();

private:
    using State = Lr2TimelineStateValue;

    static bool sameReal(qreal lhs, qreal rhs);
    static State hiddenState();
    static State visibleState(const State& state);
    static State interpolate(const State& from, const State& to, qreal progress);

    void updateState();
    bool assignState(const State& state);

    QPointer<Lr2BarPositionMap> m_positionMap;
    QMetaObject::Connection m_coordinatesConnection;
    QPointer<Lr2BarBaseStateResolver> m_baseStateResolver;
    QMetaObject::Connection m_baseStateResolverConnection;
    State m_state;
    int m_row = -1;
    bool m_enabled = true;
};
