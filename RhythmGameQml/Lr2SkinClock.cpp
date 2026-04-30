#include "Lr2SkinClock.h"

#include <algorithm>
#include <cmath>

Lr2SkinClock::Lr2SkinClock(QObject* parent) : QObject(parent) {}

qreal Lr2SkinClock::nowMs() const {
    return m_nowMs;
}

void Lr2SkinClock::setNowMs(qreal value) {
    if (sameReal(m_nowMs, value)) {
        return;
    }
    m_nowMs = value;
    emit nowMsChanged();
    recompute();
}

qreal Lr2SkinClock::sceneStartMs() const {
    return m_sceneStartMs;
}

void Lr2SkinClock::setSceneStartMs(qreal value) {
    if (sameReal(m_sceneStartMs, value)) {
        return;
    }
    m_sceneStartMs = value;
    emit sceneStartMsChanged();
    recompute();
}

int Lr2SkinClock::resolutionMs() const {
    return m_resolutionMs;
}

void Lr2SkinClock::setResolutionMs(int value) {
    value = std::max(1, value);
    if (m_resolutionMs == value) {
        return;
    }
    m_resolutionMs = value;
    emit resolutionMsChanged();
}

QString Lr2SkinClock::screenKey() const {
    return m_screenKey;
}

void Lr2SkinClock::setScreenKey(const QString& value) {
    if (m_screenKey == value) {
        return;
    }
    m_screenKey = value;
    emit screenKeyChanged();
    recompute();
}

int Lr2SkinClock::selectAnimationLimit() const {
    return m_selectAnimationLimit;
}

void Lr2SkinClock::setSelectAnimationLimit(int value) {
    value = std::max(0, value);
    if (m_selectAnimationLimit == value) {
        return;
    }
    m_selectAnimationLimit = value;
    emit selectAnimationLimitChanged();
    recompute();
}

int Lr2SkinClock::barAnimationLimit() const {
    return m_barAnimationLimit;
}

void Lr2SkinClock::setBarAnimationLimit(int value) {
    value = std::max(0, value);
    if (m_barAnimationLimit == value) {
        return;
    }
    m_barAnimationLimit = value;
    emit barAnimationLimitChanged();
    recompute();
}

int Lr2SkinClock::selectInfoAnimationLimit() const {
    return m_selectInfoAnimationLimit;
}

void Lr2SkinClock::setSelectInfoAnimationLimit(int value) {
    value = std::max(0, value);
    if (m_selectInfoAnimationLimit == value) {
        return;
    }
    m_selectInfoAnimationLimit = value;
    emit selectInfoAnimationLimitChanged();
    recompute();
}

int Lr2SkinClock::selectInfoStartSkinTime() const {
    return m_selectInfoStartSkinTime;
}

void Lr2SkinClock::setSelectInfoStartSkinTime(int value) {
    value = std::max(0, value);
    if (m_selectInfoStartSkinTime == value) {
        return;
    }
    m_selectInfoStartSkinTime = value;
    emit selectInfoStartSkinTimeChanged();
    recompute();
}

int Lr2SkinClock::globalSkinTime() const {
    return m_globalSkinTime;
}

int Lr2SkinClock::selectLiveSkinTime() const {
    return m_selectLiveSkinTime;
}

int Lr2SkinClock::selectSourceSkinTime() const {
    return m_selectSourceSkinTime;
}

int Lr2SkinClock::selectInfoElapsed() const {
    return m_selectInfoElapsed;
}

int Lr2SkinClock::renderSkinTime() const {
    return m_renderSkinTime;
}

int Lr2SkinClock::barSkinTime() const {
    return m_barSkinTime;
}

qreal Lr2SkinClock::quantize(qreal now) const {
    const auto resolution = static_cast<qreal>(std::max(1, m_resolutionMs));
    return std::floor(now / resolution) * resolution;
}

void Lr2SkinClock::advance(qreal now) {
    advanceTo(quantize(now), true);
}

void Lr2SkinClock::advanceFrame(qreal now) {
    advanceTo(quantize(now), false);
}

void Lr2SkinClock::restart(qreal now) {
    const qreal nextNow = quantize(now);
    const bool nowChanged = !sameReal(m_nowMs, nextNow);
    const bool sceneStartChanged = !sameReal(m_sceneStartMs, nextNow);

    m_nowMs = nextNow;
    m_sceneStartMs = nextNow;

    if (nowChanged) {
        emit nowMsChanged();
    }
    if (sceneStartChanged) {
        emit sceneStartMsChanged();
    }
    if (nowChanged || sceneStartChanged) {
        recompute();
    }
}

void Lr2SkinClock::advanceTo(qreal now, bool notifyNow) {
    if (sameReal(m_nowMs, now)) {
        return;
    }

    m_nowMs = now;
    if (notifyNow) {
        emit nowMsChanged();
    }
    recompute();
}

void Lr2SkinClock::recompute() {
    const int nextGlobalSkinTime = std::max(0, static_cast<int>(std::floor(m_nowMs - m_sceneStartMs)));
    const bool isSelect = m_screenKey == QStringLiteral("select");
    const int nextRenderSkinTime = isSelect ? std::min(nextGlobalSkinTime, m_selectAnimationLimit) : nextGlobalSkinTime;
    const int nextBarSkinTime = isSelect ? std::min(nextGlobalSkinTime, m_barAnimationLimit) : nextRenderSkinTime;
    const int nextSelectSourceSkinTime = nextGlobalSkinTime;
    const int nextSelectLiveSkinTime = isSelect ? std::max(nextRenderSkinTime, nextSelectSourceSkinTime) : nextRenderSkinTime;
    const int nextSelectInfoElapsed = std::clamp(
        nextSelectLiveSkinTime - m_selectInfoStartSkinTime,
        0,
        m_selectInfoAnimationLimit);

    const bool globalChanged = m_globalSkinTime != nextGlobalSkinTime;
    const bool selectLiveChanged = m_selectLiveSkinTime != nextSelectLiveSkinTime;
    const bool selectSourceChanged = m_selectSourceSkinTime != nextSelectSourceSkinTime;
    const bool infoElapsedChanged = m_selectInfoElapsed != nextSelectInfoElapsed;
    const bool renderChanged = m_renderSkinTime != nextRenderSkinTime;
    const bool barChanged = m_barSkinTime != nextBarSkinTime;

    m_globalSkinTime = nextGlobalSkinTime;
    m_selectLiveSkinTime = nextSelectLiveSkinTime;
    m_selectSourceSkinTime = nextSelectSourceSkinTime;
    m_selectInfoElapsed = nextSelectInfoElapsed;
    m_renderSkinTime = nextRenderSkinTime;
    m_barSkinTime = nextBarSkinTime;

    if (globalChanged) {
        emit globalSkinTimeChanged();
    }
    if (selectLiveChanged) {
        emit selectLiveSkinTimeChanged();
    }
    if (selectSourceChanged) {
        emit selectSourceSkinTimeChanged();
    }
    if (infoElapsedChanged) {
        emit selectInfoElapsedChanged();
    }
    if (renderChanged) {
        emit renderSkinTimeChanged();
    }
    if (barChanged) {
        emit barSkinTimeChanged();
    }
}

bool Lr2SkinClock::sameReal(qreal lhs, qreal rhs) {
    return std::abs(lhs - rhs) <= 0.000001;
}
