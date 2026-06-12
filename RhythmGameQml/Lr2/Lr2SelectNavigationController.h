#pragma once

#include "Lr2SelectVisualState.h"

#include <QObject>
#include <QMetaObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

#include <limits>

class Lr2SelectNavigationController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2SelectVisualState* visualState READ visualState WRITE setVisualState NOTIFY visualStateChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int targetIndex READ targetIndex WRITE setTargetIndex NOTIFY targetIndexChanged)
    Q_PROPERTY(int selectedOffset READ selectedOffset WRITE setSelectedOffset NOTIFY selectedOffsetChanged)
    Q_PROPERTY(qreal targetVisualIndex READ targetVisualIndex WRITE setTargetVisualIndex NOTIFY targetVisualIndexChanged)
    Q_PROPERTY(int oldBarFixed READ oldBarFixed WRITE setOldBarFixed NOTIFY oldBarFixedChanged)
    Q_PROPERTY(int nowBarFixed READ nowBarFixed WRITE setNowBarFixed NOTIFY nowBarFixedChanged)
    Q_PROPERTY(qreal barMoveStartMs READ barMoveStartMs WRITE setBarMoveStartMs NOTIFY barMoveStartMsChanged)
    Q_PROPERTY(qreal barMoveEndMs READ barMoveEndMs WRITE setBarMoveEndMs NOTIFY barMoveEndMsChanged)
    Q_PROPERTY(bool updatesActive READ updatesActive WRITE setUpdatesActive NOTIFY updatesActiveChanged)
    Q_PROPERTY(bool suppressVisualIndexPublish READ suppressVisualIndexPublish WRITE setSuppressVisualIndexPublish NOTIFY suppressVisualIndexPublishChanged)
    Q_PROPERTY(int scrollDirection READ scrollDirection WRITE setScrollDirection NOTIFY scrollDirectionChanged)
    Q_PROPERTY(bool suppressNextSelectionSound READ suppressNextSelectionSound WRITE setSuppressNextSelectionSound NOTIFY suppressNextSelectionSoundChanged)
    Q_PROPERTY(int lastSyncedCursorBaseIndex READ lastSyncedCursorBaseIndex WRITE setLastSyncedCursorBaseIndex NOTIFY lastSyncedCursorBaseIndexChanged)
    Q_PROPERTY(bool rankingMode READ rankingMode WRITE setRankingMode NOTIFY rankingModeChanged)
    Q_PROPERTY(int logicalCount READ logicalCount WRITE setLogicalCount NOTIFY logicalCountChanged)
    Q_PROPERTY(int lr2SpeedFirst READ lr2SpeedFirst WRITE setLr2SpeedFirst NOTIFY lr2SpeedFirstChanged)
    Q_PROPERTY(int lr2SpeedNext READ lr2SpeedNext WRITE setLr2SpeedNext NOTIFY lr2SpeedNextChanged)
    Q_PROPERTY(int lr2ScrollUp READ lr2ScrollUp WRITE setLr2ScrollUp NOTIFY lr2ScrollUpChanged)
    Q_PROPERTY(int lr2ScrollDown READ lr2ScrollDown WRITE setLr2ScrollDown NOTIFY lr2ScrollDownChanged)

public:
    explicit Lr2SelectNavigationController(QObject* parent = nullptr);

    Lr2SelectVisualState* visualState() const;
    void setVisualState(Lr2SelectVisualState* state);

    int currentIndex() const;
    void setCurrentIndex(int value);

    int targetIndex() const;
    void setTargetIndex(int value);

    int selectedOffset() const;
    void setSelectedOffset(int value);

    qreal targetVisualIndex() const;
    void setTargetVisualIndex(qreal value);

    int oldBarFixed() const;
    void setOldBarFixed(int value);

    int nowBarFixed() const;
    void setNowBarFixed(int value);

    qreal barMoveStartMs() const;
    void setBarMoveStartMs(qreal value);

    qreal barMoveEndMs() const;
    void setBarMoveEndMs(qreal value);

    bool updatesActive() const;
    void setUpdatesActive(bool value);

    bool suppressVisualIndexPublish() const;
    void setSuppressVisualIndexPublish(bool value);

    int scrollDirection() const;
    void setScrollDirection(int value);

    bool suppressNextSelectionSound() const;
    void setSuppressNextSelectionSound(bool value);

    int lastSyncedCursorBaseIndex() const;
    void setLastSyncedCursorBaseIndex(int value);

    bool rankingMode() const;
    void setRankingMode(bool value);

    int logicalCount() const;
    void setLogicalCount(int value);

    int lr2SpeedFirst() const;
    void setLr2SpeedFirst(int value);

    int lr2SpeedNext() const;
    void setLr2SpeedNext(int value);

    int lr2ScrollUp() const;
    void setLr2ScrollUp(int value);

    int lr2ScrollDown() const;
    void setLr2ScrollDown(int value);

    Q_INVOKABLE bool refreshFocusedState();
    Q_INVOKABLE bool touchSelection();
    Q_INVOKABLE bool commitLogicalSelection(int index);
    Q_INVOKABLE bool syncCurrentToVisual(int cursorBaseIndex = -1);
    Q_INVOKABLE bool publishCursorBaseIndex(bool force = false);
    Q_INVOKABLE void invalidateFocusedState();
    Q_INVOKABLE void applyLr2ScrollDelta(qreal entries, int durationMs, qreal nowMs = -1.0, int currentFixed = std::numeric_limits<int>::min());
    Q_INVOKABLE void scrollBy(qreal entries, int durationMs = -1);
    Q_INVOKABLE void scrollByKey(qreal entries, bool repeated);
    Q_INVOKABLE void decrementViewIndex(bool repeated);
    Q_INVOKABLE void incrementViewIndex(bool repeated);
    Q_INVOKABLE void completeFocusedStateRefresh(bool refreshed);

signals:
    void visualStateChanged();
    void currentIndexChanged();
    void targetIndexChanged();
    void selectedOffsetChanged();
    void targetVisualIndexChanged();
    void oldBarFixedChanged();
    void nowBarFixedChanged();
    void barMoveStartMsChanged();
    void barMoveEndMsChanged();
    void updatesActiveChanged();
    void suppressVisualIndexPublishChanged();
    void scrollDirectionChanged();
    void suppressNextSelectionSoundChanged();
    void lastSyncedCursorBaseIndexChanged();
    void rankingModeChanged();
    void logicalCountChanged();
    void lr2SpeedFirstChanged();
    void lr2SpeedNextChanged();
    void lr2ScrollUpChanged();
    void lr2ScrollDownChanged();
    void focusedStateRefreshRequested();
    void focusedStateChanged();
    void entryChangeSoundsRequested(int count);

private:
    int normalizeIndex(int index) const;
    int animatedTopbarFixed() const;
    qreal nearestVisualIndex(int index, qreal anchor) const;
    bool visualMoveActive() const;
    void emitEntryChangeSoundsRequested(int count);
    bool beginVisualMove(int durationMs, qreal nowMs);
    void connectVisualStateSignals();

    QPointer<Lr2SelectVisualState> m_visualState;
    QMetaObject::Connection m_visualCursorConnection;
    QMetaObject::Connection m_visualAnimationConnection;
    int m_currentIndex = 0;
    int m_targetIndex = 0;
    int m_selectedOffset = 0;
    qreal m_targetVisualIndex = 0.0;
    int m_oldBarFixed = 0;
    int m_nowBarFixed = 0;
    qreal m_barMoveStartMs = 0.0;
    qreal m_barMoveEndMs = 0.0;
    bool m_updatesActive = true;
    bool m_suppressVisualIndexPublish = false;
    int m_scrollDirection = 0;
    bool m_suppressNextSelectionSound = false;
    int m_cachedFocusedStateIndex = -1;
    bool m_cachedFocusedStateRankingMode = false;
    bool m_focusedStateDirty = false;
    int m_lastSyncedCursorBaseIndex = -1;
    bool m_rankingMode = false;
    int m_logicalCount = 0;
    int m_lr2SpeedFirst = 300;
    int m_lr2SpeedNext = 70;
    int m_lr2ScrollUp = 1;
    int m_lr2ScrollDown = 2;
    bool m_focusedStateRefreshResult = false;
};
