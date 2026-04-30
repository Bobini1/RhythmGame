#include "Lr2TimelineState.h"

#include "Lr2SkinClock.h"
#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include <QJSValue>
#include <QVariantMap>
#include <algorithm>
#include <cmath>

using gameplay_logic::lr2_skin::Lr2Dst;

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
    mode = std::clamp(mode, static_cast<int>(ManualClock), static_cast<int>(SelectLiveClock));
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

QVariant Lr2TimelineState::state() const {
    if (!m_state.valid) {
        return {};
    }

    QVariantMap state;
    state.insert(QStringLiteral("x"), m_state.x);
    state.insert(QStringLiteral("y"), m_state.y);
    state.insert(QStringLiteral("w"), m_state.w);
    state.insert(QStringLiteral("h"), m_state.h);
    state.insert(QStringLiteral("a"), m_state.a);
    state.insert(QStringLiteral("r"), m_state.r);
    state.insert(QStringLiteral("g"), m_state.g);
    state.insert(QStringLiteral("b"), m_state.b);
    state.insert(QStringLiteral("angle"), m_state.angle);
    state.insert(QStringLiteral("center"), m_state.center);
    state.insert(QStringLiteral("sortId"), m_state.sortId);
    state.insert(QStringLiteral("blend"), m_state.blend);
    state.insert(QStringLiteral("filter"), m_state.filter);
    state.insert(QStringLiteral("op1"), m_state.op1);
    state.insert(QStringLiteral("op2"), m_state.op2);
    state.insert(QStringLiteral("op3"), m_state.op3);
    state.insert(QStringLiteral("op4"), m_state.op4);
    return state;
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

void Lr2TimelineState::rebuildDsts() {
    m_dsts.clear();
    m_dsts.reserve(m_dstsValue.size());

    for (const QVariant& entry : m_dstsValue) {
        Dst dst;
        if (readDst(entry, dst)) {
            m_dsts.append(dst);
        }
    }
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

    assignState(currentState(m_dsts, m_effectiveSkinTime, effectiveTimerFire(), *this));
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
    return true;
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
