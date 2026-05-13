#pragma once

#include "Lr2SelectNavigationController.h"

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class Lr2SelectKeyNavigationFilter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QObject* screenState READ screenState WRITE setScreenState NOTIFY screenStateChanged)
    Q_PROPERTY(Lr2SelectNavigationController* navigationController READ navigationController WRITE setNavigationController NOTIFY navigationControllerChanged)

public:
    explicit Lr2SelectKeyNavigationFilter(QObject* parent = nullptr);
    ~Lr2SelectKeyNavigationFilter() override;

    QObject* target() const;
    void setTarget(QObject* target);

    QObject* screenState() const;
    void setScreenState(QObject* state);

    Lr2SelectNavigationController* navigationController() const;
    void setNavigationController(Lr2SelectNavigationController* controller);

signals:
    void targetChanged();
    void screenStateChanged();
    void navigationControllerChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool selectScrollReady() const;
    void uninstallTargetFilter();
    void installTargetFilter();

    QPointer<QObject> m_target;
    QPointer<QObject> m_screenState;
    QPointer<Lr2SelectNavigationController> m_navigationController;
};
