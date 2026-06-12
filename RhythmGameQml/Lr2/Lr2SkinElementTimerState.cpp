#include "Lr2SkinElementTimerState.h"

Lr2SkinElementTimerState::Lr2SkinElementTimerState(QObject* parent)
    : QObject(parent) {}

bool Lr2SkinElementTimerState::dstTimerCanFire() const {
    return m_dstTimerCanFire;
}

bool Lr2SkinElementTimerState::srcTimerCanFire() const {
    return m_srcTimerCanFire;
}

int Lr2SkinElementTimerState::dstTimerFire() const {
    return m_dstTimerFire;
}

int Lr2SkinElementTimerState::srcTimerFire() const {
    return m_srcTimerFire;
}

void Lr2SkinElementTimerState::setSnapshot(bool dstCanFire,
                                           int dstFire,
                                           bool srcCanFire,
                                           int srcFire) {
    const bool dstCanFireChanged = m_dstTimerCanFire != dstCanFire;
    const bool srcCanFireChanged = m_srcTimerCanFire != srcCanFire;
    const bool dstFireChanged = m_dstTimerFire != dstFire;
    const bool srcFireChanged = m_srcTimerFire != srcFire;
    if (!dstCanFireChanged && !srcCanFireChanged && !dstFireChanged && !srcFireChanged) {
        return;
    }

    m_dstTimerCanFire = dstCanFire;
    m_srcTimerCanFire = srcCanFire;
    m_dstTimerFire = dstFire;
    m_srcTimerFire = srcFire;

    if (dstCanFireChanged) {
        emit dstTimerCanFireChanged();
    }
    if (srcCanFireChanged) {
        emit srcTimerCanFireChanged();
    }
    if (dstFireChanged) {
        emit dstTimerFireChanged();
    }
    if (srcFireChanged) {
        emit srcTimerFireChanged();
    }
}
