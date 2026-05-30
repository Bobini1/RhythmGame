#pragma once

#include "Lr2SelectNavigationController.h"

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class Lr2SelectKeyNavigationFilter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool selectScrollReady READ selectScrollReady WRITE setSelectScrollReady NOTIFY selectScrollReadyChanged)
    Q_PROPERTY(Lr2SelectNavigationController* navigationController READ navigationController WRITE setNavigationController NOTIFY navigationControllerChanged)

public:
    explicit Lr2SelectKeyNavigationFilter(QObject* parent = nullptr);
    ~Lr2SelectKeyNavigationFilter() override;

    QObject* target() const;
    void setTarget(QObject* target);

    bool selectScrollReady() const;
    void setSelectScrollReady(bool ready);

    Lr2SelectNavigationController* navigationController() const;
    void setNavigationController(Lr2SelectNavigationController* controller);

signals:
    void targetChanged();
    void selectScrollReadyChanged();
    void navigationControllerChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void uninstallTargetFilter();
    void installTargetFilter();

    QPointer<QObject> m_target;
    QPointer<Lr2SelectNavigationController> m_navigationController;
    bool m_selectScrollReady = false;
};
