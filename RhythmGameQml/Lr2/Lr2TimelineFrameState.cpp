#include "Lr2TimelineFrameState.h"

#include <QJSValue>
#include <QMetaType>

#include <algorithm>
#include <cmath>

namespace {
constexpr int NoTimerFire = -2147483648;

bool variantIsNullish(const QVariant& value) {
    if (!value.isValid() || value.isNull()) {
        return true;
    }
    if (value.metaType() == QMetaType::fromType<std::nullptr_t>()) {
        return true;
    }
    if (value.canConvert<QJSValue>()) {
        const QJSValue jsValue = value.value<QJSValue>();
        return jsValue.isNull() || jsValue.isUndefined();
    }
    return false;
}

QVariant propertyValue(const QVariant& value, const char* name) {
    if (value.canConvert<Lr2TimelineStateValue>()) {
        const Lr2TimelineStateValue state = value.value<Lr2TimelineStateValue>();
        const QByteArray propertyName(name);
        if (propertyName == "x") {
            return state.x;
        }
        if (propertyName == "y") {
            return state.y;
        }
        if (propertyName == "w") {
            return state.w;
        }
        if (propertyName == "h") {
            return state.h;
        }
        if (propertyName == "a") {
            return state.a;
        }
        if (propertyName == "r") {
            return state.r;
        }
        if (propertyName == "g") {
            return state.g;
        }
        if (propertyName == "b") {
            return state.b;
        }
        if (propertyName == "angle") {
            return state.angle;
        }
        if (propertyName == "center") {
            return state.center;
        }
        if (propertyName == "blend") {
            return state.blend;
        }
        if (propertyName == "filter") {
            return state.filter;
        }
        if (propertyName == "op4") {
            return state.op4;
        }
        return {};
    }

    if (value.canConvert<QJSValue>()) {
        const QJSValue jsValue = value.value<QJSValue>();
        const QJSValue property = jsValue.property(QLatin1String(name));
        if (!property.isUndefined() && !property.isNull()) {
            return property.toVariant();
        }
        return {};
    }

    const QVariantMap map = value.toMap();
    if (!map.isEmpty() || value.metaType() == QMetaType::fromType<QVariantMap>()) {
        return map.value(QString::fromLatin1(name));
    }

    const QVariantHash hash = value.toHash();
    if (!hash.isEmpty() || value.metaType() == QMetaType::fromType<QVariantHash>()) {
        return hash.value(QString::fromLatin1(name));
    }

    return {};
}

qreal realProperty(const QVariant& value, const char* name, qreal fallback) {
    const QVariant property = propertyValue(value, name);
    if (!property.isValid() || property.isNull()) {
        return fallback;
    }

    bool ok = false;
    const qreal result = property.toDouble(&ok);
    return ok && std::isfinite(result) ? result : fallback;
}

int intProperty(const QVariant& value, const char* name, int fallback) {
    const QVariant property = propertyValue(value, name);
    if (!property.isValid() || property.isNull()) {
        return fallback;
    }

    bool ok = false;
    const int result = property.toInt(&ok);
    return ok ? result : fallback;
}
}

Lr2TimelineFrameState::Lr2TimelineFrameState(QObject* parent)
    : QObject(parent)
    , m_timeline(this) {
    connect(&m_timeline, &Lr2TimelineState::stateChanged, this, &Lr2TimelineFrameState::updateFrame);
    connect(&m_timeline, &Lr2TimelineState::hasStateChanged, this, &Lr2TimelineFrameState::updateFrame);
    connect(&m_timeline, &Lr2TimelineState::analysisChanged, this, [this] {
        updateTimelineConfiguration();
        updateTimelineTimers();
        updateFrame();
    });
    updateTimelineConfiguration();
    updateFrame();
}

QVariantList Lr2TimelineFrameState::dsts() const {
    return m_dsts;
}

void Lr2TimelineFrameState::setDsts(const QVariantList& dsts) {
    if (m_dsts == dsts) {
        return;
    }
    m_dsts = dsts;
    m_timeline.setDsts(m_dsts);
    emit dstsChanged();
    updateTimelineConfiguration();
    updateTimelineTimers();
    updateFrame();
}

int Lr2TimelineFrameState::skinTime() const {
    return m_skinTime;
}

void Lr2TimelineFrameState::setSkinTime(int skinTime) {
    skinTime = std::max(0, skinTime);
    if (m_skinTime == skinTime) {
        return;
    }
    m_skinTime = skinTime;
    m_timeline.setSkinTime(m_skinTime);
    emit skinTimeChanged();
}

Lr2SkinClock* Lr2TimelineFrameState::skinClock() const {
    return m_skinClock;
}

void Lr2TimelineFrameState::setSkinClock(Lr2SkinClock* skinClock) {
    if (m_skinClock == skinClock) {
        return;
    }
    m_skinClock = skinClock;
    m_timeline.setSkinClock(m_skinClock);
    emit skinClockChanged();
}

int Lr2TimelineFrameState::skinClockMode() const {
    return m_skinClockMode;
}

void Lr2TimelineFrameState::setSkinClockMode(int skinClockMode) {
    if (m_skinClockMode == skinClockMode) {
        return;
    }
    m_skinClockMode = skinClockMode;
    m_timeline.setClockMode(m_skinClockMode);
    emit skinClockModeChanged();
}

Lr2SkinElementActiveOptionsState* Lr2TimelineFrameState::activeOptionsState() const {
    return m_activeOptionsState;
}

void Lr2TimelineFrameState::setActiveOptionsState(Lr2SkinElementActiveOptionsState* activeOptionsState) {
    if (m_activeOptionsState == activeOptionsState) {
        return;
    }
    m_activeOptionsState = activeOptionsState;
    m_timeline.setActiveOptionsState(m_activeOptionsState);
    emit activeOptionsStateChanged();
    updateTimelineConfiguration();
    updateFrame();
}

QVariant Lr2TimelineFrameState::activeOptions() const {
    return m_activeOptions;
}

void Lr2TimelineFrameState::setActiveOptions(const QVariant& activeOptions) {
    if (m_activeOptions == activeOptions) {
        return;
    }
    m_activeOptions = activeOptions;
    m_timeline.setActiveOptions(m_activeOptions);
    emit activeOptionsChanged();
    updateTimelineConfiguration();
    updateFrame();
}

QVariant Lr2TimelineFrameState::timers() const {
    return m_timers;
}

void Lr2TimelineFrameState::setTimers(const QVariant& timers) {
    if (m_timers == timers) {
        return;
    }
    m_timers = timers;
    emit timersChanged();
    updateTimelineTimers();
}

int Lr2TimelineFrameState::timerFire() const {
    return m_timerFire;
}

void Lr2TimelineFrameState::setTimerFire(int timerFire) {
    if (m_timerFire == timerFire) {
        return;
    }
    m_timerFire = timerFire;
    m_timeline.setTimerFire(m_timerFire);
    emit timerFireChanged();
}

QVariant Lr2TimelineFrameState::stateOverride() const {
    return m_stateOverride;
}

void Lr2TimelineFrameState::setStateOverride(const QVariant& stateOverride) {
    if (m_stateOverride == stateOverride) {
        return;
    }
    m_stateOverride = stateOverride;
    emit stateOverrideChanged();
    updateTimelineConfiguration();
    updateFrame();
}

bool Lr2TimelineFrameState::forceHidden() const {
    return m_forceHidden;
}

void Lr2TimelineFrameState::setForceHidden(bool forceHidden) {
    if (m_forceHidden == forceHidden) {
        return;
    }
    m_forceHidden = forceHidden;
    emit forceHiddenChanged();
    updateTimelineConfiguration();
    updateFrame();
}

bool Lr2TimelineFrameState::sliderTranslationEnabled() const {
    return m_sliderTranslationEnabled;
}

void Lr2TimelineFrameState::setSliderTranslationEnabled(bool enabled) {
    if (m_sliderTranslationEnabled == enabled) {
        return;
    }
    m_sliderTranslationEnabled = enabled;
    m_timeline.setSliderTranslationEnabled(enabled);
    emit sliderTranslationChanged();
    updateTimelineConfiguration();
    updateFrame();
}

qreal Lr2TimelineFrameState::sliderPosition() const {
    return m_sliderPosition;
}

void Lr2TimelineFrameState::setSliderPosition(qreal position) {
    if (!std::isfinite(position)) {
        position = 0.0;
    }
    if (sameReal(m_sliderPosition, position)) {
        return;
    }
    m_sliderPosition = position;
    m_timeline.setSliderPosition(position);
    emit sliderTranslationChanged();
}

int Lr2TimelineFrameState::sliderRange() const {
    return m_sliderRange;
}

void Lr2TimelineFrameState::setSliderRange(int range) {
    range = std::max(0, range);
    if (m_sliderRange == range) {
        return;
    }
    m_sliderRange = range;
    m_timeline.setSliderRange(range);
    emit sliderTranslationChanged();
}

int Lr2TimelineFrameState::sliderDirection() const {
    return m_sliderDirection;
}

void Lr2TimelineFrameState::setSliderDirection(int direction) {
    if (m_sliderDirection == direction) {
        return;
    }
    m_sliderDirection = direction;
    m_timeline.setSliderDirection(direction);
    emit sliderTranslationChanged();
}

bool Lr2TimelineFrameState::dstOffsetsEnabled() const {
    return m_dstOffsetsEnabled;
}

void Lr2TimelineFrameState::setDstOffsetsEnabled(bool enabled) {
    if (m_dstOffsetsEnabled == enabled) {
        return;
    }
    m_dstOffsetsEnabled = enabled;
    m_timeline.setDstOffsetsEnabled(enabled);
    emit dstOffsetsChanged();
    updateTimelineConfiguration();
    updateFrame();
}

qreal Lr2TimelineFrameState::dstOffsetLiftY() const {
    return m_dstOffsetLiftY;
}

void Lr2TimelineFrameState::setDstOffsetLiftY(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (sameReal(m_dstOffsetLiftY, value)) {
        return;
    }
    m_dstOffsetLiftY = value;
    m_timeline.setDstOffsetLiftY(value);
    emit dstOffsetsChanged();
}

qreal Lr2TimelineFrameState::dstOffsetLaneCoverY() const {
    return m_dstOffsetLaneCoverY;
}

void Lr2TimelineFrameState::setDstOffsetLaneCoverY(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (sameReal(m_dstOffsetLaneCoverY, value)) {
        return;
    }
    m_dstOffsetLaneCoverY = value;
    m_timeline.setDstOffsetLaneCoverY(value);
    emit dstOffsetsChanged();
}

qreal Lr2TimelineFrameState::dstOffsetHiddenY() const {
    return m_dstOffsetHiddenY;
}

void Lr2TimelineFrameState::setDstOffsetHiddenY(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (sameReal(m_dstOffsetHiddenY, value)) {
        return;
    }
    m_dstOffsetHiddenY = value;
    m_timeline.setDstOffsetHiddenY(value);
    emit dstOffsetsChanged();
}

qreal Lr2TimelineFrameState::dstOffsetHiddenA() const {
    return m_dstOffsetHiddenA;
}

void Lr2TimelineFrameState::setDstOffsetHiddenA(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (sameReal(m_dstOffsetHiddenA, value)) {
        return;
    }
    m_dstOffsetHiddenA = value;
    m_timeline.setDstOffsetHiddenA(value);
    emit dstOffsetsChanged();
}

bool Lr2TimelineFrameState::colorKeyEnabled() const {
    return m_colorKeyEnabled;
}

void Lr2TimelineFrameState::setColorKeyEnabled(bool enabled) {
    if (m_colorKeyEnabled == enabled) {
        return;
    }
    m_colorKeyEnabled = enabled;
    emit colorKeyEnabledChanged();
    updateFrame();
}

bool Lr2TimelineFrameState::supportsInvertedBlend() const {
    return m_supportsInvertedBlend;
}

void Lr2TimelineFrameState::setSupportsInvertedBlend(bool supported) {
    if (m_supportsInvertedBlend == supported) {
        return;
    }
    m_supportsInvertedBlend = supported;
    emit supportsInvertedBlendChanged();
    updateFrame();
}

bool Lr2TimelineFrameState::canUseStaticState() const {
    return m_canUseStaticState;
}

QVariant Lr2TimelineFrameState::staticState() const {
    return m_staticStateValid ? stateVariant(m_staticState) : QVariant {};
}

QVariant Lr2TimelineFrameState::timelineTimers() const {
    return m_timelineTimers;
}

QVariant Lr2TimelineFrameState::directState() const {
    return m_directState;
}

bool Lr2TimelineFrameState::hasDirectState() const {
    return m_hasDirectState;
}

bool Lr2TimelineFrameState::hasTimelineState() const {
    return m_hasTimelineState;
}

QVariant Lr2TimelineFrameState::state() const {
    return m_directState;
}

bool Lr2TimelineFrameState::hasState() const {
    return m_hasState;
}

qreal Lr2TimelineFrameState::x() const {
    return m_fields.x;
}

qreal Lr2TimelineFrameState::y() const {
    return m_fields.y;
}

qreal Lr2TimelineFrameState::w() const {
    return m_fields.w;
}

qreal Lr2TimelineFrameState::h() const {
    return m_fields.h;
}

qreal Lr2TimelineFrameState::a() const {
    return m_hasState ? m_fields.a : 0.0;
}

qreal Lr2TimelineFrameState::r() const {
    return m_fields.r;
}

qreal Lr2TimelineFrameState::g() const {
    return m_fields.g;
}

qreal Lr2TimelineFrameState::b() const {
    return m_fields.b;
}

qreal Lr2TimelineFrameState::angle() const {
    return m_fields.angle;
}

int Lr2TimelineFrameState::center() const {
    return m_fields.center;
}

int Lr2TimelineFrameState::blend() const {
    return m_fields.blend;
}

int Lr2TimelineFrameState::filter() const {
    return m_fields.filter;
}

int Lr2TimelineFrameState::op4() const {
    return m_fields.op4;
}

int Lr2TimelineFrameState::rawBlendMode() const {
    return m_rawBlendMode;
}

int Lr2TimelineFrameState::blendMode() const {
    return m_blendMode;
}

qreal Lr2TimelineFrameState::tintR() const {
    return m_tintR;
}

qreal Lr2TimelineFrameState::tintG() const {
    return m_tintG;
}

qreal Lr2TimelineFrameState::tintB() const {
    return m_tintB;
}

bool Lr2TimelineFrameState::hasColorTint() const {
    return m_hasColorTint;
}

QColor Lr2TimelineFrameState::tintColor() const {
    return m_tintColor;
}

qreal Lr2TimelineFrameState::opacity() const {
    return m_opacity;
}

Lr2TimelineState* Lr2TimelineFrameState::timelineState() {
    return &m_timeline;
}

bool Lr2TimelineFrameState::truthy(const QVariant& value) {
    if (variantIsNullish(value)) {
        return false;
    }
    if (value.canConvert<QJSValue>()) {
        return value.value<QJSValue>().toBool();
    }
    switch (value.metaType().id()) {
    case QMetaType::Bool:
        return value.toBool();
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
    case QMetaType::Float:
        return value.toDouble() != 0.0;
    case QMetaType::QString:
        return !value.toString().isEmpty();
    default:
        return true;
    }
}

bool Lr2TimelineFrameState::sameReal(qreal left, qreal right) {
    return std::abs(left - right) <= 0.0001;
}

bool Lr2TimelineFrameState::sameStateValue(const Lr2TimelineStateValue& left,
                                           const Lr2TimelineStateValue& right) {
    return left.valid == right.valid
        && sameReal(left.x, right.x)
        && sameReal(left.y, right.y)
        && sameReal(left.w, right.w)
        && sameReal(left.h, right.h)
        && sameReal(left.a, right.a)
        && sameReal(left.r, right.r)
        && sameReal(left.g, right.g)
        && sameReal(left.b, right.b)
        && sameReal(left.angle, right.angle)
        && left.center == right.center
        && sameReal(left.sortId, right.sortId)
        && left.blend == right.blend
        && left.filter == right.filter
        && left.op1 == right.op1
        && left.op2 == right.op2
        && left.op3 == right.op3
        && left.op4 == right.op4;
}

QVariant Lr2TimelineFrameState::stateVariant(const Lr2TimelineStateValue& state) {
    return QVariant::fromValue(state);
}

Lr2TimelineFrameState::StateFields Lr2TimelineFrameState::fieldsFromVariant(const QVariant& value) {
    StateFields fields;
    fields.x = realProperty(value, "x", 0.0);
    fields.y = realProperty(value, "y", 0.0);
    fields.w = realProperty(value, "w", 0.0);
    fields.h = realProperty(value, "h", 0.0);
    fields.a = realProperty(value, "a", 255.0);
    fields.r = realProperty(value, "r", 255.0);
    fields.g = realProperty(value, "g", 255.0);
    fields.b = realProperty(value, "b", 255.0);
    fields.angle = realProperty(value, "angle", 0.0);
    fields.center = intProperty(value, "center", 0);
    fields.blend = intProperty(value, "blend", 0);
    fields.filter = intProperty(value, "filter", 0);
    fields.op4 = intProperty(value, "op4", 0);
    return fields;
}

Lr2TimelineFrameState::StateFields Lr2TimelineFrameState::fieldsFromState(const Lr2TimelineStateValue& state) {
    StateFields fields;
    fields.x = state.x;
    fields.y = state.y;
    fields.w = state.w;
    fields.h = state.h;
    fields.a = state.a;
    fields.r = state.r;
    fields.g = state.g;
    fields.b = state.b;
    fields.angle = state.angle;
    fields.center = state.center;
    fields.blend = state.blend;
    fields.filter = state.filter;
    fields.op4 = state.op4;
    return fields;
}

qreal Lr2TimelineFrameState::clampedTint(qreal value) {
    return std::clamp(value, 0.0, 255.0) / 255.0;
}

void Lr2TimelineFrameState::updateTimelineConfiguration() {
    const bool nextCanUseStaticState = !truthy(m_stateOverride)
        && !m_sliderTranslationEnabled
        && !m_dstOffsetsEnabled
        && !m_forceHidden
        && m_timeline.canUseStaticState();
    const bool nextEnabled = !truthy(m_stateOverride) && !m_forceHidden && !nextCanUseStaticState;
    m_timeline.setEnabled(nextEnabled);
}

void Lr2TimelineFrameState::updateTimelineTimers() {
    const QVariant nextTimelineTimers = m_timeline.usesDynamicTimer() ? m_timers : QVariant {};
    if (m_timelineTimers == nextTimelineTimers) {
        return;
    }
    m_timelineTimers = nextTimelineTimers;
    m_timeline.setTimers(m_timelineTimers);
    emit timelineTimersChanged();
}

void Lr2TimelineFrameState::updateFrame() {
    const bool previousCanUseStaticState = m_canUseStaticState;
    const bool previousStaticStateValid = m_staticStateValid;
    const Lr2TimelineStateValue previousStaticState = m_staticState;
    const QVariant previousDirectState = m_directState;
    const bool previousHasDirectState = m_hasDirectState;
    const bool previousHasTimelineState = m_hasTimelineState;
    const bool previousHasState = m_hasState;
    const StateFields previousFields = m_fields;
    const int previousRawBlendMode = m_rawBlendMode;
    const int previousBlendMode = m_blendMode;
    const qreal previousTintR = m_tintR;
    const qreal previousTintG = m_tintG;
    const qreal previousTintB = m_tintB;
    const bool previousHasColorTint = m_hasColorTint;
    const QColor previousTintColor = m_tintColor;
    const qreal previousOpacity = m_opacity;

    m_canUseStaticState = !truthy(m_stateOverride)
        && !m_sliderTranslationEnabled
        && !m_dstOffsetsEnabled
        && !m_forceHidden
        && m_timeline.canUseStaticState();
    m_staticState = m_canUseStaticState ? m_timeline.staticState() : Lr2TimelineStateValue {};
    m_staticStateValid = m_canUseStaticState && m_staticState.valid;

    if (m_forceHidden) {
        m_directState = {};
    } else if (truthy(m_stateOverride)) {
        m_directState = m_stateOverride;
    } else if (m_staticStateValid) {
        m_directState = stateVariant(m_staticState);
    } else {
        m_directState = {};
    }

    m_hasDirectState = truthy(m_directState);
    m_hasTimelineState = !m_forceHidden && !m_hasDirectState && m_timeline.hasState();
    m_hasState = m_hasDirectState || m_hasTimelineState;

    if (m_hasDirectState) {
        m_fields = fieldsFromVariant(m_directState);
    } else if (m_hasTimelineState) {
        m_fields = fieldsFromState(m_timeline.state());
    } else {
        m_fields = {};
        m_fields.a = 0.0;
    }

    m_rawBlendMode = m_hasState ? m_fields.blend : 1;
    m_blendMode = normalizedBlendMode(m_rawBlendMode);
    m_tintR = m_hasState ? clampedTint(m_fields.r) : 1.0;
    m_tintG = m_hasState ? clampedTint(m_fields.g) : 1.0;
    m_tintB = m_hasState ? clampedTint(m_fields.b) : 1.0;
    m_hasColorTint = std::abs(m_tintR - 1.0) > 0.001
        || std::abs(m_tintG - 1.0) > 0.001
        || std::abs(m_tintB - 1.0) > 0.001;
    m_tintColor = QColor::fromRgbF(m_tintR, m_tintG, m_tintB, 1.0);
    m_opacity = m_hasState ? m_fields.a / 255.0 : 0.0;

    if (previousCanUseStaticState != m_canUseStaticState
            || previousStaticStateValid != m_staticStateValid
            || !sameStateValue(previousStaticState, m_staticState)) {
        emit staticStateChanged();
    }
    if (previousHasDirectState != m_hasDirectState || previousDirectState != m_directState) {
        emit directStateChanged();
    }
    if (previousHasTimelineState != m_hasTimelineState) {
        emit hasTimelineStateChanged();
    }
    if (previousHasState != m_hasState) {
        emit hasStateChanged();
    }
    if (!sameReal(previousFields.x, m_fields.x)
            || !sameReal(previousFields.y, m_fields.y)
            || !sameReal(previousFields.w, m_fields.w)
            || !sameReal(previousFields.h, m_fields.h)
            || !sameReal(previousFields.angle, m_fields.angle)
            || previousFields.center != m_fields.center) {
        emit geometryChanged();
    }
    if (!sameReal(previousFields.a, m_fields.a) || !sameReal(previousOpacity, m_opacity)) {
        emit alphaChanged();
    }
    if (!sameReal(previousFields.r, m_fields.r)
            || !sameReal(previousFields.g, m_fields.g)
            || !sameReal(previousFields.b, m_fields.b)
            || !sameReal(previousTintR, m_tintR)
            || !sameReal(previousTintG, m_tintG)
            || !sameReal(previousTintB, m_tintB)
            || previousHasColorTint != m_hasColorTint
            || previousTintColor != m_tintColor) {
        emit colorChanged();
    }
    if (previousFields.blend != m_fields.blend
            || previousRawBlendMode != m_rawBlendMode
            || previousBlendMode != m_blendMode) {
        emit blendChanged();
    }
    if (previousFields.filter != m_fields.filter) {
        emit filterChanged();
    }
    if (previousFields.op4 != m_fields.op4) {
        emit op4Changed();
    }
}

int Lr2TimelineFrameState::normalizedBlendMode(int rawBlendMode) const {
    if (rawBlendMode == 0 && !m_colorKeyEnabled) {
        return 1;
    }
    if (rawBlendMode == 10 && !m_supportsInvertedBlend) {
        return 1;
    }
    if (rawBlendMode == 3 || rawBlendMode == 4 || rawBlendMode == 5 ||
        rawBlendMode == 6 || rawBlendMode == 9 || rawBlendMode == 11) {
        return 1;
    }
    return rawBlendMode;
}
