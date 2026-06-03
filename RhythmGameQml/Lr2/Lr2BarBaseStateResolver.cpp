#include "Lr2BarBaseStateResolver.h"

#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include <QVariantMap>
#include <algorithm>
#include <cmath>
#include <utility>

using gameplay_logic::lr2_skin::Lr2Dst;

namespace {
int mapInt(const QVariantMap& map, const QString& name, int fallback) {
    const auto it = map.constFind(name);
    return it == map.constEnd() || !it->isValid() || it->isNull()
        ? fallback
        : it->toInt();
}

bool mapBool(const QVariantMap& map, const QString& name, bool fallback) {
    const auto it = map.constFind(name);
    return it == map.constEnd() || !it->isValid() || it->isNull()
        ? fallback
        : it->toBool();
}

bool mapHas(const QVariantMap& map, const QString& name) {
    const auto it = map.constFind(name);
    return it != map.constEnd() && it->isValid() && !it->isNull();
}

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
        result.append(stateToVariant(state));
    }
    return result;
}

int Lr2BarBaseStateResolver::baseStatesRevision() const {
    return m_baseStatesRevision;
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

QVariant Lr2BarBaseStateResolver::stateAt(int row) const {
    return row >= 0 && row < m_baseStates.size() ? stateToVariant(m_baseStates.at(row)) : QVariant();
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

QVariant Lr2BarBaseStateResolver::positionlessStateAt(int row) const {
    if (row < 0 || row >= m_baseStates.size()) {
        return QVariant();
    }

    State state = m_baseStates.at(row);
    if (!state.valid) {
        return QVariant();
    }

    state.x = 0.0;
    state.y = 0.0;
    return stateToVariant(state);
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
        row.offDsts = parseDsts(rowField(rowValue, QStringLiteral("offDsts")));
        row.onDsts = parseDsts(rowField(rowValue, QStringLiteral("onDsts")));
        m_rows.append(row);
    }
}

void Lr2BarBaseStateResolver::rebuildActiveOptionSet() {
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
        next.append(currentState(
            dsts,
            m_effectiveSkinTime,
            first ? timerFire(first->timer) : -1.0,
            *this));
    }

    const bool statesChanged = !statesEqual(m_baseStates, next);
    const bool wasFastScrollActive = m_fastScrollActive;
    const qreal oldFastScrollDx = m_fastScrollDx;
    const qreal oldFastScrollDy = m_fastScrollDy;
    if (statesChanged) {
        m_baseStates = std::move(next);
        ++m_baseStatesRevision;
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
        const int offEnd = freezeEndTime(row.offDsts);
        const int onEnd = freezeEndTime(row.onDsts);
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

    return timerIdx == 0 ? 0.0 : -1.0;
}

bool Lr2BarBaseStateResolver::allOpsMatch(const Dst& dst) const {
    return checkSingleOp(dst.op1) && checkSingleOp(dst.op2) && checkSingleOp(dst.op3);
}

bool Lr2BarBaseStateResolver::checkSingleOp(int op) const {
    if (op == 0) {
        return true;
    }
    const bool negate = op < 0;
    const bool present = m_activeOptionSet.contains(std::abs(op));
    return negate ? !present : present;
}

QVector<Lr2BarBaseStateResolver::Dst> Lr2BarBaseStateResolver::parseDsts(const QVariant& value) {
    QVector<Dst> result;

    const QVariantList list = value.toList();
    if (!list.isEmpty()) {
        result.reserve(list.size());
        for (const QVariant& entry : list) {
            Dst dst;
            if (readDst(entry, dst)) {
                result.append(dst);
            }
        }
        return result;
    }

    return result;
}

bool Lr2BarBaseStateResolver::readDst(const QVariant& value, Dst& dst) {
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
        dst.timerCallback = parsed.timerCallback;
        dst.op1 = parsed.op1;
        dst.op2 = parsed.op2;
        dst.op3 = parsed.op3;
        dst.op4 = parsed.op4;
        dst.stretch = parsed.stretch;
        dst.hasMouseRect = parsed.hasMouseRect;
        dst.mouseRectX = parsed.mouseRectX;
        dst.mouseRectY = parsed.mouseRectY;
        dst.mouseRectW = parsed.mouseRectW;
        dst.mouseRectH = parsed.mouseRectH;
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
        dst.timerCallback = mapInt(map, QStringLiteral("timerCallback"), 0);
        dst.op1 = mapInt(map, QStringLiteral("op1"), 0);
        dst.op2 = mapInt(map, QStringLiteral("op2"), 0);
        dst.op3 = mapInt(map, QStringLiteral("op3"), 0);
        dst.op4 = mapInt(map, QStringLiteral("op4"), 0);
        dst.stretch = mapInt(map, QStringLiteral("stretch"), -1);
        dst.hasMouseRect = mapBool(map, QStringLiteral("hasMouseRect"), false);
        if (dst.hasMouseRect || mapHas(map, QStringLiteral("mouseRectX"))) {
            dst.hasMouseRect = true;
            dst.mouseRectX = mapInt(map, QStringLiteral("mouseRectX"), 0);
            dst.mouseRectY = mapInt(map, QStringLiteral("mouseRectY"), 0);
            dst.mouseRectW = mapInt(map, QStringLiteral("mouseRectW"), 0);
            dst.mouseRectH = mapInt(map, QStringLiteral("mouseRectH"), 0);
        }
        return true;
    }

    return false;
}

QVariant Lr2BarBaseStateResolver::rowField(const QVariant& row, const QString& name) {
    if (row.canConvert<QVariantMap>()) {
        return row.toMap().value(name);
    }
    return {};
}

int Lr2BarBaseStateResolver::freezeEndTime(const QVector<Dst>& dsts) {
    if (dsts.isEmpty() || !dsts.front().valid) {
        return 0;
    }

    const Dst& first = dsts.front();
    if (first.timer != 0 || first.timerCallback > 0) {
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

Lr2BarBaseStateResolver::State Lr2BarBaseStateResolver::currentState(
                                            const QVector<Dst>& dsts,
                                            int globalTime,
                                            qreal timerFire,
                                            const Lr2BarBaseStateResolver& resolver) {
    if (dsts.isEmpty() || !dsts.front().valid) {
        return {};
    }

    const Dst& first = dsts.front();
    if (!resolver.allOpsMatch(first) || timerFire < 0.0) {
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
    state.stretch = d1->stretch;
    state.hasMouseRect = first.hasMouseRect;
    state.mouseRectX = first.mouseRectX;
    state.mouseRectY = first.mouseRectY;
    state.mouseRectW = first.mouseRectW;
    state.mouseRectH = first.mouseRectH;
    return state;
}

Lr2BarBaseStateResolver::State Lr2BarBaseStateResolver::copyDstAsState(
        const Dst& dst,
        const Dst& controlDst) {
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
    state.stretch = dst.stretch;
    state.hasMouseRect = controlDst.hasMouseRect;
    state.mouseRectX = controlDst.mouseRectX;
    state.mouseRectY = controlDst.mouseRectY;
    state.mouseRectW = controlDst.mouseRectW;
    state.mouseRectH = controlDst.mouseRectH;
    return state;
}

QVariant Lr2BarBaseStateResolver::stateToVariant(const State& state) {
    if (!state.valid) {
        return {};
    }
    return QVariantMap {
        {QStringLiteral("x"), state.x},
        {QStringLiteral("y"), state.y},
        {QStringLiteral("w"), state.w},
        {QStringLiteral("h"), state.h},
        {QStringLiteral("a"), state.a},
        {QStringLiteral("r"), state.r},
        {QStringLiteral("g"), state.g},
        {QStringLiteral("b"), state.b},
        {QStringLiteral("angle"), state.angle},
        {QStringLiteral("center"), state.center},
        {QStringLiteral("sortId"), state.sortId},
        {QStringLiteral("blend"), state.blend},
        {QStringLiteral("filter"), state.filter},
        {QStringLiteral("op1"), state.op1},
        {QStringLiteral("op2"), state.op2},
        {QStringLiteral("op3"), state.op3},
        {QStringLiteral("op4"), state.op4},
        {QStringLiteral("stretch"), state.stretch},
        {QStringLiteral("hasMouseRect"), state.hasMouseRect},
        {QStringLiteral("mouseRectX"), state.mouseRectX},
        {QStringLiteral("mouseRectY"), state.mouseRectY},
        {QStringLiteral("mouseRectW"), state.mouseRectW},
        {QStringLiteral("mouseRectH"), state.mouseRectH},
    };
}

bool Lr2BarBaseStateResolver::statesEqual(const QVector<State>& lhs, const QVector<State>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (qsizetype i = 0; i < lhs.size(); ++i) {
        if (!stateEqual(lhs.at(i), rhs.at(i))) {
            return false;
        }
    }
    return true;
}

bool Lr2BarBaseStateResolver::stateEqual(const State& lhs, const State& rhs) {
    return lhs.valid == rhs.valid
        && sameStateNumber(lhs.x, rhs.x)
        && sameStateNumber(lhs.y, rhs.y)
        && sameStateNumber(lhs.w, rhs.w)
        && sameStateNumber(lhs.h, rhs.h)
        && sameStateNumber(lhs.a, rhs.a)
        && sameStateNumber(lhs.r, rhs.r)
        && sameStateNumber(lhs.g, rhs.g)
        && sameStateNumber(lhs.b, rhs.b)
        && sameStateNumber(lhs.angle, rhs.angle)
        && sameStateNumber(lhs.center, rhs.center)
        && sameStateNumber(lhs.sortId, rhs.sortId)
        && lhs.blend == rhs.blend
        && lhs.filter == rhs.filter
        && lhs.op1 == rhs.op1
        && lhs.op2 == rhs.op2
        && lhs.op3 == rhs.op3
        && lhs.op4 == rhs.op4
        && lhs.stretch == rhs.stretch
        && lhs.hasMouseRect == rhs.hasMouseRect
        && lhs.mouseRectX == rhs.mouseRectX
        && lhs.mouseRectY == rhs.mouseRectY
        && lhs.mouseRectW == rhs.mouseRectW
        && lhs.mouseRectH == rhs.mouseRectH;
}

qreal Lr2BarBaseStateResolver::applyAccel(qreal progress, int accType) {
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
