#include "Lr2SelectInfoTimerBridge.h"

#include "Lr2SkinClock.h"

Lr2SelectInfoTimerBridge::Lr2SelectInfoTimerBridge(QObject* parent) : QObject(parent) {}

QObject* Lr2SelectInfoTimerBridge::selectContext() const {
    return m_selectContext;
}

void Lr2SelectInfoTimerBridge::setSelectContext(QObject* context) {
    if (m_selectContext == context) {
        return;
    }

    if (m_focusRevisionConnection) {
        disconnect(m_focusRevisionConnection);
    }

    m_selectContext = context;
    if (m_selectContext) {
        m_focusRevisionConnection = connect(
            m_selectContext,
            SIGNAL(focusRevisionChanged()),
            this,
            SLOT(restartFromFocusChange()));
    } else {
        m_focusRevisionConnection = {};
    }
    emit selectContextChanged();
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
