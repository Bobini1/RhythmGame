#include "Lr2BarPositionMap.h"

#include "Lr2SelectVisualState.h"
#include <QtGlobal>

#include <limits>

namespace {
int boundedIntSize(qsizetype size) {
    return size > std::numeric_limits<int>::max()
        ? std::numeric_limits<int>::max()
        : static_cast<int>(size);
}
} // namespace

Lr2BarPositionMap::Lr2BarPositionMap(QObject* parent) : QObject(parent) {}

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

Lr2SelectBarModel* Lr2BarPositionMap::slotOffsetModel() const {
    return m_slotOffsetModel;
}

void Lr2BarPositionMap::setSlotOffsetModel(Lr2SelectBarModel* model) {
    if (m_slotOffsetModel == model) {
        return;
    }

    if (m_slotOffsetModelConnection) {
        disconnect(m_slotOffsetModelConnection);
        m_slotOffsetModelConnection = {};
    }

    m_slotOffsetModel = model;
    if (m_slotOffsetModel) {
        m_slotOffsetModelConnection = connect(
            m_slotOffsetModel,
            &Lr2SelectBarModel::slotOffsetChanged,
            this,
            &Lr2BarPositionMap::syncSlotOffsetFromModel);
    }

    if (m_slotOffsetModel) {
        syncSlotOffsetFromModel();
    } else {
        setSlotOffset(0);
    }
    emit slotOffsetModelChanged();
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
    return boundedIntSize(m_drawXs.size());
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

bool Lr2BarPositionMap::validAt(int row) const {
    return row >= 0 && row < m_validRows.size() && m_validRows[row];
}

int Lr2BarPositionMap::slotForRow(int row) const {
    const int count = m_slotCount > 0 ? m_slotCount : boundedIntSize(m_drawXs.size());
    if (count <= 0 || row < 0 || row >= count) {
        return -1;
    }
    return (row + m_slotOffset) % count;
}

int Lr2BarPositionMap::rowForSlot(int slot) const {
    const int count = m_slotCount > 0 ? m_slotCount : boundedIntSize(m_drawXs.size());
    if (count <= 0 || slot < 0) {
        return -1;
    }
    return ((slot - m_slotOffset) % count + count) % count;
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

void Lr2BarPositionMap::syncSlotOffsetFromModel() {
    if (!m_slotOffsetModel) {
        return;
    }
    setSlotOffset(m_slotOffsetModel->slotOffset());
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
