#include "Lr2BarBaseStateResolver.h"

#include <QVariantMap>
#include <algorithm>
#include <cmath>
#include <utility>

namespace rt = lr2skin::runtime;

namespace {
bool sameStateNumber(qreal lhs, qreal rhs) {
    return std::abs(lhs - rhs) <= 0.001;
}

QVariant normalizedVariant(const QVariant& value);

QVariantList normalizedList(const QVariantList& values) {
    QVariantList result;
    result.reserve(values.size());
    for (const QVariant& value : values) {
        result.append(normalizedVariant(value));
    }
    return result;
}

QVariantMap normalizedMap(const QVariantMap& values) {
    QVariantMap result;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        result.insert(it.key(), normalizedVariant(it.value()));
    }
    return result;
}

QVariant normalizedVariant(const QVariant& value) {
    if (value.canConvert<QVariantList>()) {
        return normalizedList(value.toList());
    }
    if (value.canConvert<QVariantMap>()) {
        return normalizedMap(value.toMap());
    }
    return value;
}
} // namespace

Lr2BarBaseStateResolver::Lr2BarBaseStateResolver(QObject* parent) : QObject(parent) {}

QVariantList Lr2BarBaseStateResolver::barRows() const {
    return m_barRows;
}

void Lr2BarBaseStateResolver::setBarRows(const QVariantList& rows) {
    const QVariantList nextRows = normalizedList(rows);
    if (m_barRows == nextRows) {
        return;
    }

    m_barRows = nextRows;
    rebuildRows();
    updateAnimationLimit();
    emit barRowsChanged();
    rebuildBaseStates();
}

int Lr2BarBaseStateResolver::selectedRow() const {
    return m_selectedRow;
}

void Lr2BarBaseStateResolver::setSelectedRow(int row) {
    if (m_selectedRow == row) {
        return;
    }

    m_selectedRow = row;
    emit selectedRowChanged();
    rebuildBaseStates();
}

int Lr2BarBaseStateResolver::skinTime() const {
    return m_requestedSkinTime;
}

void Lr2BarBaseStateResolver::setSkinTime(int time) {
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
        rebuildBaseStates();
    }
}

QVariant Lr2BarBaseStateResolver::timers() const {
    return m_timers;
}

void Lr2BarBaseStateResolver::setTimers(const QVariant& timers) {
    const QVariant nextTimers = normalizedVariant(timers);
    if (m_timers == nextTimers) {
        return;
    }

    m_timers = nextTimers;
    emit timersChanged();
    rebuildBaseStates();
}

QVariant Lr2BarBaseStateResolver::activeOptions() const {
    return m_activeOptions;
}

void Lr2BarBaseStateResolver::setActiveOptions(const QVariant& options) {
    const QVariant nextOptions = normalizedVariant(options);
    if (m_activeOptions == nextOptions) {
        return;
    }

    m_activeOptions = nextOptions;
    rebuildActiveOptionSet();
    emit activeOptionsChanged();
    rebuildBaseStates();
}

QVariantList Lr2BarBaseStateResolver::baseStates() const {
    QVariantList result;
    result.reserve(m_baseStates.size());
    for (const State& state : m_baseStates) {
        result.append(QVariant::fromValue(state));
    }
    return result;
}

QVariantList Lr2BarBaseStateResolver::positionlessBaseStates() const {
    QVariantList result;
    result.reserve(m_baseStates.size());
    for (const State& state : m_baseStates) {
        if (!state.valid) {
            result.append(QVariant::fromValue(State {}));
            continue;
        }

        State positionless = state;
        positionless.x = 0.0;
        positionless.y = 0.0;
        result.append(QVariant::fromValue(positionless));
    }
    return result;
}

QVariantList Lr2BarBaseStateResolver::interpolationNeededByRow() const {
    QVariantList result;
    result.reserve(m_baseStates.size());
    for (int row = 0; row < m_baseStates.size(); ++row) {
        result.append(stateNeedsInterpolationAt(row));
    }
    return result;
}

int Lr2BarBaseStateResolver::animationLimit() const {
    return m_animationLimit;
}

bool Lr2BarBaseStateResolver::fastScrollActive() const {
    return m_fastScrollActive;
}

qreal Lr2BarBaseStateResolver::fastScrollDx() const {
    return m_fastScrollDx;
}

qreal Lr2BarBaseStateResolver::fastScrollDy() const {
    return m_fastScrollDy;
}

int Lr2BarBaseStateResolver::stateCount() const {
    return m_baseStates.size();
}

Lr2TimelineStateValue Lr2BarBaseStateResolver::stateAt(int row) const {
    return stateValueAt(row);
}

Lr2TimelineStateValue Lr2BarBaseStateResolver::stateValueAt(int row) const {
    return row >= 0 && row < m_baseStates.size() ? m_baseStates.at(row) : Lr2TimelineStateValue {};
}

bool Lr2BarBaseStateResolver::stateNeedsInterpolationAt(int row) const {
    if (row <= 0 || row >= m_baseStates.size()) {
        return false;
    }

    const State& fromState = m_baseStates.at(row);
    const State& toState = m_baseStates.at(row - 1);
    if (!fromState.valid || !toState.valid) {
        return false;
    }

    return !sameStateNumber(fromState.w, toState.w)
        || !sameStateNumber(fromState.h, toState.h)
        || !sameStateNumber(fromState.a, toState.a)
        || !sameStateNumber(fromState.r, toState.r)
        || !sameStateNumber(fromState.g, toState.g)
        || !sameStateNumber(fromState.b, toState.b)
        || !sameStateNumber(fromState.angle, toState.angle)
        || !sameStateNumber(fromState.center, toState.center)
        || fromState.blend != toState.blend
        || fromState.filter != toState.filter
        || fromState.op4 != toState.op4;
}

Lr2TimelineStateValue Lr2BarBaseStateResolver::positionlessStateAt(int row) const {
    if (row < 0 || row >= m_baseStates.size()) {
        return {};
    }

    State state = m_baseStates.at(row);
    if (!state.valid) {
        return {};
    }

    state.x = 0.0;
    state.y = 0.0;
    return state;
}

bool Lr2BarBaseStateResolver::stateValidAt(int row) const {
    return row >= 0 && row < m_baseStates.size() && m_baseStates.at(row).valid;
}

qreal Lr2BarBaseStateResolver::stateXAt(int row) const {
    return row >= 0 && row < m_baseStates.size() ? m_baseStates.at(row).x : 0.0;
}

qreal Lr2BarBaseStateResolver::stateYAt(int row) const {
    return row >= 0 && row < m_baseStates.size() ? m_baseStates.at(row).y : 0.0;
}

void Lr2BarBaseStateResolver::rebuildRows() {
    m_rows.clear();
    m_rows.reserve(m_barRows.size());

    for (const QVariant& rowValue : m_barRows) {
        Row row;
        row.offDsts = rt::readDsts(rowField(rowValue, QStringLiteral("offDsts")));
        row.onDsts = rt::readDsts(rowField(rowValue, QStringLiteral("onDsts")));
        m_rows.append(row);
    }
}

void Lr2BarBaseStateResolver::rebuildActiveOptionSet() {
    m_activeOptionSet = rt::activeOptionSet(m_activeOptions);
}

void Lr2BarBaseStateResolver::rebuildBaseStates() {
    QVector<State> next;
    next.reserve(m_rows.size());

    for (qsizetype rowIndex = 0; rowIndex < m_rows.size(); ++rowIndex) {
        const Row& row = m_rows[rowIndex];
        const bool useOn = rowIndex == m_selectedRow && !row.onDsts.isEmpty();
        const QVector<Dst>& primary = useOn ? row.onDsts : row.offDsts;
        const QVector<Dst>& dsts = primary.isEmpty() ? row.onDsts : primary;
        const Dst* first = dsts.isEmpty() ? nullptr : &dsts.front();
        next.append(rt::currentState(
            dsts,
            m_effectiveSkinTime,
            first ? timerFire(first->timer) : -1.0,
            m_activeOptionSet));
    }

    const bool statesChanged = !statesEqual(m_baseStates, next);
    const bool wasFastScrollActive = m_fastScrollActive;
    const qreal oldFastScrollDx = m_fastScrollDx;
    const qreal oldFastScrollDy = m_fastScrollDy;
    if (statesChanged) {
        m_baseStates = std::move(next);
    }
    rebuildFastScrollStep();
    if (!statesChanged
            && wasFastScrollActive == m_fastScrollActive
            && std::abs(oldFastScrollDx - m_fastScrollDx) <= 0.000001
            && std::abs(oldFastScrollDy - m_fastScrollDy) <= 0.000001) {
        return;
    }

    emit baseStatesChanged();
}

void Lr2BarBaseStateResolver::rebuildFastScrollStep() {
    m_fastScrollActive = false;
    m_fastScrollDx = 0.0;
    m_fastScrollDy = 0.0;

    if (m_baseStates.size() < 2) {
        return;
    }

    bool haveStep = false;
    qreal dx = 0.0;
    qreal dy = 0.0;
    for (int row = 1; row < m_baseStates.size(); ++row) {
        const State& previous = m_baseStates.at(row - 1);
        const State& current = m_baseStates.at(row);
        if (!previous.valid || !current.valid) {
            continue;
        }

        const qreal nextDx = current.x - previous.x;
        const qreal nextDy = current.y - previous.y;
        if (!haveStep) {
            dx = nextDx;
            dy = nextDy;
            haveStep = true;
        } else if (std::abs(dx - nextDx) > 0.001 || std::abs(dy - nextDy) > 0.001) {
            return;
        }

        if (row != m_selectedRow
                && row - 1 != m_selectedRow
                && (!sameStateNumber(previous.w, current.w)
                    || !sameStateNumber(previous.h, current.h)
                    || !sameStateNumber(previous.a, current.a)
                    || !sameStateNumber(previous.r, current.r)
                    || !sameStateNumber(previous.g, current.g)
                    || !sameStateNumber(previous.b, current.b)
                    || !sameStateNumber(previous.angle, current.angle)
                    || !sameStateNumber(previous.center, current.center)
                    || previous.blend != current.blend
                    || previous.filter != current.filter
                    || previous.op4 != current.op4)) {
            return;
        }
    }

    m_fastScrollActive = haveStep && (std::abs(dx) > 0.001 || std::abs(dy) > 0.001);
    m_fastScrollDx = m_fastScrollActive ? dx : 0.0;
    m_fastScrollDy = m_fastScrollActive ? dy : 0.0;
}

void Lr2BarBaseStateResolver::updateAnimationLimit() {
    int nextLimit = 0;
    bool live = false;

    for (const Row& row : m_rows) {
        const int offEnd = rt::animationLimitFor(row.offDsts);
        const int onEnd = rt::animationLimitFor(row.onDsts);
        if (offEnd < 0 || onEnd < 0) {
            live = true;
            break;
        }
        nextLimit = std::max({ nextLimit, offEnd, onEnd });
    }

    if (live) {
        nextLimit = -1;
    }

    if (m_animationLimit == nextLimit) {
        m_effectiveSkinTime = effectiveSkinTime(m_requestedSkinTime);
        return;
    }

    m_animationLimit = nextLimit;
    m_effectiveSkinTime = effectiveSkinTime(m_requestedSkinTime);
    emit animationLimitChanged();
}

int Lr2BarBaseStateResolver::effectiveSkinTime(int requestedTime) const {
    return m_animationLimit < 0 ? requestedTime : std::min(requestedTime, m_animationLimit);
}

qreal Lr2BarBaseStateResolver::timerFire(int timerIdx) const {
    return rt::timerValue(m_timers, timerIdx);
}

QVariant Lr2BarBaseStateResolver::rowField(const QVariant& row, const QString& name) {
    if (row.canConvert<QVariantMap>()) {
        return row.toMap().value(name);
    }
    return {};
}

bool Lr2BarBaseStateResolver::statesEqual(const QVector<State>& lhs, const QVector<State>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (qsizetype i = 0; i < lhs.size(); ++i) {
        if (!rt::sameState(lhs.at(i), rhs.at(i))) {
            return false;
        }
    }
    return true;
}
