#include "Lr2AnimationFrameState.h"

#include "Lr2SkinClock.h"
#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include <QJSValue>
#include <QVariantMap>
#include <algorithm>
#include <cmath>

using gameplay_logic::lr2_skin::Lr2SrcImage;

namespace {
int mapInt(const QVariantMap& map, const QString& name, int fallback) {
    const auto it = map.constFind(name);
    return it == map.constEnd() || !it->isValid() || it->isNull()
        ? fallback
        : it->toInt();
}

int jsInt(const QJSValue& value, const QString& name, int fallback) {
    const QJSValue field = value.property(name);
    return field.isUndefined() || field.isNull() ? fallback : field.toInt();
}
} // namespace

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
    mode = std::clamp(mode, static_cast<int>(ManualClock), static_cast<int>(SelectLiveClock));
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

void Lr2AnimationFrameState::rebuildSource() {
    Source next;
    readSource(m_sourceData, next);
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

    if (m_enabled && m_frameOverride >= 0) {
        next = std::clamp(m_frameOverride, 0, std::max(0, frames - 1));
    } else if (m_enabled && m_source.valid && frames > 1 && m_source.cycle > 0) {
        const qreal fire = effectiveTimerFire();
        const qreal animTime = m_skinTime - fire;
        const qreal msPerFrame = static_cast<qreal>(m_source.cycle) / frames;
        if (fire >= 0.0 && animTime >= 0.0 && msPerFrame >= 1.0) {
            const qreal phase = std::fmod(animTime, static_cast<qreal>(m_source.cycle));
            next = std::clamp(static_cast<int>(std::floor(phase / msPerFrame)), 0, frames - 1);
        }
    }

    const QVector4D nextRect = sourceRectFor(m_source, next, m_textureWidth, m_textureHeight);
    const bool frameChanged = m_frameIndex != next;
    const bool rectChanged = m_sourceRect != nextRect;

    if (frameChanged) {
        m_frameIndex = next;
        emit frameIndexChanged();
    }
    if (rectChanged) {
        m_sourceRect = nextRect;
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
    default:
        return m_skinTime;
    }
}

qreal Lr2AnimationFrameState::effectiveTimerFire() const {
    if (m_timerFire > -2147483648) {
        return m_timerFire;
    }
    return timerValue(m_source.timer);
}

qreal Lr2AnimationFrameState::timerValue(int timerIdx) const {
    if (!m_timers.isValid() || m_timers.isNull()) {
        return 0.0;
    }

    const QString key = QString::number(timerIdx);
    if (m_timers.canConvert<QVariantMap>()) {
        const QVariantMap map = m_timers.toMap();
        const auto it = map.constFind(key);
        return it == map.constEnd() || !it->isValid() || it->isNull()
            ? -1.0
            : it->toDouble();
    }

    if (m_timers.canConvert<QJSValue>()) {
        const QJSValue value = m_timers.value<QJSValue>();
        if (!value.isObject()) {
            return -1.0;
        }
        const QJSValue field = value.property(key);
        return field.isUndefined() || field.isNull() ? -1.0 : field.toNumber();
    }

    return timerIdx == 0 ? 0.0 : -1.0;
}

bool Lr2AnimationFrameState::readSource(const QVariant& value, Source& source) {
    if (value.canConvert<Lr2SrcImage>()) {
        const auto parsed = value.value<Lr2SrcImage>();
        source.valid = true;
        source.divX = std::max(1, parsed.div_x);
        source.divY = std::max(1, parsed.div_y);
        source.cycle = parsed.cycle;
        source.timer = parsed.timer;
        source.x = parsed.x;
        source.y = parsed.y;
        source.w = parsed.w;
        source.h = parsed.h;
        return true;
    }

    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        source.valid = true;
        source.divX = std::max(1, mapInt(map, QStringLiteral("div_x"), 1));
        source.divY = std::max(1, mapInt(map, QStringLiteral("div_y"), 1));
        source.cycle = mapInt(map, QStringLiteral("cycle"), 0);
        source.timer = mapInt(map, QStringLiteral("timer"), 0);
        source.x = mapInt(map, QStringLiteral("x"), 0);
        source.y = mapInt(map, QStringLiteral("y"), 0);
        source.w = mapInt(map, QStringLiteral("w"), 0);
        source.h = mapInt(map, QStringLiteral("h"), 0);
        return true;
    }

    if (!value.canConvert<QJSValue>()) {
        return false;
    }

    const QJSValue jsValue = value.value<QJSValue>();
    if (!jsValue.isObject()) {
        return false;
    }

    source.valid = true;
    source.divX = std::max(1, jsInt(jsValue, QStringLiteral("div_x"), 1));
    source.divY = std::max(1, jsInt(jsValue, QStringLiteral("div_y"), 1));
    source.cycle = jsInt(jsValue, QStringLiteral("cycle"), 0);
    source.timer = jsInt(jsValue, QStringLiteral("timer"), 0);
    source.x = jsInt(jsValue, QStringLiteral("x"), 0);
    source.y = jsInt(jsValue, QStringLiteral("y"), 0);
    source.w = jsInt(jsValue, QStringLiteral("w"), 0);
    source.h = jsInt(jsValue, QStringLiteral("h"), 0);
    return true;
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
