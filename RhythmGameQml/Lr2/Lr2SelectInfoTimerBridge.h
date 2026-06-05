#pragma once

#include "Lr2SelectNavigationController.h"
#include "Lr2SkinClock.h"

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class Lr2SelectInfoTimerBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2SelectNavigationController* navigationController READ navigationController WRITE setNavigationController NOTIFY navigationControllerChanged)
    Q_PROPERTY(Lr2SkinClock* clock READ clock WRITE setClock NOTIFY clockChanged)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)

public:
    explicit Lr2SelectInfoTimerBridge(QObject* parent = nullptr);

    Lr2SelectNavigationController* navigationController() const;
    void setNavigationController(Lr2SelectNavigationController* controller);

    Lr2SkinClock* clock() const;
    void setClock(Lr2SkinClock* clock);

    bool isActive() const;
    void setActive(bool active);

signals:
    void navigationControllerChanged();
    void clockChanged();
    void activeChanged();

private slots:
    void restartFromFocusChange();

private:
    QPointer<Lr2SelectNavigationController> m_navigationController;
    QPointer<Lr2SkinClock> m_clock;
    QMetaObject::Connection m_focusedStateConnection;
    bool m_active = false;
};
