#include "Lr2TimelineState.h"

#include "Lr2SkinClock.h"
#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include <QJSValue>
#include <QVariantMap>
#include <algorithm>
#include <cmath>

using gameplay_logic::lr2_skin::Lr2Dst;
using gameplay_logic::lr2_skin::Lr2SrcBarGraph;
using gameplay_logic::lr2_skin::Lr2SrcImage;
using gameplay_logic::lr2_skin::Lr2SrcNumber;

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

Lr2TimelineState::Lr2TimelineState(QObject* parent) : QObject(parent) {}

QObject* Lr2TimelineState::skinClock() const {
    return m_skinClock;
}

void Lr2TimelineState::setSkinClock(QObject* clock) {
    auto* typedClock = qobject_cast<Lr2SkinClock*>(clock);
    if (m_skinClock == typedClock) {
        return;
    }

    m_skinClock = typedClock;
    reconnectClock();
    emit skinClockChanged();
}

int Lr2TimelineState::clockMode() const {
    return m_clockMode;
}

void Lr2TimelineState::setClockMode(int mode) {
    mode = std::clamp(mode, static_cast<int>(ManualClock), static_cast<int>(SelectInfoClock));
    if (m_clockMode == mode) {
        return;
    }

    m_clockMode = mode;
    reconnectClock();
    emit clockModeChanged();
}

bool Lr2TimelineState::isEnabled() const {
    return m_enabled;
}

void Lr2TimelineState::setEnabled(bool enabled) {
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    reconnectClock();
    emit enabledChanged();
    updateState();
}

QVariantList Lr2TimelineState::dsts() const {
    return m_dstsValue;
}

void Lr2TimelineState::setDsts(const QVariantList& dsts) {
    if (m_dstsValue == dsts) {
        return;
    }

    m_dstsValue = dsts;
    rebuildDsts();
    rebuildAnalysis();
    updateAnimationLimit();
    emit dstsChanged();
    updateState();
}

int Lr2TimelineState::skinTime() const {
    return m_requestedSkinTime;
}

void Lr2TimelineState::setSkinTime(int time) {
    time = std::max(0, time);
    const int nextEffective = effectiveSkinTime(time);
    const bool requestedChanged = m_requestedSkinTime != time;
    const bool effectiveChanged = m_effectiveSkinTime != nextEffective;

    if (!requestedChanged && !effectiveChanged) {
        return;
    }

    m_requestedSkinTime = time;
    m_effectiveSkinTime = nextEffective;

    if (requestedChanged) {
        emit skinTimeChanged();
    }
    if (effectiveChanged) {
        updateState();
    }
}

QVariant Lr2TimelineState::timers() const {
    return m_timers;
}

void Lr2TimelineState::setTimers(const QVariant& timers) {
    if (m_timers == timers) {
        return;
    }

    m_timers = timers;
    emit timersChanged();
    updateState();
}

int Lr2TimelineState::timerFire() const {
    return m_timerFire;
}

void Lr2TimelineState::setTimerFire(int timerFire) {
    if (m_timerFire == timerFire) {
        return;
    }

    m_timerFire = timerFire;
    emit timerFireChanged();
    updateState();
}

QVariant Lr2TimelineState::activeOptions() const {
    return m_activeOptions;
}

void Lr2TimelineState::setActiveOptions(const QVariant& options) {
    if (m_activeOptions == options) {
        return;
    }

    m_activeOptions = options;
    rebuildActiveOptionSet();
    emit activeOptionsChanged();
    updateState();
}

bool Lr2TimelineState::sliderTranslationEnabled() const {
    return m_sliderTranslationEnabled;
}

void Lr2TimelineState::setSliderTranslationEnabled(bool enabled) {
    if (m_sliderTranslationEnabled == enabled) {
        return;
    }

    m_sliderTranslationEnabled = enabled;
    emit sliderTranslationChanged();
    updateState();
}

qreal Lr2TimelineState::sliderPosition() const {
    return m_sliderPosition;
}

void Lr2TimelineState::setSliderPosition(qreal position) {
    if (!std::isfinite(position)) {
        position = 0.0;
    }
    if (std::abs(m_sliderPosition - position) <= 0.0001) {
        return;
    }

    m_sliderPosition = position;
    emit sliderTranslationChanged();
    updateState();
}

int Lr2TimelineState::sliderRange() const {
    return m_sliderRange;
}

void Lr2TimelineState::setSliderRange(int range) {
    range = std::max(0, range);
    if (m_sliderRange == range) {
        return;
    }

    m_sliderRange = range;
    emit sliderTranslationChanged();
    updateState();
}

int Lr2TimelineState::sliderDirection() const {
    return m_sliderDirection;
}

void Lr2TimelineState::setSliderDirection(int direction) {
    if (m_sliderDirection == direction) {
        return;
    }

    m_sliderDirection = direction;
    emit sliderTranslationChanged();
    updateState();
}

bool Lr2TimelineState::dstOffsetsEnabled() const {
    return m_dstOffsetsEnabled;
}

void Lr2TimelineState::setDstOffsetsEnabled(bool enabled) {
    if (m_dstOffsetsEnabled == enabled) {
        return;
    }

    m_dstOffsetsEnabled = enabled;
    emit dstOffsetsChanged();
    updateState();
}

qreal Lr2TimelineState::dstOffsetLiftY() const {
    return m_dstOffsetLiftY;
}

void Lr2TimelineState::setDstOffsetLiftY(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (std::abs(m_dstOffsetLiftY - value) <= 0.0001) {
        return;
    }

    m_dstOffsetLiftY = value;
    emit dstOffsetsChanged();
    updateState();
}

qreal Lr2TimelineState::dstOffsetLaneCoverY() const {
    return m_dstOffsetLaneCoverY;
}

void Lr2TimelineState::setDstOffsetLaneCoverY(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (std::abs(m_dstOffsetLaneCoverY - value) <= 0.0001) {
        return;
    }

    m_dstOffsetLaneCoverY = value;
    emit dstOffsetsChanged();
    updateState();
}

qreal Lr2TimelineState::dstOffsetHiddenY() const {
    return m_dstOffsetHiddenY;
}

void Lr2TimelineState::setDstOffsetHiddenY(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (std::abs(m_dstOffsetHiddenY - value) <= 0.0001) {
        return;
    }

    m_dstOffsetHiddenY = value;
    emit dstOffsetsChanged();
    updateState();
}

qreal Lr2TimelineState::dstOffsetHiddenA() const {
    return m_dstOffsetHiddenA;
}

void Lr2TimelineState::setDstOffsetHiddenA(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (std::abs(m_dstOffsetHiddenA - value) <= 0.0001) {
        return;
    }

    m_dstOffsetHiddenA = value;
    emit dstOffsetsChanged();
    updateState();
}

QVariant Lr2TimelineState::state() const {
    return stateToVariant(m_state);
}

bool Lr2TimelineState::hasState() const {
    return m_state.valid;
}

qreal Lr2TimelineState::stateX() const {
    return m_state.x;
}

qreal Lr2TimelineState::stateY() const {
    return m_state.y;
}

qreal Lr2TimelineState::stateW() const {
    return m_state.w;
}

qreal Lr2TimelineState::stateH() const {
    return m_state.h;
}

qreal Lr2TimelineState::stateA() const {
    return m_state.a;
}

qreal Lr2TimelineState::stateR() const {
    return m_state.r;
}

qreal Lr2TimelineState::stateG() const {
    return m_state.g;
}

qreal Lr2TimelineState::stateB() const {
    return m_state.b;
}

qreal Lr2TimelineState::stateAngle() const {
    return m_state.angle;
}

int Lr2TimelineState::stateCenter() const {
    return m_state.center;
}

qreal Lr2TimelineState::stateSortId() const {
    return m_state.sortId;
}

int Lr2TimelineState::stateBlend() const {
    return m_state.blend;
}

int Lr2TimelineState::stateFilter() const {
    return m_state.filter;
}

int Lr2TimelineState::stateOp1() const {
    return m_state.op1;
}

int Lr2TimelineState::stateOp2() const {
    return m_state.op2;
}

int Lr2TimelineState::stateOp3() const {
    return m_state.op3;
}

int Lr2TimelineState::stateOp4() const {
    return m_state.op4;
}

bool Lr2TimelineState::canUseStaticState() const {
    return m_canUseStaticState;
}

QVariant Lr2TimelineState::staticState() const {
    if (!m_canUseStaticState || m_dsts.isEmpty()) {
        return {};
    }
    return stateToVariant(copyDstAsState(m_dsts.front(), m_dsts.front()));
}

bool Lr2TimelineState::usesActiveOptions() const {
    return m_usesActiveOptions;
}

bool Lr2TimelineState::usesDynamicTimer() const {
    return m_usesDynamicTimer;
}

bool Lr2TimelineState::loopsContinuously() const {
    return m_loopsContinuously;
}

int Lr2TimelineState::scratchRotationSide() const {
    return m_scratchRotationSide;
}

int Lr2TimelineState::firstTimer() const {
    return m_firstTimer;
}

int Lr2TimelineState::firstSortId() const {
    return m_firstSortId;
}

QVariant Lr2TimelineState::stateFor(const QVariantList& dsts,
                                    int skinTime,
                                    const QVariant& timers,
                                    const QVariant& activeOptions) const {
    const QVector<Dst> parsed = readDsts(dsts);
    if (parsed.isEmpty()) {
        return {};
    }

    Lr2TimelineState cache;
    cache.m_timers = timers;
    cache.m_activeOptions = activeOptions;
    cache.rebuildActiveOptionSet();
    return stateToVariant(currentState(parsed, skinTime, cache.timerValue(parsed.front().timer), cache));
}

QVariant Lr2TimelineState::stateFromTimerFire(const QVariantList& dsts,
                                              int skinTime,
                                              int timerFire,
                                              const QVariant& activeOptions) const {
    const QVector<Dst> parsed = readDsts(dsts);
    if (parsed.isEmpty()) {
        return {};
    }

    Lr2TimelineState cache;
    cache.m_activeOptions = activeOptions;
    cache.rebuildActiveOptionSet();
    return stateToVariant(currentState(parsed, skinTime, timerFire, cache));
}

QVariant Lr2TimelineState::staticStateFor(const QVariantList& dsts) const {
    const QVector<Dst> parsed = readDsts(dsts);
    const DstAnalysis analysis = analyzeDsts(parsed);
    if (!analysis.canUseStaticState || parsed.isEmpty()) {
        return {};
    }
    return stateToVariant(copyDstAsState(parsed.front(), parsed.front()));
}

bool Lr2TimelineState::canUseStaticStateFor(const QVariantList& dsts) const {
    return analyzeDsts(readDsts(dsts)).canUseStaticState;
}

bool Lr2TimelineState::usesActiveOptionsFor(const QVariantList& dsts) const {
    return analyzeDsts(readDsts(dsts)).usesActiveOptions;
}

bool Lr2TimelineState::usesDynamicTimerFor(const QVariantList& dsts) const {
    return analyzeDsts(readDsts(dsts)).usesDynamicTimer;
}

bool Lr2TimelineState::loopsContinuouslyFor(const QVariantList& dsts) const {
    return analyzeDsts(readDsts(dsts)).loopsContinuously;
}

int Lr2TimelineState::scratchRotationSideFor(const QVariantList& dsts) const {
    return analyzeDsts(readDsts(dsts)).scratchRotationSide;
}

int Lr2TimelineState::firstTimerFor(const QVariantList& dsts) const {
    return analyzeDsts(readDsts(dsts)).firstTimer;
}

int Lr2TimelineState::firstSortIdFor(const QVariantList& dsts) const {
    return analyzeDsts(readDsts(dsts)).firstSortId;
}

int Lr2TimelineState::timerFireFor(const QVariant& timers, int timerIdx) const {
    Lr2TimelineState cache;
    cache.m_timers = timers;
    return static_cast<int>(std::floor(cache.timerValue(timerIdx)));
}

int Lr2TimelineState::sourceTimerFor(const QVariant& src) const {
    Source source;
    return readSource(src, source) ? source.timer : 0;
}

bool Lr2TimelineState::sourceUsesDynamicTimer(const QVariant& src) const {
    Source source;
    return readSource(src, source) && source.timer != 0 && source.cycle > 0;
}

bool Lr2TimelineState::sourceCyclesContinuously(const QVariant& src) const {
    Source source;
    return readSource(src, source)
        && source.cycle > 0
        && std::max(1, source.divX) * std::max(1, source.divY) > 1;
}

void Lr2TimelineState::rebuildDsts() {
    m_dsts = readDsts(m_dstsValue);
}

void Lr2TimelineState::rebuildAnalysis() {
    const DstAnalysis analysis = analyzeDsts(m_dsts);

    if (m_canUseStaticState == analysis.canUseStaticState
            && m_usesActiveOptions == analysis.usesActiveOptions
            && m_usesDynamicTimer == analysis.usesDynamicTimer
            && m_loopsContinuously == analysis.loopsContinuously
            && m_scratchRotationSide == analysis.scratchRotationSide
            && m_firstTimer == analysis.firstTimer
            && m_firstSortId == analysis.firstSortId) {
        return;
    }

    m_canUseStaticState = analysis.canUseStaticState;
    m_usesActiveOptions = analysis.usesActiveOptions;
    m_usesDynamicTimer = analysis.usesDynamicTimer;
    m_loopsContinuously = analysis.loopsContinuously;
    m_scratchRotationSide = analysis.scratchRotationSide;
    m_firstTimer = analysis.firstTimer;
    m_firstSortId = analysis.firstSortId;
    emit analysisChanged();
}

void Lr2TimelineState::rebuildActiveOptionSet() {
    m_activeOptionSet.clear();

    const QVariantList list = m_activeOptions.toList();
    if (!list.isEmpty()) {
        for (const QVariant& option : list) {
            bool ok = false;
            const int value = option.toInt(&ok);
            if (ok) {
                m_activeOptionSet.insert(value);
            }
        }
        return;
    }

    if (!m_activeOptions.canConvert<QJSValue>()) {
        return;
    }

    const QJSValue value = m_activeOptions.value<QJSValue>();
    const int length = value.property(QStringLiteral("length")).toInt();
    for (int i = 0; i < length; ++i) {
        const QJSValue option = value.property(static_cast<quint32>(i));
        if (option.isNumber()) {
            m_activeOptionSet.insert(option.toInt());
        }
    }
}

void Lr2TimelineState::reconnectClock() {
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
            &Lr2TimelineState::updateSkinTimeFromClock);
        break;
    case SelectSourceClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::selectSourceSkinTimeChanged,
            this,
            &Lr2TimelineState::updateSkinTimeFromClock);
        break;
    case BarClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::barSkinTimeChanged,
            this,
            &Lr2TimelineState::updateSkinTimeFromClock);
        break;
    case GlobalClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::globalSkinTimeChanged,
            this,
            &Lr2TimelineState::updateSkinTimeFromClock);
        break;
    case SelectLiveClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::selectLiveSkinTimeChanged,
            this,
            &Lr2TimelineState::updateSkinTimeFromClock);
        break;
    case SelectInfoClock:
        m_clockConnection = QObject::connect(
            m_skinClock,
            &Lr2SkinClock::selectInfoElapsedChanged,
            this,
            &Lr2TimelineState::updateSkinTimeFromClock);
        break;
    default:
        break;
    }

    updateSkinTimeFromClock();
}

void Lr2TimelineState::updateSkinTimeFromClock() {
    if (!m_skinClock || m_clockMode == ManualClock) {
        return;
    }

    setSkinTime(clockSkinTime());
}

void Lr2TimelineState::updateAnimationLimit() {
    const int nextLimit = animationLimitFor(m_dsts);
    if (m_animationLimit == nextLimit) {
        m_effectiveSkinTime = effectiveSkinTime(m_requestedSkinTime);
        return;
    }

    m_animationLimit = nextLimit;
    m_effectiveSkinTime = effectiveSkinTime(m_requestedSkinTime);
}

void Lr2TimelineState::updateState() {
    if (!m_enabled) {
        assignState(State {});
        return;
    }

    State state = currentState(m_dsts, m_effectiveSkinTime, effectiveTimerFire(), *this);
    if (m_sliderTranslationEnabled) {
        state = translatedSliderState(state);
    }
    if (m_dstOffsetsEnabled) {
        state = offsetDstState(state);
    }
    assignState(state);
}

int Lr2TimelineState::clockSkinTime() const {
    if (!m_skinClock) {
        return m_requestedSkinTime;
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
        return m_requestedSkinTime;
    }
}

int Lr2TimelineState::effectiveSkinTime(int requestedTime) const {
    return m_animationLimit < 0 ? requestedTime : std::min(requestedTime, m_animationLimit);
}

qreal Lr2TimelineState::effectiveTimerFire() const {
    if (m_timerFire > -2147483648) {
        return m_timerFire;
    }
    if (m_dsts.isEmpty()) {
        return -1.0;
    }
    return timerValue(m_dsts.front().timer);
}

qreal Lr2TimelineState::timerValue(int timerIdx) const {
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

bool Lr2TimelineState::allOpsMatch(const Dst& dst) const {
    return checkSingleOp(dst.op1) && checkSingleOp(dst.op2) && checkSingleOp(dst.op3);
}

bool Lr2TimelineState::checkSingleOp(int op) const {
    if (op == 0) {
        return true;
    }
    const bool negate = op < 0;
    const bool present = m_activeOptionSet.contains(std::abs(op));
    return negate ? !present : present;
}

Lr2TimelineState::State Lr2TimelineState::translatedSliderState(State state) const {
    if (!state.valid) {
        return state;
    }

    const qreal offset = m_sliderPosition * std::max(1, m_sliderRange);
    switch (m_sliderDirection) {
    case 0:
        state.y -= offset;
        break;
    case 1:
        state.x += offset;
        break;
    case 2:
        state.y += offset;
        break;
    case 3:
        state.x -= offset;
        break;
    default:
        state.valid = false;
        break;
    }
    return state;
}

Lr2TimelineState::State Lr2TimelineState::offsetDstState(State state) const {
    if (!state.valid || m_dsts.isEmpty() || m_dsts.front().offsets.isEmpty()) {
        return state;
    }

    for (int offset : m_dsts.front().offsets) {
        switch (offset) {
        case 3:
        case 50:
            state.y += m_dstOffsetLiftY;
            break;
        case 4:
        case 51:
            state.y += m_dstOffsetLaneCoverY;
            break;
        case 5:
            state.y += m_dstOffsetHiddenY;
            state.a += m_dstOffsetHiddenA;
            break;
        default:
            break;
        }
    }
    return state;
}

void Lr2TimelineState::assignState(const State& state) {
    if (sameState(m_state, state)) {
        return;
    }

    m_state = state;
    emit stateChanged();
}

bool Lr2TimelineState::readDst(const QVariant& value, Dst& dst) {
    if (value.canConvert<Lr2Dst>()) {
        const auto parsed = value.value<Lr2Dst>();
        dst.valid = true;
        dst.time = parsed.time;
        dst.x = parsed.x;
        dst.y = parsed.y;
        dst.w = parsed.w;
        dst.h = parsed.h;
        dst.acc = parsed.acc;
        dst.a = parsed.a;
        dst.r = parsed.r;
        dst.g = parsed.g;
        dst.b = parsed.b;
        dst.blend = parsed.blend;
        dst.filter = parsed.filter;
        dst.angle = parsed.angle;
        dst.center = parsed.center;
        dst.sortId = parsed.sortId;
        dst.loop = parsed.loop;
        dst.timer = parsed.timer;
        dst.op1 = parsed.op1;
        dst.op2 = parsed.op2;
        dst.op3 = parsed.op3;
        dst.op4 = parsed.op4;
        dst.offsets = readOffsets(parsed.offsets);
        return true;
    }

    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        dst.valid = true;
        dst.time = mapInt(map, QStringLiteral("time"), 0);
        dst.x = mapInt(map, QStringLiteral("x"), 0);
        dst.y = mapInt(map, QStringLiteral("y"), 0);
        dst.w = mapInt(map, QStringLiteral("w"), 0);
        dst.h = mapInt(map, QStringLiteral("h"), 0);
        dst.acc = mapInt(map, QStringLiteral("acc"), 0);
        dst.a = mapInt(map, QStringLiteral("a"), 255);
        dst.r = mapInt(map, QStringLiteral("r"), 255);
        dst.g = mapInt(map, QStringLiteral("g"), 255);
        dst.b = mapInt(map, QStringLiteral("b"), 255);
        dst.blend = mapInt(map, QStringLiteral("blend"), 0);
        dst.filter = mapInt(map, QStringLiteral("filter"), 0);
        dst.angle = mapInt(map, QStringLiteral("angle"), 0);
        dst.center = mapInt(map, QStringLiteral("center"), 0);
        dst.sortId = mapInt(map, QStringLiteral("sortId"), 0);
        dst.loop = mapInt(map, QStringLiteral("loop"), 0);
        dst.timer = mapInt(map, QStringLiteral("timer"), 0);
        dst.op1 = mapInt(map, QStringLiteral("op1"), 0);
        dst.op2 = mapInt(map, QStringLiteral("op2"), 0);
        dst.op3 = mapInt(map, QStringLiteral("op3"), 0);
        dst.op4 = mapInt(map, QStringLiteral("op4"), 0);
        const auto offsetsIt = map.constFind(QStringLiteral("offsets"));
        if (offsetsIt != map.constEnd()) {
            dst.offsets = readOffsets(*offsetsIt);
        }
        return true;
    }

    if (!value.canConvert<QJSValue>()) {
        return false;
    }

    const QJSValue jsValue = value.value<QJSValue>();
    if (!jsValue.isObject()) {
        return false;
    }

    dst.valid = true;
    dst.time = jsInt(jsValue, QStringLiteral("time"), 0);
    dst.x = jsInt(jsValue, QStringLiteral("x"), 0);
    dst.y = jsInt(jsValue, QStringLiteral("y"), 0);
    dst.w = jsInt(jsValue, QStringLiteral("w"), 0);
    dst.h = jsInt(jsValue, QStringLiteral("h"), 0);
    dst.acc = jsInt(jsValue, QStringLiteral("acc"), 0);
    dst.a = jsInt(jsValue, QStringLiteral("a"), 255);
    dst.r = jsInt(jsValue, QStringLiteral("r"), 255);
    dst.g = jsInt(jsValue, QStringLiteral("g"), 255);
    dst.b = jsInt(jsValue, QStringLiteral("b"), 255);
    dst.blend = jsInt(jsValue, QStringLiteral("blend"), 0);
    dst.filter = jsInt(jsValue, QStringLiteral("filter"), 0);
    dst.angle = jsInt(jsValue, QStringLiteral("angle"), 0);
    dst.center = jsInt(jsValue, QStringLiteral("center"), 0);
    dst.sortId = jsInt(jsValue, QStringLiteral("sortId"), 0);
    dst.loop = jsInt(jsValue, QStringLiteral("loop"), 0);
    dst.timer = jsInt(jsValue, QStringLiteral("timer"), 0);
    dst.op1 = jsInt(jsValue, QStringLiteral("op1"), 0);
    dst.op2 = jsInt(jsValue, QStringLiteral("op2"), 0);
    dst.op3 = jsInt(jsValue, QStringLiteral("op3"), 0);
    dst.op4 = jsInt(jsValue, QStringLiteral("op4"), 0);
    dst.offsets = readOffsets(QVariant::fromValue(jsValue.property(QStringLiteral("offsets"))));
    return true;
}

QVector<Lr2TimelineState::Dst> Lr2TimelineState::readDsts(const QVariantList& dsts) {
    QVector<Dst> result;
    result.reserve(dsts.size());

    for (const QVariant& entry : dsts) {
        Dst dst;
        if (readDst(entry, dst)) {
            result.append(dst);
        }
    }
    return result;
}

bool Lr2TimelineState::readSource(const QVariant& value, Source& source) {
    if (!value.isValid() || value.isNull()) {
        return false;
    }

    if (value.canConvert<Lr2SrcImage>()) {
        const auto parsed = value.value<Lr2SrcImage>();
        source.valid = true;
        source.divX = std::max(1, parsed.div_x);
        source.divY = std::max(1, parsed.div_y);
        source.cycle = parsed.cycle;
        source.timer = parsed.timer;
        return true;
    }

    if (value.canConvert<Lr2SrcNumber>()) {
        const auto parsed = value.value<Lr2SrcNumber>();
        source.valid = true;
        source.divX = std::max(1, parsed.div_x);
        source.divY = std::max(1, parsed.div_y);
        source.cycle = parsed.cycle;
        source.timer = parsed.timer;
        return true;
    }

    if (value.canConvert<Lr2SrcBarGraph>()) {
        const auto parsed = value.value<Lr2SrcBarGraph>();
        source.valid = true;
        source.divX = std::max(1, parsed.div_x);
        source.divY = std::max(1, parsed.div_y);
        source.cycle = parsed.cycle;
        source.timer = parsed.timer;
        return true;
    }

    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        source.valid = true;
        source.divX = std::max(1, mapInt(map, QStringLiteral("div_x"), 1));
        source.divY = std::max(1, mapInt(map, QStringLiteral("div_y"), 1));
        source.cycle = mapInt(map, QStringLiteral("cycle"), 0);
        source.timer = mapInt(map, QStringLiteral("timer"), 0);
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
    return true;
}

QVector<int> Lr2TimelineState::readOffsets(const QVariant& value) {
    QVector<int> offsets;

    const QVariantList list = value.toList();
    if (!list.isEmpty()) {
        offsets.reserve(list.size());
        for (const QVariant& entry : list) {
            bool ok = false;
            const int offset = entry.toInt(&ok);
            if (ok) {
                offsets.append(offset);
            }
        }
        return offsets;
    }

    if (!value.canConvert<QJSValue>()) {
        return offsets;
    }

    const QJSValue jsValue = value.value<QJSValue>();
    if (!jsValue.isArray()) {
        return offsets;
    }
    const int length = jsValue.property(QStringLiteral("length")).toInt();
    offsets.reserve(length);
    for (int i = 0; i < length; ++i) {
        const QJSValue entry = jsValue.property(static_cast<quint32>(i));
        if (entry.isNumber()) {
            offsets.append(entry.toInt());
        }
    }
    return offsets;
}

Lr2TimelineState::DstAnalysis Lr2TimelineState::analyzeDsts(const QVector<Dst>& dsts) {
    DstAnalysis analysis;
    if (dsts.isEmpty() || !dsts.front().valid) {
        return analysis;
    }

    const Dst& first = dsts.front();
    analysis.firstTimer = first.timer;
    analysis.firstSortId = first.sortId;
    analysis.usesDynamicTimer = first.timer != 0;
    analysis.usesActiveOptions = first.op1 != 0 || first.op2 != 0 || first.op3 != 0;
    analysis.scratchRotationSide = first.op4 == 1 || first.op4 == 2 ? first.op4 : 0;
    analysis.canUseStaticState = dsts.size() == 1
        && first.time <= 0
        && first.timer == 0
        && first.op1 == 0
        && first.op2 == 0
        && first.op3 == 0;

    if (dsts.size() >= 2) {
        const Dst& last = dsts.back();
        analysis.loopsContinuously = first.loop >= 0 && first.loop < last.time;
    }
    return analysis;
}

int Lr2TimelineState::animationLimitFor(const QVector<Dst>& dsts) {
    if (dsts.isEmpty() || !dsts.front().valid) {
        return 0;
    }

    const Dst& first = dsts.front();
    if (first.timer != 0) {
        return -1;
    }

    const int endTime = dsts.back().time;
    if (dsts.size() == 1) {
        return first.loop < 0 ? endTime + 1 : first.time;
    }

    if (first.loop < 0 || first.loop > endTime) {
        return endTime + 1;
    }
    if (first.loop == endTime) {
        return endTime;
    }
    return -1;
}

Lr2TimelineState::State Lr2TimelineState::currentState(const QVector<Dst>& dsts,
                                                       int globalTime,
                                                       qreal timerFire,
                                                       const Lr2TimelineState& cache) {
    if (dsts.isEmpty() || !dsts.front().valid) {
        return {};
    }

    const Dst& first = dsts.front();
    if (!cache.allOpsMatch(first) || timerFire < 0.0) {
        return {};
    }

    int time = globalTime - static_cast<int>(std::floor(timerFire));
    if (time < first.time) {
        return {};
    }

    const Dst& last = dsts.back();
    const int endTime = last.time;
    const int loopTo = first.loop;

    if (dsts.size() == 1) {
        if (loopTo < 0 && time > endTime) {
            return {};
        }
        return copyDstAsState(first, first);
    }

    if (time > endTime) {
        if (loopTo < 0) {
            return {};
        }
        if (loopTo == endTime) {
            time = endTime;
        } else if (loopTo > endTime) {
            time = 0;
        } else {
            const int period = endTime - loopTo;
            if (period <= 0) {
                return {};
            }
            time = loopTo + ((time - loopTo) % period);
        }
    }

    if (time >= endTime) {
        return copyDstAsState(last, first);
    }

    const Dst* d1 = &dsts.front();
    const Dst* d2 = &dsts.front();
    for (qsizetype i = 0; i < dsts.size() - 1; ++i) {
        if (time >= dsts[i].time && time < dsts[i + 1].time) {
            d1 = &dsts[i];
            d2 = &dsts[i + 1];
            break;
        }
    }

    const int segment = d2->time - d1->time;
    if (segment <= 0) {
        return copyDstAsState(*d1, first);
    }

    const qreal progress = applyAccel(static_cast<qreal>(time - d1->time) / segment, d1->acc);
    const auto mix = [progress](qreal a, qreal b) {
        return a + (b - a) * progress;
    };

    State state;
    state.valid = true;
    state.x = mix(d1->x, d2->x);
    state.y = mix(d1->y, d2->y);
    state.w = mix(d1->w, d2->w);
    state.h = mix(d1->h, d2->h);
    state.a = mix(d1->a, d2->a);
    state.r = mix(d1->r, d2->r);
    state.g = mix(d1->g, d2->g);
    state.b = mix(d1->b, d2->b);
    state.angle = mix(d1->angle, d2->angle);
    state.center = d1->center;
    state.sortId = mix(d1->sortId, d2->sortId);
    state.blend = d1->blend;
    state.filter = d1->filter;
    state.op1 = first.op1;
    state.op2 = first.op2;
    state.op3 = first.op3;
    state.op4 = first.op4;
    return state;
}

Lr2TimelineState::State Lr2TimelineState::copyDstAsState(const Dst& dst, const Dst& controlDst) {
    State state;
    state.valid = true;
    state.x = dst.x;
    state.y = dst.y;
    state.w = dst.w;
    state.h = dst.h;
    state.a = dst.a;
    state.r = dst.r;
    state.g = dst.g;
    state.b = dst.b;
    state.angle = dst.angle;
    state.center = dst.center;
    state.sortId = dst.sortId;
    state.blend = dst.blend;
    state.filter = dst.filter;
    state.op1 = controlDst.op1;
    state.op2 = controlDst.op2;
    state.op3 = controlDst.op3;
    state.op4 = controlDst.op4;
    return state;
}

QVariant Lr2TimelineState::stateToVariant(const State& state) {
    if (!state.valid) {
        return {};
    }

    QVariantMap value;
    value.insert(QStringLiteral("x"), state.x);
    value.insert(QStringLiteral("y"), state.y);
    value.insert(QStringLiteral("w"), state.w);
    value.insert(QStringLiteral("h"), state.h);
    value.insert(QStringLiteral("a"), state.a);
    value.insert(QStringLiteral("r"), state.r);
    value.insert(QStringLiteral("g"), state.g);
    value.insert(QStringLiteral("b"), state.b);
    value.insert(QStringLiteral("angle"), state.angle);
    value.insert(QStringLiteral("center"), state.center);
    value.insert(QStringLiteral("sortId"), state.sortId);
    value.insert(QStringLiteral("blend"), state.blend);
    value.insert(QStringLiteral("filter"), state.filter);
    value.insert(QStringLiteral("op1"), state.op1);
    value.insert(QStringLiteral("op2"), state.op2);
    value.insert(QStringLiteral("op3"), state.op3);
    value.insert(QStringLiteral("op4"), state.op4);
    return value;
}

qreal Lr2TimelineState::applyAccel(qreal progress, int accType) {
    if (accType == 1) {
        return progress * progress;
    }
    if (accType == 2) {
        const qreal inv = 1.0 - progress;
        return 1.0 - inv * inv;
    }
    if (accType == 3) {
        return 0.0;
    }
    return progress;
}

bool Lr2TimelineState::sameState(const State& lhs, const State& rhs) {
    const auto sameReal = [](qreal a, qreal b) {
        return std::abs(a - b) <= 0.0001;
    };

    return lhs.valid == rhs.valid
        && sameReal(lhs.x, rhs.x)
        && sameReal(lhs.y, rhs.y)
        && sameReal(lhs.w, rhs.w)
        && sameReal(lhs.h, rhs.h)
        && sameReal(lhs.a, rhs.a)
        && sameReal(lhs.r, rhs.r)
        && sameReal(lhs.g, rhs.g)
        && sameReal(lhs.b, rhs.b)
        && sameReal(lhs.angle, rhs.angle)
        && lhs.center == rhs.center
        && sameReal(lhs.sortId, rhs.sortId)
        && lhs.blend == rhs.blend
        && lhs.filter == rhs.filter
        && lhs.op1 == rhs.op1
        && lhs.op2 == rhs.op2
        && lhs.op3 == rhs.op3
        && lhs.op4 == rhs.op4;
}
