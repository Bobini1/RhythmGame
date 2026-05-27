#include "Lr2BarInterpolatedState.h"

#include <QVariantMap>
#include <QtMath>

Lr2BarInterpolatedState::Lr2BarInterpolatedState(QObject* parent) : QObject(parent) {}

QVariantList Lr2BarInterpolatedState::baseStates() const {
    return {};
}

void Lr2BarInterpolatedState::setBaseStates(const QVariantList& states) {
    m_baseStates.resize(states.size());
    for (qsizetype i = 0; i < states.size(); ++i) {
        m_baseStates[i] = extractState(states[i]);
    }
    emit baseStatesChanged();
    updateState();
}

Lr2BarBaseStateResolver* Lr2BarInterpolatedState::baseStateResolver() const {
    return m_baseStateResolver;
}

void Lr2BarInterpolatedState::setBaseStateResolver(Lr2BarBaseStateResolver* resolver) {
    if (m_baseStateResolver == resolver) {
        return;
    }

    if (m_baseStateResolverConnection) {
        disconnect(m_baseStateResolverConnection);
    }

    m_baseStateResolver = resolver;
    if (m_baseStateResolver) {
        m_baseStateResolverConnection = connect(
            m_baseStateResolver,
            &Lr2BarBaseStateResolver::baseStatesChanged,
            this,
            [this]() {
                emit baseStatesChanged();
                updateState();
            });
    } else {
        m_baseStateResolverConnection = {};
    }

    emit baseStateResolverChanged();
    updateState();
}

Lr2BarPositionMap* Lr2BarInterpolatedState::positionMap() const {
    return m_positionMap;
}

void Lr2BarInterpolatedState::setPositionMap(Lr2BarPositionMap* map) {
    if (m_positionMap == map) {
        return;
    }

    if (m_coordinatesConnection) {
        disconnect(m_coordinatesConnection);
    }

    m_positionMap = map;
    if (m_positionMap) {
        m_coordinatesConnection = connect(
            m_positionMap,
            &Lr2BarPositionMap::coordinatesChanged,
            this,
            &Lr2BarInterpolatedState::updateState);
    } else {
        m_coordinatesConnection = {};
    }

    emit positionMapChanged();
    updateState();
}

int Lr2BarInterpolatedState::row() const {
    return m_row;
}

void Lr2BarInterpolatedState::setRow(int row) {
    if (m_row == row) {
        return;
    }
    m_row = row;
    emit rowChanged();
    updateState();
}

bool Lr2BarInterpolatedState::isEnabled() const {
    return m_enabled;
}

void Lr2BarInterpolatedState::setEnabled(bool enabled) {
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    emit enabledChanged();
    updateState();
}

qreal Lr2BarInterpolatedState::x() const {
    return m_state.x;
}

qreal Lr2BarInterpolatedState::y() const {
    return m_state.y;
}

qreal Lr2BarInterpolatedState::w() const {
    return m_state.w;
}

qreal Lr2BarInterpolatedState::h() const {
    return m_state.h;
}

qreal Lr2BarInterpolatedState::a() const {
    return m_state.a;
}

qreal Lr2BarInterpolatedState::r() const {
    return m_state.r;
}

qreal Lr2BarInterpolatedState::g() const {
    return m_state.g;
}

qreal Lr2BarInterpolatedState::b() const {
    return m_state.b;
}

qreal Lr2BarInterpolatedState::angle() const {
    return m_state.angle;
}

qreal Lr2BarInterpolatedState::center() const {
    return m_state.center;
}

qreal Lr2BarInterpolatedState::sortId() const {
    return m_state.sortId;
}

qreal Lr2BarInterpolatedState::blend() const {
    return m_state.blend;
}

qreal Lr2BarInterpolatedState::filter() const {
    return m_state.filter;
}

qreal Lr2BarInterpolatedState::op1() const {
    return m_state.op1;
}

qreal Lr2BarInterpolatedState::op2() const {
    return m_state.op2;
}

qreal Lr2BarInterpolatedState::op3() const {
    return m_state.op3;
}

qreal Lr2BarInterpolatedState::op4() const {
    return m_state.op4;
}

bool Lr2BarInterpolatedState::sameReal(qreal lhs, qreal rhs) {
    return qAbs(lhs - rhs) <= 0.000001;
}

qreal Lr2BarInterpolatedState::readField(const QVariantMap& map, const QString& name, qreal fallback) {
    const auto it = map.constFind(name);
    return it == map.constEnd() || !it->isValid() || it->isNull() ? fallback : it->toDouble();
}

Lr2BarInterpolatedState::State Lr2BarInterpolatedState::extractState(const QVariant& variant) {
    State state;
    if (!variant.isValid() || variant.isNull()) {
        return state;
    }

    if (variant.canConvert<QVariantMap>()) {
        const QVariantMap map = variant.toMap();
        state.valid = map.contains(QStringLiteral("x")) || map.contains(QStringLiteral("y"));
        state.x = readField(map, QStringLiteral("x"), 0.0);
        state.y = readField(map, QStringLiteral("y"), 0.0);
        state.w = readField(map, QStringLiteral("w"), 0.0);
        state.h = readField(map, QStringLiteral("h"), 0.0);
        state.a = readField(map, QStringLiteral("a"), 0.0);
        state.r = readField(map, QStringLiteral("r"), 255.0);
        state.g = readField(map, QStringLiteral("g"), 255.0);
        state.b = readField(map, QStringLiteral("b"), 255.0);
        state.angle = readField(map, QStringLiteral("angle"), 0.0);
        state.center = readField(map, QStringLiteral("center"), 0.0);
        state.sortId = readField(map, QStringLiteral("sortId"), 0.0);
        state.blend = readField(map, QStringLiteral("blend"), 0.0);
        state.filter = readField(map, QStringLiteral("filter"), 0.0);
        state.op1 = readField(map, QStringLiteral("op1"), 0.0);
        state.op2 = readField(map, QStringLiteral("op2"), 0.0);
        state.op3 = readField(map, QStringLiteral("op3"), 0.0);
        state.op4 = readField(map, QStringLiteral("op4"), 0.0);
        return state;
    }

    return state;
}

Lr2BarInterpolatedState::State Lr2BarInterpolatedState::interpolate(
    const State& from,
    const State& to,
    qreal progress) {
    const qreal inv = 1.0 - progress;
    auto mix = [inv, progress](qreal a, qreal b) {
        return a * inv + b * progress;
    };

    State result;
    result.valid = from.valid;
    result.x = mix(from.x, to.x);
    result.y = mix(from.y, to.y);
    result.w = mix(from.w, to.w);
    result.h = mix(from.h, to.h);
    result.a = mix(from.a, to.a);
    result.r = mix(from.r, to.r);
    result.g = mix(from.g, to.g);
    result.b = mix(from.b, to.b);
    result.angle = mix(from.angle, to.angle);
    result.center = from.center;
    result.sortId = mix(from.sortId, to.sortId);
    result.blend = from.blend;
    result.filter = from.filter;
    return result;
}

void Lr2BarInterpolatedState::updateState() {
    State next;
    const bool useResolver = m_baseStateResolver && m_baseStateResolver->stateCount() > 0;
    const int stateCount = useResolver ? m_baseStateResolver->stateCount() : m_baseStates.size();
    if (m_enabled && m_row >= 0 && m_row < stateCount) {
        const State from = useResolver
            ? extractState(m_baseStateResolver->stateAt(m_row))
            : m_baseStates[m_row];
        next = from;

        const qreal progress = m_positionMap ? m_positionMap->scrollOffset() : 0.0;
        const State previous = progress > 0.001 && m_row > 0
            ? (useResolver
                ? extractState(m_baseStateResolver->stateAt(m_row - 1))
                : m_baseStates[m_row - 1])
            : State {};
        if (progress > 0.001 && m_row > 0 && previous.valid) {
            next = interpolate(from, previous, progress);
        }
    }

    if (assignState(next)) {
        emit stateChanged();
    }
}

bool Lr2BarInterpolatedState::assignState(const State& state) {
    const bool changed = m_state.valid != state.valid
        || !sameReal(m_state.x, state.x)
        || !sameReal(m_state.y, state.y)
        || !sameReal(m_state.w, state.w)
        || !sameReal(m_state.h, state.h)
        || !sameReal(m_state.a, state.a)
        || !sameReal(m_state.r, state.r)
        || !sameReal(m_state.g, state.g)
        || !sameReal(m_state.b, state.b)
        || !sameReal(m_state.angle, state.angle)
        || !sameReal(m_state.center, state.center)
        || !sameReal(m_state.sortId, state.sortId)
        || !sameReal(m_state.blend, state.blend)
        || !sameReal(m_state.filter, state.filter)
        || !sameReal(m_state.op1, state.op1)
        || !sameReal(m_state.op2, state.op2)
        || !sameReal(m_state.op3, state.op3)
        || !sameReal(m_state.op4, state.op4);
    if (changed) {
        m_state = state;
    }
    return changed;
}
