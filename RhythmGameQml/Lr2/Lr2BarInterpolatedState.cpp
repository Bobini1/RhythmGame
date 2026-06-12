#include "Lr2BarInterpolatedState.h"

#include <QtMath>

Lr2BarInterpolatedState::Lr2BarInterpolatedState(QObject* parent)
    : QObject(parent),
      m_state(hiddenState()) {}

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
            &Lr2BarInterpolatedState::updateState);
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

bool Lr2BarInterpolatedState::isValid() const {
    return m_state.valid;
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

Lr2BarInterpolatedState::State Lr2BarInterpolatedState::hiddenState() {
    State state;
    state.a = 0.0;
    return state;
}

Lr2BarInterpolatedState::State Lr2BarInterpolatedState::visibleState(const State& state) {
    return state.valid ? state : hiddenState();
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
    State next = hiddenState();
    const int stateCount = m_baseStateResolver ? m_baseStateResolver->stateCount() : 0;
    if (m_enabled && m_row >= 0 && m_row < stateCount) {
        const State from = visibleState(m_baseStateResolver->stateValueAt(m_row));
        next = from;

        const qreal progress = m_positionMap ? m_positionMap->scrollOffset() : 0.0;
        const State previous = progress > 0.001 && m_row > 0
            ? visibleState(m_baseStateResolver->stateValueAt(m_row - 1))
            : hiddenState();
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
