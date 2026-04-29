#include "Lr2BarPositionCache.h"

#include "Lr2SelectVisualState.h"

#include <QJSValue>
#include <QVariantMap>
#include <QtGlobal>

Lr2BarPositionCache::Lr2BarPositionCache(QObject* parent) : QObject(parent) {}

QVariantList Lr2BarPositionCache::baseStates() const {
    return {};
}

void Lr2BarPositionCache::setBaseStates(const QVariantList& states) {
    rebuildBaseCoordinates(states);
    rebuildDrawCoordinates();
    emit baseStatesChanged();
    bumpRevision();
}

qreal Lr2BarPositionCache::scrollOffset() const {
    return m_scrollOffset;
}

void Lr2BarPositionCache::setScrollOffset(qreal offset) {
    if (qAbs(m_scrollOffset - offset) <= 0.000001) {
        return;
    }
    m_scrollOffset = offset;
    rebuildDrawCoordinates();
    emit scrollOffsetChanged();
    bumpRevision();
}

Lr2SelectVisualState* Lr2BarPositionCache::visualState() const {
    return m_visualState;
}

void Lr2BarPositionCache::setVisualState(Lr2SelectVisualState* state) {
    if (m_visualState == state) {
        return;
    }

    if (m_visualState) {
        disconnect(m_visualStateOffsetConnection);
    }

    m_visualState = state;
    if (m_visualState) {
        m_visualStateOffsetConnection = connect(
            m_visualState,
            &Lr2SelectVisualState::offsetChanged,
            this,
            [this]() {
                setScrollOffset(m_visualState ? m_visualState->offset() : 0.0);
            });
        setScrollOffset(m_visualState->offset());
    } else {
        m_visualStateOffsetConnection = {};
        setScrollOffset(0.0);
    }

    emit visualStateChanged();
}

int Lr2BarPositionCache::revision() const {
    return m_revision;
}

int Lr2BarPositionCache::count() const {
    return m_drawXs.size();
}

qreal Lr2BarPositionCache::xAt(int row) const {
    if (row < 0 || row >= m_drawXs.size()) {
        return 0.0;
    }
    return m_drawXs[row];
}

qreal Lr2BarPositionCache::yAt(int row) const {
    if (row < 0 || row >= m_drawYs.size()) {
        return 0.0;
    }
    return m_drawYs[row];
}

void Lr2BarPositionCache::rebuildBaseCoordinates(const QVariantList& states) {
    const auto size = states.size();
    m_baseXs.resize(size);
    m_baseYs.resize(size);
    m_validRows.resize(size);

    for (qsizetype row = 0; row < size; ++row) {
        qreal x = 0.0;
        qreal y = 0.0;
        const bool valid = extractCoordinates(states[row], x, y);
        m_baseXs[row] = x;
        m_baseYs[row] = y;
        m_validRows[row] = valid;
    }
}

void Lr2BarPositionCache::rebuildDrawCoordinates() {
    const auto size = m_baseXs.size();
    m_drawXs.resize(size);
    m_drawYs.resize(size);

    const bool interpolate = m_scrollOffset > 0.001;
    const qreal inv = 1.0 - m_scrollOffset;

    for (qsizetype row = 0; row < size; ++row) {
        if (!m_validRows[row]) {
            m_drawXs[row] = 0.0;
            m_drawYs[row] = 0.0;
            continue;
        }

        if (interpolate && row > 0 && m_validRows[row - 1]) {
            m_drawXs[row] = m_baseXs[row] * inv + m_baseXs[row - 1] * m_scrollOffset;
            m_drawYs[row] = m_baseYs[row] * inv + m_baseYs[row - 1] * m_scrollOffset;
        } else {
            m_drawXs[row] = m_baseXs[row];
            m_drawYs[row] = m_baseYs[row];
        }
    }
}

void Lr2BarPositionCache::bumpRevision() {
    ++m_revision;
    emit revisionChanged();
}

bool Lr2BarPositionCache::extractCoordinates(const QVariant& state, qreal& x, qreal& y) {
    if (!state.isValid() || state.isNull()) {
        return false;
    }

    if (state.canConvert<QVariantMap>()) {
        const QVariantMap map = state.toMap();
        x = map.value(QStringLiteral("x")).toDouble();
        y = map.value(QStringLiteral("y")).toDouble();
        return map.contains(QStringLiteral("x")) || map.contains(QStringLiteral("y"));
    }

    if (state.canConvert<QJSValue>()) {
        const QJSValue jsValue = state.value<QJSValue>();
        if (!jsValue.isObject()) {
            return false;
        }
        x = jsValue.property(QStringLiteral("x")).toNumber();
        y = jsValue.property(QStringLiteral("y")).toNumber();
        return true;
    }

    return false;
}
