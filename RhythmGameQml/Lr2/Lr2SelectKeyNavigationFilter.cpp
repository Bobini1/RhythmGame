#include "Lr2SelectKeyNavigationFilter.h"

#include <QEvent>
#include <QKeyEvent>

Lr2SelectKeyNavigationFilter::Lr2SelectKeyNavigationFilter(QObject* parent)
    : QObject(parent) {}

Lr2SelectKeyNavigationFilter::~Lr2SelectKeyNavigationFilter() {
    uninstallTargetFilter();
}

QObject* Lr2SelectKeyNavigationFilter::target() const {
    return m_target;
}

void Lr2SelectKeyNavigationFilter::setTarget(QObject* target) {
    if (m_target == target) {
        return;
    }

    uninstallTargetFilter();
    m_target = target;
    installTargetFilter();
    emit targetChanged();
}

bool Lr2SelectKeyNavigationFilter::selectScrollReady() const {
    return m_selectScrollReady;
}

void Lr2SelectKeyNavigationFilter::setSelectScrollReady(bool ready) {
    if (m_selectScrollReady == ready) {
        return;
    }

    m_selectScrollReady = ready;
    emit selectScrollReadyChanged();
}

Lr2SelectNavigationController* Lr2SelectKeyNavigationFilter::navigationController() const {
    return m_navigationController;
}

void Lr2SelectKeyNavigationFilter::setNavigationController(Lr2SelectNavigationController* controller) {
    if (m_navigationController == controller) {
        return;
    }

    m_navigationController = controller;
    emit navigationControllerChanged();
}

bool Lr2SelectKeyNavigationFilter::eventFilter(QObject* watched, QEvent* event) {
    Lr2SelectNavigationController* controller = m_navigationController.data();
    if (watched != m_target || event->type() != QEvent::KeyPress || !controller || !selectScrollReady()) {
        return QObject::eventFilter(watched, event);
    }

    auto* keyEvent = static_cast<QKeyEvent*>(event);
    switch (keyEvent->key()) {
    case Qt::Key_Up:
        controller->decrementViewIndex(keyEvent->isAutoRepeat());
        keyEvent->accept();
        return true;
    case Qt::Key_Down:
        controller->incrementViewIndex(keyEvent->isAutoRepeat());
        keyEvent->accept();
        return true;
    default:
        return QObject::eventFilter(watched, event);
    }
}

void Lr2SelectKeyNavigationFilter::uninstallTargetFilter() {
    if (m_target) {
        m_target->removeEventFilter(this);
    }
}

void Lr2SelectKeyNavigationFilter::installTargetFilter() {
    if (m_target) {
        m_target->installEventFilter(this);
    }
}
