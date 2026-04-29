#include "Lr2SelectVisualState.h"

#include <algorithm>
#include <cmath>

Lr2SelectVisualState::Lr2SelectVisualState(QObject* parent) : QObject(parent) {}

qreal Lr2SelectVisualState::visualIndex() const {
    return m_visualIndex;
}

void Lr2SelectVisualState::setVisualIndex(qreal value) {
    if (m_visualIndex == value) {
        return;
    }
    m_visualIndex = value;
    emit visualIndexChanged();
    recompute();
}

int Lr2SelectVisualState::logicalCount() const {
    return m_logicalCount;
}

void Lr2SelectVisualState::setLogicalCount(int value) {
    value = std::max(0, value);
    if (m_logicalCount == value) {
        return;
    }
    m_logicalCount = value;
    emit logicalCountChanged();
    recompute();
}

int Lr2SelectVisualState::scrollDirection() const {
    return m_scrollDirection;
}

void Lr2SelectVisualState::setScrollDirection(int value) {
    if (m_scrollDirection == value) {
        return;
    }
    m_scrollDirection = value;
    emit scrollDirectionChanged();
    recompute();
}

int Lr2SelectVisualState::scrollDownDirection() const {
    return m_scrollDownDirection;
}

void Lr2SelectVisualState::setScrollDownDirection(int value) {
    if (m_scrollDownDirection == value) {
        return;
    }
    m_scrollDownDirection = value;
    emit scrollDownDirectionChanged();
    recompute();
}

int Lr2SelectVisualState::fixed() const {
    return m_fixed;
}

int Lr2SelectVisualState::rawFixed() const {
    return m_rawFixed;
}

int Lr2SelectVisualState::baseIndex() const {
    return m_baseIndex;
}

int Lr2SelectVisualState::cursorBaseIndex() const {
    return m_cursorBaseIndex;
}

qreal Lr2SelectVisualState::offset() const {
    return m_offset;
}

void Lr2SelectVisualState::recompute() {
    int nextFixed = 0;
    int nextRawFixed = 0;
    int nextBaseIndex = 0;
    int nextCursorBaseIndex = 0;
    qreal nextOffset = 0.0;

    if (m_logicalCount > 0) {
        const auto span = static_cast<qint64>(m_logicalCount) * 1000;
        const auto rounded = static_cast<qint64>(std::floor(m_visualIndex * 1000.0 + 0.5));
        nextRawFixed = static_cast<int>(rounded);
        nextFixed = static_cast<int>(((rounded % span) + span) % span);
        nextBaseIndex = nextFixed / 1000;
        nextOffset = nextFixed / 1000.0 - nextBaseIndex;
        nextCursorBaseIndex = nextBaseIndex;
        if (nextFixed % 1000 != 0 && m_scrollDirection == m_scrollDownDirection) {
            ++nextCursorBaseIndex;
        }
    }

    const bool nextFixedChanged = m_fixed != nextFixed;
    const bool nextRawFixedChanged = m_rawFixed != nextRawFixed;
    const bool nextBaseChanged = m_baseIndex != nextBaseIndex;
    const bool nextCursorChanged = m_cursorBaseIndex != nextCursorBaseIndex;
    const bool nextOffsetChanged = m_offset != nextOffset;

    m_fixed = nextFixed;
    m_rawFixed = nextRawFixed;
    m_baseIndex = nextBaseIndex;
    m_cursorBaseIndex = nextCursorBaseIndex;
    m_offset = nextOffset;

    if (nextFixedChanged) {
        emit fixedChanged();
    }
    if (nextRawFixedChanged) {
        emit rawFixedChanged();
    }
    if (nextBaseChanged) {
        emit baseIndexChanged();
    }
    if (nextCursorChanged) {
        emit cursorBaseIndexChanged();
    }
    if (nextBaseChanged || nextCursorChanged) {
        emit logicalPositionChanged();
    }
    if (nextOffsetChanged) {
        emit offsetChanged();
    }
}
