#include "Lr2SelectNavigationController.h"

#include <QDateTime>

#include <algorithm>
#include <cmath>
#include <limits>

Lr2SelectNavigationController::Lr2SelectNavigationController(QObject* parent) : QObject(parent) {}

QObject* Lr2SelectNavigationController::context() const {
    return m_context;
}

void Lr2SelectNavigationController::setContext(QObject* context) {
    if (m_context == context) {
        return;
    }
    m_context = context;
    emit contextChanged();
}

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

bool Lr2SelectNavigationController::refreshFocusedState() {
    if (!m_context) {
        return false;
    }

    const int focusedIndex = intProperty("focusedIndex");
    const int scoreRevision = intProperty("scoreRevision");
    const int listRevision = intProperty("listRevision");
    const bool rankingMode = boolProperty("rankingMode");

    if (intProperty("refreshedFocusedIndex", -1) == focusedIndex
            && intProperty("refreshedFocusedScoreRevision", -1) == scoreRevision
            && intProperty("refreshedFocusedListRevision", -1) == listRevision
            && boolProperty("refreshedFocusedRankingMode") == rankingMode) {
        return false;
    }

    setPropertyIfChanged("refreshedFocusedIndex", focusedIndex);
    setPropertyIfChanged("refreshedFocusedScoreRevision", scoreRevision);
    setPropertyIfChanged("refreshedFocusedListRevision", listRevision);
    setPropertyIfChanged("refreshedFocusedRankingMode", rankingMode);

    bool refreshed = false;
    QVariant result;
    if (QMetaObject::invokeMethod(m_context, "refreshSelectedScoreState", Q_RETURN_ARG(QVariant, result))) {
        refreshed = result.toBool();
    }
    if (refreshed) {
        incrementProperty("focusRevision");
        return true;
    }
    return false;
}

bool Lr2SelectNavigationController::touchSelection() {
    if (!m_context) {
        return false;
    }

    refreshFocusedState();
    if (boolProperty("suppressNextSelectionSound")) {
        setPropertyIfChanged("suppressNextSelectionSound", false);
    } else {
        emitEntryChangeSoundsRequested(1);
    }
    incrementProperty("selectionRevision");
    return true;
}

bool Lr2SelectNavigationController::commitLogicalSelection(int index) {
    if (!m_context || logicalCount() == 0) {
        return false;
    }

    const int normalized = normalizeIndex(index);
    setPropertyIfChanged("targetIndex", normalized);
    if (intProperty("currentIndex") == normalized) {
        return false;
    }
    setPropertyIfChanged("currentIndex", normalized);
    touchSelection();
    return true;
}

bool Lr2SelectNavigationController::syncCurrentToVisual(int cursorBaseIndex) {
    if (!m_context || !m_visualState || logicalCount() == 0) {
        return false;
    }

    const int baseIndex = cursorBaseIndex >= 0 ? cursorBaseIndex : m_visualState->cursorBaseIndex();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const int nextIndex = (visualMoveActive() || now < realProperty("barMoveEndMs"))
        ? normalizeIndex(intProperty("targetIndex"))
        : normalizeIndex(baseIndex + intProperty("selectedOffset"));
    if (intProperty("currentIndex") == nextIndex) {
        return false;
    }
    setPropertyIfChanged("currentIndex", nextIndex);
    setPropertyIfChanged("targetIndex", nextIndex);
    touchSelection();
    return true;
}

void Lr2SelectNavigationController::applyLr2ScrollDelta(qreal entries, int durationMs, qreal nowMs, int currentFixed) {
    if (!m_context || !m_visualState || logicalCount() == 0 || entries == 0.0) {
        return;
    }

    if (nowMs < 0.0) {
        nowMs = QDateTime::currentMSecsSinceEpoch();
    }
    const int topbarFixed = currentFixed == std::numeric_limits<int>::min()
        ? animatedTopbarFixed()
        : currentFixed;
    setPropertyIfChanged("oldBarFixed", topbarFixed);

    const int roundedEntries = static_cast<int>(std::lround(entries));
    const int nextNowBarFixed = intProperty("nowBarFixed") + static_cast<int>(std::lround(entries * 1000.0));
    setPropertyIfChanged("nowBarFixed", nextNowBarFixed);
    setPropertyIfChanged("scrollDirection", entries < 0.0
        ? intProperty("lr2ScrollUp")
        : intProperty("lr2ScrollDown"));

    const int nextIndex = normalizeIndex(static_cast<int>(std::lround(nextNowBarFixed / 1000.0)) + intProperty("selectedOffset"));
    setPropertyIfChanged("targetIndex", nextIndex);
    emitEntryChangeSoundsRequested(std::abs(roundedEntries));
    setPropertyIfChanged("suppressNextSelectionSound", true);
    const bool focusTouched = commitLogicalSelection(nextIndex);
    const bool selectionTouched = beginVisualMove(durationMs, nowMs);
    if (!selectionTouched && !focusTouched) {
        touchSelection();
    }
    setPropertyIfChanged("suppressNextSelectionSound", false);
}

void Lr2SelectNavigationController::scrollBy(qreal entries, int durationMs) {
    if (!m_context || logicalCount() == 0 || entries == 0.0) {
        return;
    }
    const qreal now = QDateTime::currentMSecsSinceEpoch();
    const int duration = durationMs >= 0 ? durationMs : intProperty("lr2SpeedFirst", 300);
    applyLr2ScrollDelta(entries, duration, now, animatedTopbarFixed());
}

void Lr2SelectNavigationController::scrollByKey(qreal entries, bool repeated) {
    if (!m_context || logicalCount() == 0 || entries == 0.0) {
        return;
    }
    scrollBy(entries, repeated ? intProperty("lr2SpeedNext", 70) : intProperty("lr2SpeedFirst", 300));
}

void Lr2SelectNavigationController::decrementViewIndex(bool repeated) {
    scrollByKey(-1, repeated);
}

void Lr2SelectNavigationController::incrementViewIndex(bool repeated) {
    scrollByKey(1, repeated);
}

int Lr2SelectNavigationController::intProperty(const char* name, int fallback) const {
    if (!m_context) {
        return fallback;
    }
    bool ok = false;
    const int value = m_context->property(name).toInt(&ok);
    return ok ? value : fallback;
}

qreal Lr2SelectNavigationController::realProperty(const char* name, qreal fallback) const {
    if (!m_context) {
        return fallback;
    }
    bool ok = false;
    const qreal value = m_context->property(name).toReal(&ok);
    return ok ? value : fallback;
}

bool Lr2SelectNavigationController::boolProperty(const char* name, bool fallback) const {
    if (!m_context) {
        return fallback;
    }
    const QVariant value = m_context->property(name);
    return value.isValid() ? value.toBool() : fallback;
}

void Lr2SelectNavigationController::setPropertyIfChanged(const char* name, const QVariant& value) const {
    if (!m_context || m_context->property(name) == value) {
        return;
    }
    m_context->setProperty(name, value);
}

void Lr2SelectNavigationController::incrementProperty(const char* name) const {
    setPropertyIfChanged(name, intProperty(name) + 1);
}

int Lr2SelectNavigationController::logicalCount() const {
    return intProperty("logicalCount");
}

int Lr2SelectNavigationController::normalizeIndex(int index) const {
    const int count = logicalCount();
    return count > 0 ? (index % count + count) % count : 0;
}

int Lr2SelectNavigationController::animatedTopbarFixed() const {
    return m_visualState ? m_visualState->rawFixed() : 0;
}

qreal Lr2SelectNavigationController::nearestVisualIndex(int index, qreal anchor) const {
    const int count = logicalCount();
    if (count == 0) {
        return 0.0;
    }
    qreal target = index;
    const qreal halfCount = count / 2.0;
    while (target - anchor > halfCount) {
        target -= count;
    }
    while (target - anchor < -halfCount) {
        target += count;
    }
    return target;
}

bool Lr2SelectNavigationController::visualMoveActive() const {
    return std::abs(intProperty("nowBarFixed") - intProperty("listTopbarFixed")) > 0;
}

void Lr2SelectNavigationController::emitEntryChangeSoundsRequested(int count) const {
    if (!m_context || count <= 0) {
        return;
    }
    QMetaObject::invokeMethod(m_context, "entryChangeSoundsRequested", Q_ARG(int, count));
}

bool Lr2SelectNavigationController::beginVisualMove(int durationMs, qreal nowMs) {
    if (!m_context || !m_visualState) {
        return false;
    }

    durationMs = std::max(1, durationMs);
    setPropertyIfChanged("barMoveStartMs", nowMs);
    setPropertyIfChanged("barMoveEndMs", nowMs + durationMs);

    setPropertyIfChanged("suppressVisualIndexPublish", true);
    m_visualState->jumpTo(intProperty("listTopbarFixed") / 1000.0);
    const qreal targetVisualIndex = intProperty("nowBarFixed") / 1000.0;
    setPropertyIfChanged("targetVisualIndex", targetVisualIndex);
    m_visualState->startAnimation(intProperty("oldBarFixed") / 1000.0,
                                  targetVisualIndex,
                                  durationMs,
                                  nowMs);
    setPropertyIfChanged("suppressVisualIndexPublish", false);
    return publishCursorBaseIndex(false);
}

bool Lr2SelectNavigationController::publishCursorBaseIndex(bool force) {
    if (!m_context || !m_visualState || !boolProperty("updatesActive", true) || boolProperty("suppressVisualIndexPublish")) {
        return false;
    }
    const int cursorBaseIndex = m_visualState->cursorBaseIndex();
    if (force || intProperty("lastSyncedCursorBaseIndex", -1) != cursorBaseIndex) {
        setPropertyIfChanged("lastSyncedCursorBaseIndex", cursorBaseIndex);
        return syncCurrentToVisual(cursorBaseIndex);
    }
    return false;
}

