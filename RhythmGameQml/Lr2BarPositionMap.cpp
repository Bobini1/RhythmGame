#include "Lr2BarPositionMap.h"

#include "Lr2SelectVisualState.h"

#include <QVariantMap>
#include <QtGlobal>

Lr2BarPositionMap::Lr2BarPositionMap(QObject* parent) : QObject(parent) {}

QVariantList Lr2BarPositionMap::baseStates() const {
    return {};
}

void Lr2BarPositionMap::setBaseStates(const QVariantList& states) {
    rebuildBaseCoordinates(states);
    rebuildDrawCoordinates();
    emit baseStatesChanged();
    notifyCoordinatesChanged();
}

Lr2BarBaseStateResolver* Lr2BarPositionMap::baseStateResolver() const {
    return m_baseStateResolver;
}

void Lr2BarPositionMap::setBaseStateResolver(Lr2BarBaseStateResolver* resolver) {
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
                rebuildBaseCoordinatesFromResolver();
                rebuildDrawCoordinates();
                emit baseStatesChanged();
                notifyCoordinatesChanged();
            });
        rebuildBaseCoordinatesFromResolver();
    } else {
        m_baseStateResolverConnection = {};
        m_baseXs.clear();
        m_baseYs.clear();
        m_validRows.clear();
    }
    rebuildDrawCoordinates();

    emit baseStateResolverChanged();
    emit baseStatesChanged();
    notifyCoordinatesChanged();
}

qreal Lr2BarPositionMap::scrollOffset() const {
    return m_scrollOffset;
}

void Lr2BarPositionMap::setScrollOffset(qreal offset) {
    if (qAbs(m_scrollOffset - offset) <= 0.000001) {
        return;
    }
    m_scrollOffset = offset;
    rebuildDrawCoordinates();
    notifyCoordinatesChanged();
}

int Lr2BarPositionMap::slotOffset() const {
    return m_slotOffset;
}

void Lr2BarPositionMap::setSlotOffset(int offset) {
    if (m_slotOffset == offset) {
        return;
    }
    m_slotOffset = offset;
    emit slotOffsetChanged();
}

QObject* Lr2BarPositionMap::slotOffsetSource() const {
    return m_slotOffsetSource;
}

void Lr2BarPositionMap::setSlotOffsetSource(QObject* source) {
    if (m_slotOffsetSource == source) {
        return;
    }

    if (m_slotOffsetSourceConnection) {
        disconnect(m_slotOffsetSourceConnection);
    }

    m_slotOffsetSource = source;
    if (m_slotOffsetSource) {
        m_slotOffsetSourceConnection = connect(
            m_slotOffsetSource,
            SIGNAL(visibleBarSlotOffsetChanged()),
            this,
            SLOT(updateSlotOffsetFromSource()));
        updateSlotOffsetFromSource();
    } else {
        m_slotOffsetSourceConnection = {};
        setSlotOffset(0);
    }
    emit slotOffsetSourceChanged();
}

void Lr2BarPositionMap::updateSlotOffsetFromSource() {
    setSlotOffset(m_slotOffsetSource
        ? m_slotOffsetSource->property("visibleBarSlotOffset").toInt()
        : 0);
}

int Lr2BarPositionMap::slotCount() const {
    return m_slotCount;
}

void Lr2BarPositionMap::setSlotCount(int count) {
    count = qMax(0, count);
    if (m_slotCount == count) {
        return;
    }
    m_slotCount = count;
    emit slotCountChanged();
}

Lr2SelectVisualState* Lr2BarPositionMap::visualState() const {
    return m_visualState;
}

void Lr2BarPositionMap::setVisualState(Lr2SelectVisualState* state) {
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

}
int Lr2BarPositionMap::count() const {
    return m_drawXs.size();
}

qreal Lr2BarPositionMap::xAt(int row) const {
    if (row < 0 || row >= m_drawXs.size()) {
        return 0.0;
    }
    return m_drawXs[row];
}

qreal Lr2BarPositionMap::yAt(int row) const {
    if (row < 0 || row >= m_drawYs.size()) {
        return 0.0;
    }
    return m_drawYs[row];
}

int Lr2BarPositionMap::slotForRow(int row) const {
    const int count = m_slotCount > 0 ? m_slotCount : m_drawXs.size();
    if (count <= 0 || row < 0 || row >= count) {
        return -1;
    }
    return (row + m_slotOffset) % count;
}

int Lr2BarPositionMap::rowForSlot(int slot) const {
    const int count = m_slotCount > 0 ? m_slotCount : m_drawXs.size();
    if (count <= 0 || slot < 0) {
        return -1;
    }
    return ((slot - m_slotOffset) % count + count) % count;
}

void Lr2BarPositionMap::rebuildBaseCoordinates(const QVariantList& states) {
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

void Lr2BarPositionMap::rebuildDrawCoordinates() {
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

void Lr2BarPositionMap::notifyCoordinatesChanged() {
    emit coordinatesChanged();
}

void Lr2BarPositionMap::rebuildBaseCoordinatesFromResolver() {
    const int size = m_baseStateResolver ? m_baseStateResolver->stateCount() : 0;
    m_baseXs.resize(size);
    m_baseYs.resize(size);
    m_validRows.resize(size);

    for (int row = 0; row < size; ++row) {
        m_validRows[row] = m_baseStateResolver->stateValidAt(row);
        m_baseXs[row] = m_baseStateResolver->stateXAt(row);
        m_baseYs[row] = m_baseStateResolver->stateYAt(row);
    }
}

bool Lr2BarPositionMap::extractCoordinates(const QVariant& state, qreal& x, qreal& y) {
    if (!state.isValid() || state.isNull()) {
        return false;
    }

    if (state.canConvert<QVariantMap>()) {
        const QVariantMap map = state.toMap();
        x = map.value(QStringLiteral("x")).toDouble();
        y = map.value(QStringLiteral("y")).toDouble();
        return map.contains(QStringLiteral("x")) || map.contains(QStringLiteral("y"));
    }

    return false;
}
