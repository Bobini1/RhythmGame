#include "Lr2SkinFrameDriver.h"

#include "Lr2GameplayFrameState.h"
#include "Lr2SelectVisualState.h"
#include "Lr2SkinClock.h"

#include <QDateTime>
#include <QVariant>

#include <algorithm>
#include <cmath>

Lr2SkinFrameDriver::Lr2SkinFrameDriver(QObject* parent) : QObject(parent) {}

Lr2SkinClock* Lr2SkinFrameDriver::clock() const {
    return m_clock;
}

void Lr2SkinFrameDriver::setClock(Lr2SkinClock* clock) {
    if (m_clock == clock) {
        return;
    }
    m_clock = clock;
    emit clockChanged();
}

QObject* Lr2SkinFrameDriver::frameAnimation() const {
    return m_frameAnimation;
}

void Lr2SkinFrameDriver::setFrameAnimation(QObject* animation) {
    if (m_frameAnimation == animation) {
        return;
    }
    m_frameAnimation = animation;
    reconnectFrameAnimation();
    emit frameAnimationChanged();
}

Lr2GameplayFrameState* Lr2SkinFrameDriver::gameplayFrameState() const {
    return m_gameplayFrameState;
}

void Lr2SkinFrameDriver::setGameplayFrameState(Lr2GameplayFrameState* state) {
    if (m_gameplayFrameState == state) {
        return;
    }
    m_gameplayFrameState = state;
    emit gameplayFrameStateChanged();
}

Lr2SelectVisualState* Lr2SkinFrameDriver::selectVisualState() const {
    return m_selectVisualState;
}

void Lr2SkinFrameDriver::setSelectVisualState(Lr2SelectVisualState* state) {
    if (m_selectVisualState == state) {
        return;
    }
    m_selectVisualState = state;
    emit selectVisualStateChanged();
}

bool Lr2SkinFrameDriver::gameplayScreen() const {
    return m_gameplayScreen;
}

void Lr2SkinFrameDriver::setGameplayScreen(bool value) {
    if (m_gameplayScreen == value) {
        return;
    }
    m_gameplayScreen = value;
    emit gameplayScreenChanged();
}

bool Lr2SkinFrameDriver::gameplayStartupPending() const {
    return m_gameplayStartupPending;
}

void Lr2SkinFrameDriver::setGameplayStartupPending(bool value) {
    if (m_gameplayStartupPending == value) {
        return;
    }
    m_gameplayStartupPending = value;
    emit gameplayStartupPendingChanged();
}

int Lr2SkinFrameDriver::currentFps() const {
    return m_currentFps;
}

int Lr2SkinFrameDriver::fpsSampleIntervalMs() const {
    return m_fpsSampleIntervalMs;
}

void Lr2SkinFrameDriver::setFpsSampleIntervalMs(int value) {
    value = std::max(1, value);
    if (m_fpsSampleIntervalMs == value) {
        return;
    }
    m_fpsSampleIntervalMs = value;
    emit fpsSampleIntervalMsChanged();
}

void Lr2SkinFrameDriver::tick(qreal smoothFrameTime) {
    if (!m_clock) {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qreal now = static_cast<qreal>(nowMs);
    if (m_clockRestartPending) {
        m_clockRestartPending = false;
        m_clock->restart(now);
        m_clock->restartSelectInfoTimer();
    }

    if (nowMs - m_lastFpsSampleMs >= m_fpsSampleIntervalMs) {
        m_lastFpsSampleMs = nowMs;
        setCurrentFps(smoothFrameTime > 0.0
            ? static_cast<int>(std::lround(1.0 / smoothFrameTime))
            : 0);
    }

    if (m_gameplayScreen && m_gameplayFrameState) {
        const int frameSkinTime = std::max(
            0,
            static_cast<int>(std::floor(now - m_clock->sceneStartMs())));
        m_gameplayFrameState->refresh(frameSkinTime);
    } else if (m_selectVisualState) {
        m_selectVisualState->advanceAnimation(now);
    }

    if (m_gameplayScreen) {
        m_clock->advancePreciseFrame(now);
    } else {
        m_clock->advanceFrame(now);
    }

    if (m_gameplayScreen && m_gameplayStartupPending) {
        emit gameplayStartupTickRequested();
    }
}

void Lr2SkinFrameDriver::setCurrentFps(int value) {
    if (m_currentFps == value) {
        return;
    }
    m_currentFps = value;
    emit currentFpsChanged();
}

void Lr2SkinFrameDriver::requestClockRestart() {
    m_clockRestartPending = true;
}

void Lr2SkinFrameDriver::reconnectFrameAnimation() {
    if (m_frameAnimationConnection) {
        QObject::disconnect(m_frameAnimationConnection);
        m_frameAnimationConnection = {};
    }
    if (!m_frameAnimation) {
        return;
    }

    m_frameAnimationConnection = QObject::connect(
        m_frameAnimation,
        SIGNAL(triggered()),
        this,
        SLOT(tickFrameAnimation()));
}

void Lr2SkinFrameDriver::tickFrameAnimation() {
    if (!m_frameAnimation) {
        return;
    }
    tick(m_frameAnimation->property("smoothFrameTime").toReal());
}
