#include "Lr2BarBaseStateCache.h"

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

QVariant invalidState() {
    return {};
}
} // namespace

Lr2BarBaseStateCache::Lr2BarBaseStateCache(QObject* parent) : QObject(parent) {}

QVariantList Lr2BarBaseStateCache::barRows() const {
    return m_barRows;
}

void Lr2BarBaseStateCache::setBarRows(const QVariantList& rows) {
    if (m_barRows == rows) {
        return;
    }

    m_barRows = rows;
    rebuildRows();
    updateAnimationLimit();
    emit barRowsChanged();
    rebuildBaseStates();
}

int Lr2BarBaseStateCache::selectedRow() const {
    return m_selectedRow;
}

void Lr2BarBaseStateCache::setSelectedRow(int row) {
    if (m_selectedRow == row) {
        return;
    }

    m_selectedRow = row;
    emit selectedRowChanged();
    rebuildBaseStates();
}

int Lr2BarBaseStateCache::skinTime() const {
    return m_requestedSkinTime;
}

void Lr2BarBaseStateCache::setSkinTime(int time) {
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

QVariant Lr2BarBaseStateCache::timers() const {
    return m_timers;
}

void Lr2BarBaseStateCache::setTimers(const QVariant& timers) {
    if (m_timers == timers) {
        return;
    }

    m_timers = timers;
    emit timersChanged();
    rebuildBaseStates();
}

QVariant Lr2BarBaseStateCache::activeOptions() const {
    return m_activeOptions;
}

void Lr2BarBaseStateCache::setActiveOptions(const QVariant& options) {
    if (m_activeOptions == options) {
        return;
    }

    m_activeOptions = options;
    rebuildActiveOptionSet();
    emit activeOptionsChanged();
    rebuildBaseStates();
}

QVariantList Lr2BarBaseStateCache::baseStates() const {
    return m_baseStates;
}

int Lr2BarBaseStateCache::animationLimit() const {
    return m_animationLimit;
}

void Lr2BarBaseStateCache::rebuildRows() {
    m_rows.clear();
    m_rows.reserve(m_barRows.size());

    for (const QVariant& rowValue : m_barRows) {
        Row row;
        row.offDsts = parseDsts(rowField(rowValue, QStringLiteral("offDsts")));
        row.onDsts = parseDsts(rowField(rowValue, QStringLiteral("onDsts")));
        m_rows.append(row);
    }
}

void Lr2BarBaseStateCache::rebuildActiveOptionSet() {
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

void Lr2BarBaseStateCache::rebuildBaseStates() {
    QVariantList next;
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

    m_baseStates = next;
    emit baseStatesChanged();
}

void Lr2BarBaseStateCache::updateAnimationLimit() {
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

int Lr2BarBaseStateCache::effectiveSkinTime(int requestedTime) const {
    return m_animationLimit < 0 ? requestedTime : std::min(requestedTime, m_animationLimit);
}

qreal Lr2BarBaseStateCache::timerFire(int timerIdx) const {
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

bool Lr2BarBaseStateCache::allOpsMatch(const Dst& dst) const {
    return checkSingleOp(dst.op1) && checkSingleOp(dst.op2) && checkSingleOp(dst.op3);
}

bool Lr2BarBaseStateCache::checkSingleOp(int op) const {
    if (op == 0) {
        return true;
    }
    const bool negate = op < 0;
    const bool present = m_activeOptionSet.contains(std::abs(op));
    return negate ? !present : present;
}

QVector<Lr2BarBaseStateCache::Dst> Lr2BarBaseStateCache::parseDsts(const QVariant& value) {
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

    if (!value.canConvert<QJSValue>()) {
        return result;
    }

    const QJSValue array = value.value<QJSValue>();
    const int length = array.property(QStringLiteral("length")).toInt();
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        Dst dst;
        if (readDst(QVariant::fromValue(array.property(static_cast<quint32>(i))), dst)) {
            result.append(dst);
        }
    }
    return result;
}

bool Lr2BarBaseStateCache::readDst(const QVariant& value, Dst& dst) {
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

QVariant Lr2BarBaseStateCache::rowField(const QVariant& row, const QString& name) {
    if (row.canConvert<QVariantMap>()) {
        return row.toMap().value(name);
    }

    if (row.canConvert<QJSValue>()) {
        const QJSValue value = row.value<QJSValue>();
        if (value.isObject()) {
            return QVariant::fromValue(value.property(name));
        }
    }

    return {};
}

int Lr2BarBaseStateCache::freezeEndTime(const QVector<Dst>& dsts) {
    if (dsts.isEmpty() || !dsts.front().valid) {
        return 0;
    }

    const Dst& first = dsts.front();
    if (first.timer != 0) {
        return -1;
    }

    if (dsts.size() == 1) {
        return first.loop < 0 ? -1 : first.time;
    }

    const int endTime = dsts.back().time;
    return first.loop == endTime ? endTime : -1;
}

QVariant Lr2BarBaseStateCache::currentState(const QVector<Dst>& dsts,
                                            int globalTime,
                                            qreal timerFire,
                                            const Lr2BarBaseStateCache& cache) {
    if (dsts.isEmpty() || !dsts.front().valid) {
        return invalidState();
    }

    const Dst& first = dsts.front();
    if (!cache.allOpsMatch(first) || timerFire < 0.0) {
        return invalidState();
    }

    int time = globalTime - static_cast<int>(std::floor(timerFire));
    if (time < first.time) {
        return invalidState();
    }

    const Dst& last = dsts.back();
    const int endTime = last.time;
    const int loopTo = first.loop;

    if (dsts.size() == 1) {
        if (loopTo < 0 && time > endTime) {
            return invalidState();
        }
        return copyDstAsState(first, first);
    }

    if (time > endTime) {
        if (loopTo < 0) {
            return invalidState();
        }
        if (loopTo == endTime) {
            time = endTime;
        } else if (loopTo > endTime) {
            time = 0;
        } else {
            const int period = endTime - loopTo;
            if (period <= 0) {
                return invalidState();
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

    QVariantMap state;
    state.insert(QStringLiteral("x"), mix(d1->x, d2->x));
    state.insert(QStringLiteral("y"), mix(d1->y, d2->y));
    state.insert(QStringLiteral("w"), mix(d1->w, d2->w));
    state.insert(QStringLiteral("h"), mix(d1->h, d2->h));
    state.insert(QStringLiteral("a"), mix(d1->a, d2->a));
    state.insert(QStringLiteral("r"), mix(d1->r, d2->r));
    state.insert(QStringLiteral("g"), mix(d1->g, d2->g));
    state.insert(QStringLiteral("b"), mix(d1->b, d2->b));
    state.insert(QStringLiteral("angle"), mix(d1->angle, d2->angle));
    state.insert(QStringLiteral("center"), d1->center);
    state.insert(QStringLiteral("sortId"), mix(d1->sortId, d2->sortId));
    state.insert(QStringLiteral("blend"), d1->blend);
    state.insert(QStringLiteral("filter"), d1->filter);
    state.insert(QStringLiteral("op1"), first.op1);
    state.insert(QStringLiteral("op2"), first.op2);
    state.insert(QStringLiteral("op3"), first.op3);
    state.insert(QStringLiteral("op4"), first.op4);
    return state;
}

QVariant Lr2BarBaseStateCache::copyDstAsState(const Dst& dst, const Dst& controlDst) {
    QVariantMap state;
    state.insert(QStringLiteral("x"), dst.x);
    state.insert(QStringLiteral("y"), dst.y);
    state.insert(QStringLiteral("w"), dst.w);
    state.insert(QStringLiteral("h"), dst.h);
    state.insert(QStringLiteral("a"), dst.a);
    state.insert(QStringLiteral("r"), dst.r);
    state.insert(QStringLiteral("g"), dst.g);
    state.insert(QStringLiteral("b"), dst.b);
    state.insert(QStringLiteral("angle"), dst.angle);
    state.insert(QStringLiteral("center"), dst.center);
    state.insert(QStringLiteral("sortId"), dst.sortId);
    state.insert(QStringLiteral("blend"), dst.blend);
    state.insert(QStringLiteral("filter"), dst.filter);
    state.insert(QStringLiteral("op1"), controlDst.op1);
    state.insert(QStringLiteral("op2"), controlDst.op2);
    state.insert(QStringLiteral("op3"), controlDst.op3);
    state.insert(QStringLiteral("op4"), controlDst.op4);
    return state;
}

qreal Lr2BarBaseStateCache::applyAccel(qreal progress, int accType) {
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
