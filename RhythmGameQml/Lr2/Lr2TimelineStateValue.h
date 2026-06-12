#pragma once

#include <QMetaType>
#include <QVariant>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>

class Lr2TimelineStateValue {
    Q_GADGET
    QML_VALUE_TYPE(lr2TimelineStateValue)
    Q_PROPERTY(bool valid MEMBER valid)
    Q_PROPERTY(qreal x MEMBER x)
    Q_PROPERTY(qreal y MEMBER y)
    Q_PROPERTY(qreal w MEMBER w)
    Q_PROPERTY(qreal h MEMBER h)
    Q_PROPERTY(qreal a MEMBER a)
    Q_PROPERTY(qreal r MEMBER r)
    Q_PROPERTY(qreal g MEMBER g)
    Q_PROPERTY(qreal b MEMBER b)
    Q_PROPERTY(qreal angle MEMBER angle)
    Q_PROPERTY(int center MEMBER center)
    Q_PROPERTY(qreal sortId MEMBER sortId)
    Q_PROPERTY(int blend MEMBER blend)
    Q_PROPERTY(int filter MEMBER filter)
    Q_PROPERTY(int op1 MEMBER op1)
    Q_PROPERTY(int op2 MEMBER op2)
    Q_PROPERTY(int op3 MEMBER op3)
    Q_PROPERTY(int op4 MEMBER op4)

public:
    bool valid = false;
    qreal x = 0.0;
    qreal y = 0.0;
    qreal w = 0.0;
    qreal h = 0.0;
    qreal a = 255.0;
    qreal r = 255.0;
    qreal g = 255.0;
    qreal b = 255.0;
    qreal angle = 0.0;
    int center = 0;
    qreal sortId = 0.0;
    int blend = 0;
    int filter = 0;
    int op1 = 0;
    int op2 = 0;
    int op3 = 0;
    int op4 = 0;

    operator QVariant() const { return QVariant::fromValue(*this); }
};

Q_DECLARE_METATYPE(Lr2TimelineStateValue)
