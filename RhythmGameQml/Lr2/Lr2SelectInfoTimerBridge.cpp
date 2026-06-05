#include "Lr2SelectInfoTimerBridge.h"

#include "Lr2SelectNavigationController.h"
#include "Lr2SkinClock.h"

Lr2SelectInfoTimerBridge::Lr2SelectInfoTimerBridge(QObject* parent) : QObject(parent) {}

Lr2SelectNavigationController* Lr2SelectInfoTimerBridge::navigationController() const {
    return m_navigationController;
}

void Lr2SelectInfoTimerBridge::setNavigationController(Lr2SelectNavigationController* controller) {
    if (m_navigationController == controller) {
        return;
    }

    if (m_focusedStateConnection) {
        disconnect(m_focusedStateConnection);
    }

    m_navigationController = controller;
    if (m_navigationController) {
        m_focusedStateConnection = connect(
            m_navigationController,
            &Lr2SelectNavigationController::focusedStateChanged,
            this,
            &Lr2SelectInfoTimerBridge::restartFromFocusChange);
    } else {
        m_focusedStateConnection = {};
    }
    emit navigationControllerChanged();
}

Lr2SkinClock* Lr2SelectInfoTimerBridge::clock() const {
    return m_clock;
}

void Lr2SelectInfoTimerBridge::setClock(Lr2SkinClock* clock) {
    if (m_clock == clock) {
        return;
    }
    m_clock = clock;
    emit clockChanged();
}

bool Lr2SelectInfoTimerBridge::isActive() const {
    return m_active;
}

void Lr2SelectInfoTimerBridge::setActive(bool active) {
    if (m_active == active) {
        return;
    }
    m_active = active;
    emit activeChanged();
}

void Lr2SelectInfoTimerBridge::restartFromFocusChange() {
    if (m_active && m_clock) {
        m_clock->restartSelectInfoTimer();
    }
}
