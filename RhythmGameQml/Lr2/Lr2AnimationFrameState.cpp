#include "Lr2AnimationFrameState.h"

#include "Lr2SkinClock.h"

#include <algorithm>
#include <cmath>

namespace rt = lr2skin::runtime;

Lr2AnimationFrameState::Lr2AnimationFrameState(QObject* parent) : QObject(parent) {}

QObject* Lr2AnimationFrameState::skinClock() const {
    return m_skinClock;
}

void Lr2AnimationFrameState::setSkinClock(QObject* clock) {
    auto* typedClock = qobject_cast<Lr2SkinClock*>(clock);
    if (m_skinClock == typedClock) {
        return;
    }

    m_skinClock = typedClock;
    reconnectClock();
    emit skinClockChanged();
}

int Lr2AnimationFrameState::clockMode() const {
    return m_clockMode;
}

void Lr2AnimationFrameState::setClockMode(int mode) {
    mode = std::clamp(mode, static_cast<int>(ManualClock), static_cast<int>(SelectInfoClock));
    if (m_clockMode == mode) {
        return;
    }

    m_clockMode = mode;
    reconnectClock();
    emit clockModeChanged();
}

bool Lr2AnimationFrameState::isEnabled() const {
    return m_enabled;
}

void Lr2AnimationFrameState::setEnabled(bool enabled) {
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    reconnectClock();
    emit enabledChanged();
    updateFrameIndex();
}

QVariant Lr2AnimationFrameState::sourceData() const {
    return m_sourceData;
}

void Lr2AnimationFrameState::setSourceData(const QVariant& sourceData) {
    if (m_sourceData == sourceData) {
        return;
    }

    m_sourceData = sourceData;
    rebuildSource();
    emit sourceDataChanged();
    updateFrameIndex();
}

int Lr2AnimationFrameState::skinTime() const {
    return m_skinTime;
}

void Lr2AnimationFrameState::setSkinTime(int skinTime) {
    skinTime = std::max(0, skinTime);
    if (m_skinTime == skinTime) {
        return;
    }

    m_skinTime = skinTime;
    emit skinTimeChanged();
    updateFrameIndex();
}

QVariant Lr2AnimationFrameState::timers() const {
    return m_timers;
}

void Lr2AnimationFrameState::setTimers(const QVariant& timers) {
    if (m_timers == timers) {
        return;
    }

    m_timers = timers;
    emit timersChanged();
    updateFrameIndex();
}

int Lr2AnimationFrameState::timerFire() const {
    return m_timerFire;
}

void Lr2AnimationFrameState::setTimerFire(int timerFire) {
    if (m_timerFire == timerFire) {
        return;
    }

    m_timerFire = timerFire;
    emit timerFireChanged();
    updateFrameIndex();
}

int Lr2AnimationFrameState::frameOverride() const {
    return m_frameOverride;
}

void Lr2AnimationFrameState::setFrameOverride(int frameOverride) {
    if (m_frameOverride == frameOverride) {
        return;
    }

    m_frameOverride = frameOverride;
    emit frameOverrideChanged();
    updateFrameIndex();
}

int Lr2AnimationFrameState::frameGroupSize() const {
    return m_frameGroupSize;
}

void Lr2AnimationFrameState::setFrameGroupSize(int frameGroupSize) {
    frameGroupSize = std::max(1, frameGroupSize);
    if (m_frameGroupSize == frameGroupSize) {
        return;
    }

    m_frameGroupSize = frameGroupSize;
    emit frameGroupSizeChanged();
    updateFrameIndex();
}

int Lr2AnimationFrameState::textureWidth() const {
    return m_textureWidth;
}

void Lr2AnimationFrameState::setTextureWidth(int textureWidth) {
    textureWidth = std::max(0, textureWidth);
    if (m_textureWidth == textureWidth) {
        return;
    }

    m_textureWidth = textureWidth;
    emit textureWidthChanged();
    updateFrameIndex();
}

int Lr2AnimationFrameState::textureHeight() const {
    return m_textureHeight;
}

void Lr2AnimationFrameState::setTextureHeight(int textureHeight) {
    textureHeight = std::max(0, textureHeight);
    if (m_textureHeight == textureHeight) {
        return;
    }

    m_textureHeight = textureHeight;
    emit textureHeightChanged();
    updateFrameIndex();
}

int Lr2AnimationFrameState::frameIndex() const {
    return m_frameIndex;
}

QVector4D Lr2AnimationFrameState::sourceRect() const {
    return m_sourceRect;
}

QRectF Lr2AnimationFrameState::sourceClipRect() const {
    return m_sourceClipRect;
}

void Lr2AnimationFrameState::rebuildSource() {
    Source next;
    rt::readSource(m_sourceData, next);
    m_source = next;
}

void Lr2AnimationFrameState::reconnectClock() {
    if (m_clockConnection) {
        QObject::disconnect(m_clockConnection);
        m_clockConnection = {};
    }

    if (!m_enabled || !m_skinClock || m_clockMode == ManualClock) {
        return;
    }

    switch (m_clockMode) {
    case RenderClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::renderSkinTimeChanged,
            this,
            &Lr2AnimationFrameState::updateSkinTimeFromClock);
        break;
    case SelectSourceClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::selectSourceSkinTimeChanged,
            this,
            &Lr2AnimationFrameState::updateSkinTimeFromClock);
        break;
    case BarClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::barSkinTimeChanged,
            this,
            &Lr2AnimationFrameState::updateSkinTimeFromClock);
        break;
    case GlobalClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::globalSkinTimeChanged,
            this,
            &Lr2AnimationFrameState::updateSkinTimeFromClock);
        break;
    case SelectLiveClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::selectLiveSkinTimeChanged,
            this,
            &Lr2AnimationFrameState::updateSkinTimeFromClock);
        break;
    case SelectInfoClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::selectInfoElapsedChanged,
            this,
            &Lr2AnimationFrameState::updateSkinTimeFromClock);
        break;
    default:
        break;
    }

    updateSkinTimeFromClock();
}

void Lr2AnimationFrameState::updateSkinTimeFromClock() {
    if (!m_skinClock || m_clockMode == ManualClock) {
        return;
    }

    setSkinTime(clockSkinTime());
}

void Lr2AnimationFrameState::updateFrameIndex() {
    int next = 0;
    const int frames = frameCount(m_source);
    const int animationFrames = m_frameGroupSize > 1
        ? std::max(1, frames / m_frameGroupSize)
        : frames;

    if (m_enabled && m_frameOverride >= 0) {
        next = std::clamp(m_frameOverride, 0, std::max(0, frames - 1));
    } else if (m_enabled && m_source.valid && animationFrames > 1 && m_source.cycle > 0) {
        const qreal fire = effectiveTimerFire();
        const qreal animTime = m_skinTime - fire;
        const qreal msPerFrame = static_cast<qreal>(m_source.cycle) / animationFrames;
        if (fire >= 0.0 && animTime >= 0.0 && msPerFrame >= 1.0) {
            const qreal phase = std::fmod(animTime, static_cast<qreal>(m_source.cycle));
            next = std::clamp(static_cast<int>(std::floor(phase / msPerFrame)), 0, animationFrames - 1);
        }
    }

    const QVector4D nextRect = sourceRectFor(m_source, next, m_textureWidth, m_textureHeight);
    const QRectF nextClipRect = sourceClipRectFor(m_source, next);
    const bool frameChanged = m_frameIndex != next;
    const bool rectChanged = m_sourceRect != nextRect || m_sourceClipRect != nextClipRect;

    if (frameChanged) {
        m_frameIndex = next;
        emit frameIndexChanged();
    }
    if (rectChanged) {
        m_sourceRect = nextRect;
        m_sourceClipRect = nextClipRect;
        emit sourceRectChanged();
    }
}

int Lr2AnimationFrameState::clockSkinTime() const {
    if (!m_skinClock) {
        return m_skinTime;
    }

    switch (m_clockMode) {
    case RenderClock:
        return m_skinClock->renderSkinTime();
    case SelectSourceClock:
        return m_skinClock->selectSourceSkinTime();
    case BarClock:
        return m_skinClock->barSkinTime();
    case GlobalClock:
        return m_skinClock->globalSkinTime();
    case SelectLiveClock:
        return m_skinClock->selectLiveSkinTime();
    case SelectInfoClock:
        return m_skinClock->selectInfoElapsed();
    default:
        return m_skinTime;
    }
}

qreal Lr2AnimationFrameState::effectiveTimerFire() const {
    if (m_timerFire > -2147483648) {
        return m_timerFire;
    }
    if (!m_timers.isValid() || m_timers.isNull()) {
        return 0.0;
    }
    return rt::timerValue(m_timers, m_source.timer);
}

int Lr2AnimationFrameState::frameCount(const Source& source) {
    return std::max(1, source.divX) * std::max(1, source.divY);
}

QVector4D Lr2AnimationFrameState::sourceRectFor(const Source& source,
                                                int frameIndex,
                                                int textureWidth,
                                                int textureHeight) {
    if (!source.valid
            || textureWidth <= 0
            || textureHeight <= 0
            || source.x < 0
            || source.y < 0
            || source.w <= 0
            || source.h <= 0) {
        return QVector4D(0.0f, 0.0f, 1.0f, 1.0f);
    }

    const int divX = std::max(1, source.divX);
    const int divY = std::max(1, source.divY);
    const qreal cellW = static_cast<qreal>(source.w) / divX;
    const qreal cellH = static_cast<qreal>(source.h) / divY;
    const int col = frameIndex % divX;
    const int row = (frameIndex / divX) % divY;

    return QVector4D(
        static_cast<float>((source.x + col * cellW) / textureWidth),
        static_cast<float>((source.y + row * cellH) / textureHeight),
        static_cast<float>(cellW / textureWidth),
        static_cast<float>(cellH / textureHeight));
}

QRectF Lr2AnimationFrameState::sourceClipRectFor(const Source& source, int frameIndex) {
    if (!source.valid || source.w <= 0 || source.h <= 0) {
        return QRectF(0.0, 0.0, 0.0, 0.0);
    }

    const int divX = std::max(1, source.divX);
    const int divY = std::max(1, source.divY);
    const qreal cellW = static_cast<qreal>(source.w) / divX;
    const qreal cellH = static_cast<qreal>(source.h) / divY;
    const int col = frameIndex % divX;
    const int row = (frameIndex / divX) % divY;

    return QRectF(source.x + col * cellW, source.y + row * cellH, cellW, cellH);
}
