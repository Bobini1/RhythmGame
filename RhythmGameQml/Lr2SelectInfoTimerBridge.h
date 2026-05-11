#pragma once

#include "Lr2SkinClock.h"

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class Lr2SelectInfoTimerBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* selectContext READ selectContext WRITE setSelectContext NOTIFY selectContextChanged)
    Q_PROPERTY(Lr2SkinClock* clock READ clock WRITE setClock NOTIFY clockChanged)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)

public:
    explicit Lr2SelectInfoTimerBridge(QObject* parent = nullptr);

    QObject* selectContext() const;
    void setSelectContext(QObject* context);

    Lr2SkinClock* clock() const;
    void setClock(Lr2SkinClock* clock);

    bool isActive() const;
    void setActive(bool active);

signals:
    void selectContextChanged();
    void clockChanged();
    void activeChanged();

private slots:
    void restartFromFocusChange();

private:
    QPointer<QObject> m_selectContext;
    QPointer<Lr2SkinClock> m_clock;
    QMetaObject::Connection m_focusRevisionConnection;
    bool m_active = false;
};
