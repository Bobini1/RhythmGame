#include "Lr2TimelineState.h"

#include "Lr2SkinClock.h"
#include "Lr2SkinElementActiveOptionsState.h"

#include <algorithm>
#include <cmath>

namespace rt = lr2skin::runtime;

Lr2TimelineState::Lr2TimelineState(QObject* parent) : QObject(parent) {}

Lr2SkinClock* Lr2TimelineState::skinClock() const {
    return m_skinClock;
}

void Lr2TimelineState::setSkinClock(Lr2SkinClock* clock) {
    if (m_skinClock == clock) {
        return;
    }

    m_skinClock = clock;
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

Lr2SkinElementActiveOptionsState* Lr2TimelineState::activeOptionsState() const {
    return m_activeOptionsState;
}

void Lr2TimelineState::setActiveOptionsState(Lr2SkinElementActiveOptionsState* state) {
    if (m_activeOptionsState == state) {
        return;
    }

    m_activeOptionsState = state;
    reconnectActiveOptionsState();
    emit activeOptionsStateChanged();
    activeOptionsStateDidChange();
}

QVariant Lr2TimelineState::activeOptions() const {
    if (m_activeOptionsState) {
        return m_activeOptionsState->activeOptions();
    }
    return m_activeOptions;
}

void Lr2TimelineState::setActiveOptions(const QVariant& options) {
    if (m_activeOptions == options) {
        return;
    }

    m_activeOptions = options;
    emit activeOptionsChanged();
    if (!m_activeOptionsState) {
        rebuildActiveOptionSet();
        emit analysisChanged();
        updateState();
    }
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

Lr2TimelineStateValue Lr2TimelineState::state() const {
    return m_state;
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

Lr2TimelineStateValue Lr2TimelineState::staticState() const {
    if (m_activeOptionsState && !m_activeOptionsState->isActive()) {
        return {};
    }
    if (!m_canUseStaticState || m_dsts.isEmpty() || !rt::allOpsMatch(m_dsts.front(), m_activeOptionSet)) {
        return {};
    }
    return rt::copyDstAsState(m_dsts.front(), m_dsts.front());
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

Lr2TimelineStateValue Lr2TimelineState::stateFor(const QVariantList& dsts,
                                                 int skinTime,
                                                 const QVariant& timers,
                                                 const QVariant& activeOptions) const {
    const QVector<Dst> parsed = rt::readDsts(dsts);
    if (parsed.isEmpty()) {
        return {};
    }

    Lr2TimelineState cache;
    cache.m_timers = timers;
    cache.m_activeOptions = activeOptions;
    cache.rebuildActiveOptionSet();
    return rt::currentState(parsed,
                            skinTime,
                            cache.timerValue(parsed.front().timer),
                            cache.m_activeOptionSet);
}

Lr2TimelineStateValue Lr2TimelineState::stateFromTimerFire(const QVariantList& dsts,
                                                           int skinTime,
                                                           int timerFire,
                                                           const QVariant& activeOptions) const {
    const QVector<Dst> parsed = rt::readDsts(dsts);
    if (parsed.isEmpty()) {
        return {};
    }

    Lr2TimelineState cache;
    cache.m_activeOptions = activeOptions;
    cache.rebuildActiveOptionSet();
    return rt::currentState(parsed, skinTime, timerFire, cache.m_activeOptionSet);
}

Lr2TimelineStateValue Lr2TimelineState::staticStateFor(const QVariantList& dsts) const {
    const QVector<Dst> parsed = rt::readDsts(dsts);
    const rt::DstAnalysis analysis = rt::analyzeDsts(parsed);
    if (!analysis.canUseStaticState || parsed.isEmpty()) {
        return {};
    }
    return rt::copyDstAsState(parsed.front(), parsed.front());
}

bool Lr2TimelineState::canUseStaticStateFor(const QVariantList& dsts) const {
    return rt::analyzeDsts(rt::readDsts(dsts)).canUseStaticState;
}

bool Lr2TimelineState::usesActiveOptionsFor(const QVariantList& dsts) const {
    return rt::analyzeDsts(rt::readDsts(dsts)).usesActiveOptions;
}

bool Lr2TimelineState::usesDynamicTimerFor(const QVariantList& dsts) const {
    return rt::analyzeDsts(rt::readDsts(dsts)).usesDynamicTimer;
}

bool Lr2TimelineState::loopsContinuouslyFor(const QVariantList& dsts) const {
    return rt::analyzeDsts(rt::readDsts(dsts)).loopsContinuously;
}

int Lr2TimelineState::scratchRotationSideFor(const QVariantList& dsts) const {
    return rt::analyzeDsts(rt::readDsts(dsts)).scratchRotationSide;
}

int Lr2TimelineState::firstTimerFor(const QVariantList& dsts) const {
    return rt::analyzeDsts(rt::readDsts(dsts)).firstTimer;
}

int Lr2TimelineState::firstSortIdFor(const QVariantList& dsts) const {
    return rt::analyzeDsts(rt::readDsts(dsts)).firstSortId;
}

int Lr2TimelineState::timerFireFor(const QVariant& timers, int timerIdx) const {
    Lr2TimelineState cache;
    cache.m_timers = timers;
    return static_cast<int>(std::floor(cache.timerValue(timerIdx)));
}

int Lr2TimelineState::sourceTimerFor(const QVariant& src) const {
    rt::Source source;
    return rt::readSource(src, source) ? source.timer : 0;
}

bool Lr2TimelineState::sourceUsesDynamicTimer(const QVariant& src) const {
    rt::Source source;
    return rt::readSource(src, source) && rt::sourceUsesDynamicTimer(source);
}

bool Lr2TimelineState::sourceCyclesContinuously(const QVariant& src) const {
    rt::Source source;
    return rt::readSource(src, source) && rt::sourceCyclesContinuously(source);
}

void Lr2TimelineState::rebuildDsts() {
    m_dsts = rt::readDsts(m_dstsValue);
}

void Lr2TimelineState::rebuildAnalysis() {
    const rt::DstAnalysis analysis = rt::analyzeDsts(m_dsts);

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
    if (m_activeOptionsState) {
        m_activeOptionSet = m_activeOptionsState->activeOptionSet();
        return;
    }

    m_activeOptionSet = rt::activeOptionSet(m_activeOptions);
}

void Lr2TimelineState::reconnectActiveOptionsState() {
    if (m_activeOptionsStateConnection) {
        QObject::disconnect(m_activeOptionsStateConnection);
        m_activeOptionsStateConnection = {};
    }

    if (m_activeOptionsState) {
        m_activeOptionsStateConnection = QObject::connect(
            m_activeOptionsState,
            &Lr2SkinElementActiveOptionsState::activeOptionsChanged,
            this,
            &Lr2TimelineState::activeOptionsStateDidChange);
    }
}

void Lr2TimelineState::activeOptionsStateDidChange() {
    rebuildActiveOptionSet();
    emit activeOptionsChanged();
    emit analysisChanged();
    updateState();
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
    const int nextLimit = rt::animationLimitFor(m_dsts);
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
    if (m_activeOptionsState && !m_activeOptionsState->isActive()) {
        assignState(State {});
        return;
    }

    State state = rt::currentState(m_dsts,
                                   m_effectiveSkinTime,
                                   effectiveTimerFire(),
                                   m_activeOptionSet);
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
    return rt::timerValue(m_timers, timerIdx);
}

Lr2TimelineState::State Lr2TimelineState::translatedSliderState(State state) const {
    return rt::translatedSliderState(state, m_sliderPosition, m_sliderRange, m_sliderDirection);
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
    const bool hadState = m_state.valid;
    if (rt::sameState(m_state, state)) {
        return;
    }

    m_state = state;
    emit stateChanged();
    if (hadState != m_state.valid) {
        emit hasStateChanged();
    }
}
