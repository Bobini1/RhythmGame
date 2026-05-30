#include "Lr2SelectNavigationController.h"

#include <QDateTime>

#include <algorithm>
#include <cmath>
#include <limits>

Lr2SelectNavigationController::Lr2SelectNavigationController(QObject* parent) : QObject(parent) {}

Lr2SelectVisualState* Lr2SelectNavigationController::visualState() const {
    return m_visualState;
}

void Lr2SelectNavigationController::setVisualState(Lr2SelectVisualState* state) {
    if (m_visualState == state) {
        return;
    }
    m_visualState = state;
    emit visualStateChanged();
}

int Lr2SelectNavigationController::currentIndex() const { return m_currentIndex; }
void Lr2SelectNavigationController::setCurrentIndex(int value) {
    if (m_currentIndex == value) {
        return;
    }
    m_currentIndex = value;
    emit currentIndexChanged();
}

int Lr2SelectNavigationController::targetIndex() const { return m_targetIndex; }
void Lr2SelectNavigationController::setTargetIndex(int value) {
    if (m_targetIndex == value) {
        return;
    }
    m_targetIndex = value;
    emit targetIndexChanged();
}

int Lr2SelectNavigationController::selectedOffset() const { return m_selectedOffset; }
void Lr2SelectNavigationController::setSelectedOffset(int value) {
    if (m_selectedOffset == value) {
        return;
    }
    m_selectedOffset = value;
    emit selectedOffsetChanged();
}

qreal Lr2SelectNavigationController::targetVisualIndex() const { return m_targetVisualIndex; }
void Lr2SelectNavigationController::setTargetVisualIndex(qreal value) {
    if (m_targetVisualIndex == value) {
        return;
    }
    m_targetVisualIndex = value;
    emit targetVisualIndexChanged();
}

int Lr2SelectNavigationController::oldBarFixed() const { return m_oldBarFixed; }
void Lr2SelectNavigationController::setOldBarFixed(int value) {
    if (m_oldBarFixed == value) {
        return;
    }
    m_oldBarFixed = value;
    emit oldBarFixedChanged();
}

int Lr2SelectNavigationController::nowBarFixed() const { return m_nowBarFixed; }
void Lr2SelectNavigationController::setNowBarFixed(int value) {
    if (m_nowBarFixed == value) {
        return;
    }
    m_nowBarFixed = value;
    emit nowBarFixedChanged();
}

qreal Lr2SelectNavigationController::barMoveStartMs() const { return m_barMoveStartMs; }
void Lr2SelectNavigationController::setBarMoveStartMs(qreal value) {
    if (m_barMoveStartMs == value) {
        return;
    }
    m_barMoveStartMs = value;
    emit barMoveStartMsChanged();
}

qreal Lr2SelectNavigationController::barMoveEndMs() const { return m_barMoveEndMs; }
void Lr2SelectNavigationController::setBarMoveEndMs(qreal value) {
    if (m_barMoveEndMs == value) {
        return;
    }
    m_barMoveEndMs = value;
    emit barMoveEndMsChanged();
}

bool Lr2SelectNavigationController::updatesActive() const { return m_updatesActive; }
void Lr2SelectNavigationController::setUpdatesActive(bool value) {
    if (m_updatesActive == value) {
        return;
    }
    m_updatesActive = value;
    emit updatesActiveChanged();
}

bool Lr2SelectNavigationController::suppressVisualIndexPublish() const { return m_suppressVisualIndexPublish; }
void Lr2SelectNavigationController::setSuppressVisualIndexPublish(bool value) {
    if (m_suppressVisualIndexPublish == value) {
        return;
    }
    m_suppressVisualIndexPublish = value;
    emit suppressVisualIndexPublishChanged();
}

int Lr2SelectNavigationController::scrollDirection() const { return m_scrollDirection; }
void Lr2SelectNavigationController::setScrollDirection(int value) {
    if (m_scrollDirection == value) {
        return;
    }
    m_scrollDirection = value;
    emit scrollDirectionChanged();
}

int Lr2SelectNavigationController::listRevision() const { return m_listRevision; }
void Lr2SelectNavigationController::setListRevision(int value) {
    if (m_listRevision == value) {
        return;
    }
    m_listRevision = value;
    emit listRevisionChanged();
}

int Lr2SelectNavigationController::selectionRevision() const { return m_selectionRevision; }
void Lr2SelectNavigationController::setSelectionRevision(int value) {
    if (m_selectionRevision == value) {
        return;
    }
    m_selectionRevision = value;
    emit selectionRevisionChanged();
}

int Lr2SelectNavigationController::focusRevision() const { return m_focusRevision; }
void Lr2SelectNavigationController::setFocusRevision(int value) {
    if (m_focusRevision == value) {
        return;
    }
    m_focusRevision = value;
    emit focusRevisionChanged();
}

int Lr2SelectNavigationController::scoreRevision() const { return m_scoreRevision; }
void Lr2SelectNavigationController::setScoreRevision(int value) {
    if (m_scoreRevision == value) {
        return;
    }
    m_scoreRevision = value;
    emit scoreRevisionChanged();
}

bool Lr2SelectNavigationController::suppressNextSelectionSound() const { return m_suppressNextSelectionSound; }
void Lr2SelectNavigationController::setSuppressNextSelectionSound(bool value) {
    if (m_suppressNextSelectionSound == value) {
        return;
    }
    m_suppressNextSelectionSound = value;
    emit suppressNextSelectionSoundChanged();
}

int Lr2SelectNavigationController::refreshedFocusedIndex() const { return m_refreshedFocusedIndex; }
void Lr2SelectNavigationController::setRefreshedFocusedIndex(int value) {
    if (m_refreshedFocusedIndex == value) {
        return;
    }
    m_refreshedFocusedIndex = value;
    emit refreshedFocusedIndexChanged();
}

int Lr2SelectNavigationController::refreshedFocusedScoreRevision() const { return m_refreshedFocusedScoreRevision; }
void Lr2SelectNavigationController::setRefreshedFocusedScoreRevision(int value) {
    if (m_refreshedFocusedScoreRevision == value) {
        return;
    }
    m_refreshedFocusedScoreRevision = value;
    emit refreshedFocusedScoreRevisionChanged();
}

int Lr2SelectNavigationController::refreshedFocusedListRevision() const { return m_refreshedFocusedListRevision; }
void Lr2SelectNavigationController::setRefreshedFocusedListRevision(int value) {
    if (m_refreshedFocusedListRevision == value) {
        return;
    }
    m_refreshedFocusedListRevision = value;
    emit refreshedFocusedListRevisionChanged();
}

bool Lr2SelectNavigationController::refreshedFocusedRankingMode() const { return m_refreshedFocusedRankingMode; }
void Lr2SelectNavigationController::setRefreshedFocusedRankingMode(bool value) {
    if (m_refreshedFocusedRankingMode == value) {
        return;
    }
    m_refreshedFocusedRankingMode = value;
    emit refreshedFocusedRankingModeChanged();
}

int Lr2SelectNavigationController::lastSyncedCursorBaseIndex() const { return m_lastSyncedCursorBaseIndex; }
void Lr2SelectNavigationController::setLastSyncedCursorBaseIndex(int value) {
    if (m_lastSyncedCursorBaseIndex == value) {
        return;
    }
    m_lastSyncedCursorBaseIndex = value;
    emit lastSyncedCursorBaseIndexChanged();
}

bool Lr2SelectNavigationController::rankingMode() const { return m_rankingMode; }
void Lr2SelectNavigationController::setRankingMode(bool value) {
    if (m_rankingMode == value) {
        return;
    }
    m_rankingMode = value;
    emit rankingModeChanged();
}

int Lr2SelectNavigationController::logicalCount() const { return m_logicalCount; }
void Lr2SelectNavigationController::setLogicalCount(int value) {
    value = std::max(0, value);
    if (m_logicalCount == value) {
        return;
    }
    m_logicalCount = value;
    emit logicalCountChanged();
}

int Lr2SelectNavigationController::lr2SpeedFirst() const { return m_lr2SpeedFirst; }
void Lr2SelectNavigationController::setLr2SpeedFirst(int value) {
    if (m_lr2SpeedFirst == value) {
        return;
    }
    m_lr2SpeedFirst = value;
    emit lr2SpeedFirstChanged();
}

int Lr2SelectNavigationController::lr2SpeedNext() const { return m_lr2SpeedNext; }
void Lr2SelectNavigationController::setLr2SpeedNext(int value) {
    if (m_lr2SpeedNext == value) {
        return;
    }
    m_lr2SpeedNext = value;
    emit lr2SpeedNextChanged();
}

int Lr2SelectNavigationController::lr2ScrollUp() const { return m_lr2ScrollUp; }
void Lr2SelectNavigationController::setLr2ScrollUp(int value) {
    if (m_lr2ScrollUp == value) {
        return;
    }
    m_lr2ScrollUp = value;
    emit lr2ScrollUpChanged();
}

int Lr2SelectNavigationController::lr2ScrollDown() const { return m_lr2ScrollDown; }
void Lr2SelectNavigationController::setLr2ScrollDown(int value) {
    if (m_lr2ScrollDown == value) {
        return;
    }
    m_lr2ScrollDown = value;
    emit lr2ScrollDownChanged();
}

bool Lr2SelectNavigationController::refreshFocusedState() {
    const int focusedIndex = m_logicalCount > 0 ? m_currentIndex : 0;
    if (m_refreshedFocusedIndex == focusedIndex
            && m_refreshedFocusedScoreRevision == m_scoreRevision
            && m_refreshedFocusedListRevision == m_listRevision
            && m_refreshedFocusedRankingMode == m_rankingMode) {
        return false;
    }

    setRefreshedFocusedIndex(focusedIndex);
    setRefreshedFocusedScoreRevision(m_scoreRevision);
    setRefreshedFocusedListRevision(m_listRevision);
    setRefreshedFocusedRankingMode(m_rankingMode);

    m_focusedStateRefreshResult = false;
    emit focusedStateRefreshRequested();
    if (m_focusedStateRefreshResult) {
        setFocusRevision(m_focusRevision + 1);
        return true;
    }
    return false;
}

bool Lr2SelectNavigationController::touchSelection() {
    refreshFocusedState();
    if (m_suppressNextSelectionSound) {
        setSuppressNextSelectionSound(false);
    } else {
        emitEntryChangeSoundsRequested(1);
    }
    setSelectionRevision(m_selectionRevision + 1);
    return true;
}

bool Lr2SelectNavigationController::commitLogicalSelection(int index) {
    if (m_logicalCount == 0) {
        return false;
    }

    const int normalized = normalizeIndex(index);
    setTargetIndex(normalized);
    if (m_currentIndex == normalized) {
        return false;
    }
    setCurrentIndex(normalized);
    touchSelection();
    return true;
}

bool Lr2SelectNavigationController::syncCurrentToVisual(int cursorBaseIndex) {
    Lr2SelectVisualState* visualState = m_visualState.data();
    if (!visualState || m_logicalCount == 0) {
        return false;
    }

    const int baseIndex = cursorBaseIndex >= 0 ? cursorBaseIndex : visualState->cursorBaseIndex();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const int nextIndex = (visualMoveActive() || static_cast<qreal>(now) < m_barMoveEndMs)
        ? normalizeIndex(m_targetIndex)
        : normalizeIndex(baseIndex + m_selectedOffset);
    if (m_currentIndex == nextIndex) {
        return false;
    }
    setCurrentIndex(nextIndex);
    setTargetIndex(nextIndex);
    touchSelection();
    return true;
}

void Lr2SelectNavigationController::applyLr2ScrollDelta(qreal entries, int durationMs, qreal nowMs, int currentFixed) {
    Lr2SelectVisualState* visualState = m_visualState.data();
    if (!visualState || m_logicalCount == 0 || entries == 0.0) {
        return;
    }

    if (nowMs < 0.0) {
        nowMs = static_cast<qreal>(QDateTime::currentMSecsSinceEpoch());
    }
    const int topbarFixed = currentFixed == std::numeric_limits<int>::min()
        ? animatedTopbarFixed()
        : currentFixed;
    setOldBarFixed(topbarFixed);

    const int roundedEntries = static_cast<int>(std::lround(entries));
    const int nextNowBarFixed = m_nowBarFixed + static_cast<int>(std::lround(entries * 1000.0));
    setNowBarFixed(nextNowBarFixed);
    setScrollDirection(entries < 0.0 ? m_lr2ScrollUp : m_lr2ScrollDown);

    const int nextIndex = normalizeIndex(static_cast<int>(std::lround(nextNowBarFixed / 1000.0)) + m_selectedOffset);
    setTargetIndex(nextIndex);
    emitEntryChangeSoundsRequested(std::abs(roundedEntries));
    setSuppressNextSelectionSound(true);
    const bool focusTouched = commitLogicalSelection(nextIndex);
    const bool selectionTouched = beginVisualMove(durationMs, nowMs);
    if (!selectionTouched && !focusTouched) {
        touchSelection();
    }
    setSuppressNextSelectionSound(false);
}

void Lr2SelectNavigationController::scrollBy(qreal entries, int durationMs) {
    if (m_logicalCount == 0 || entries == 0.0) {
        return;
    }
    const qreal now = static_cast<qreal>(QDateTime::currentMSecsSinceEpoch());
    const int duration = durationMs >= 0 ? durationMs : m_lr2SpeedFirst;
    applyLr2ScrollDelta(entries, duration, now, animatedTopbarFixed());
}

void Lr2SelectNavigationController::scrollByKey(qreal entries, bool repeated) {
    if (m_logicalCount == 0 || entries == 0.0) {
        return;
    }
    scrollBy(entries, repeated ? m_lr2SpeedNext : m_lr2SpeedFirst);
}

void Lr2SelectNavigationController::decrementViewIndex(bool repeated) {
    scrollByKey(-1, repeated);
}

void Lr2SelectNavigationController::incrementViewIndex(bool repeated) {
    scrollByKey(1, repeated);
}

void Lr2SelectNavigationController::completeFocusedStateRefresh(bool refreshed) {
    m_focusedStateRefreshResult = refreshed;
}

int Lr2SelectNavigationController::normalizeIndex(int index) const {
    return m_logicalCount > 0 ? (index % m_logicalCount + m_logicalCount) % m_logicalCount : 0;
}

int Lr2SelectNavigationController::animatedTopbarFixed() const {
    const Lr2SelectVisualState* visualState = m_visualState.data();
    return visualState ? visualState->rawFixed() : 0;
}

qreal Lr2SelectNavigationController::nearestVisualIndex(int index, qreal anchor) const {
    if (m_logicalCount == 0) {
        return 0.0;
    }
    qreal target = index;
    const qreal halfCount = m_logicalCount / 2.0;
    while (target - anchor > halfCount) {
        target -= m_logicalCount;
    }
    while (target - anchor < -halfCount) {
        target += m_logicalCount;
    }
    return target;
}

bool Lr2SelectNavigationController::visualMoveActive() const {
    const Lr2SelectVisualState* visualState = m_visualState.data();
    return visualState && std::abs(m_nowBarFixed - visualState->rawFixed()) > 0;
}

void Lr2SelectNavigationController::emitEntryChangeSoundsRequested(int count) {
    if (count <= 0) {
        return;
    }
    emit entryChangeSoundsRequested(count);
}

bool Lr2SelectNavigationController::beginVisualMove(int durationMs, qreal nowMs) {
    Lr2SelectVisualState* visualState = m_visualState.data();
    if (!visualState) {
        return false;
    }

    durationMs = std::max(1, durationMs);
    setBarMoveStartMs(nowMs);
    setBarMoveEndMs(nowMs + durationMs);

    setSuppressVisualIndexPublish(true);
    visualState->jumpTo(visualState->rawFixed() / 1000.0);
    const qreal nextTargetVisualIndex = m_nowBarFixed / 1000.0;
    setTargetVisualIndex(nextTargetVisualIndex);
    visualState->startAnimation(m_oldBarFixed / 1000.0,
                                nextTargetVisualIndex,
                                durationMs,
                                nowMs);
    setSuppressVisualIndexPublish(false);
    return publishCursorBaseIndex(false);
}

bool Lr2SelectNavigationController::publishCursorBaseIndex(bool force) {
    Lr2SelectVisualState* visualState = m_visualState.data();
    if (!visualState || !m_updatesActive || m_suppressVisualIndexPublish) {
        return false;
    }
    const int cursorBaseIndex = visualState->cursorBaseIndex();
    if (force || m_lastSyncedCursorBaseIndex != cursorBaseIndex) {
        setLastSyncedCursorBaseIndex(cursorBaseIndex);
        return syncCurrentToVisual(cursorBaseIndex);
    }
    return false;
}
