#pragma once

#include "Lr2SelectStateCache.h"
#include "Lr2SelectVisualState.h"

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <limits>
#include <utility>

class Lr2SelectNavigationController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2SelectVisualState* visualState READ visualState WRITE
                 setVisualState NOTIFY visualStateChanged)
    Q_PROPERTY(Lr2SelectStateCache* stateCache READ stateCache WRITE
                 setStateCache NOTIFY stateCacheChanged)

    Q_PROPERTY(int logicalCount READ logicalCount WRITE setLogicalCount NOTIFY
                 logicalCountChanged)
    Q_PROPERTY(int focusedIndex READ focusedIndex WRITE setFocusedIndex NOTIFY
                 focusedIndexChanged)
    Q_PROPERTY(QVariant focusedItem READ focusedItem WRITE setFocusedItem NOTIFY
                 focusedItemChanged)

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                 currentIndexChanged)
    Q_PROPERTY(int targetIndex READ targetIndex WRITE setTargetIndex NOTIFY
                 targetIndexChanged)
    Q_PROPERTY(qreal targetVisualIndex READ targetVisualIndex WRITE
                 setTargetVisualIndex NOTIFY targetVisualIndexChanged)
    Q_PROPERTY(int oldBarFixed READ oldBarFixed WRITE setOldBarFixed NOTIFY
                 oldBarFixedChanged)
    Q_PROPERTY(int nowBarFixed READ nowBarFixed WRITE setNowBarFixed NOTIFY
                 nowBarFixedChanged)
    Q_PROPERTY(int selectedOffset READ selectedOffset WRITE setSelectedOffset
                 NOTIFY selectedOffsetChanged)
    Q_PROPERTY(qreal barMoveStartMs READ barMoveStartMs WRITE setBarMoveStartMs
                 NOTIFY barMoveStartMsChanged)
    Q_PROPERTY(qreal barMoveEndMs READ barMoveEndMs WRITE setBarMoveEndMs NOTIFY
                 barMoveEndMsChanged)
    Q_PROPERTY(bool updatesActive READ updatesActive WRITE setUpdatesActive
                 NOTIFY updatesActiveChanged)
    Q_PROPERTY(
      bool suppressVisualIndexPublish READ suppressVisualIndexPublish WRITE
        setSuppressVisualIndexPublish NOTIFY suppressVisualIndexPublishChanged)
    Q_PROPERTY(int scrollDirection READ scrollDirection WRITE setScrollDirection
                 NOTIFY scrollDirectionChanged)

    Q_PROPERTY(
      int revision READ revision WRITE setRevision NOTIFY revisionChanged)
    Q_PROPERTY(int listRevision READ listRevision WRITE setListRevision NOTIFY
                 listRevisionChanged)
    Q_PROPERTY(int selectionRevision READ selectionRevision WRITE
                 setSelectionRevision NOTIFY selectionRevisionChanged)
    Q_PROPERTY(int focusRevision READ focusRevision WRITE setFocusRevision
                 NOTIFY focusRevisionChanged)
    Q_PROPERTY(int scoreRevision READ scoreRevision WRITE setScoreRevision
                 NOTIFY scoreRevisionChanged)

    Q_PROPERTY(
      bool suppressNextSelectionSound READ suppressNextSelectionSound WRITE
        setSuppressNextSelectionSound NOTIFY suppressNextSelectionSoundChanged)
    Q_PROPERTY(int cachedSyncedCursorBaseIndex READ cachedSyncedCursorBaseIndex
                 WRITE setCachedSyncedCursorBaseIndex NOTIFY
                   cachedSyncedCursorBaseIndexChanged)
    Q_PROPERTY(bool rankingMode READ rankingMode WRITE setRankingMode NOTIFY
                 rankingModeChanged)
    Q_PROPERTY(QVariant rankingBaseItem READ rankingBaseItem WRITE
                 setRankingBaseItem NOTIFY rankingBaseItemChanged)

    Q_PROPERTY(QVariant selectedScoreState READ selectedScoreState WRITE
                 setSelectedScoreState NOTIFY selectedScoreStateChanged)
    Q_PROPERTY(QVariant visualChartWrapper READ visualChartWrapper WRITE
                 setVisualChartWrapper NOTIFY visualChartWrapperChanged)
    Q_PROPERTY(
      QString visualChartContentRevision READ visualChartContentRevision WRITE
        setVisualChartContentRevision NOTIFY visualChartContentRevisionChanged)
    Q_PROPERTY(QString visualStageFileSource READ visualStageFileSource WRITE
                 setVisualStageFileSource NOTIFY visualStageFileSourceChanged)
    Q_PROPERTY(QString visualBackBmpSource READ visualBackBmpSource WRITE
                 setVisualBackBmpSource NOTIFY visualBackBmpSourceChanged)
    Q_PROPERTY(QString visualBannerSource READ visualBannerSource WRITE
                 setVisualBannerSource NOTIFY visualBannerSourceChanged)

    Q_PROPERTY(int lr2SpeedFirst READ lr2SpeedFirst CONSTANT)
    Q_PROPERTY(int lr2SpeedNext READ lr2SpeedNext CONSTANT)
    Q_PROPERTY(int lr2ScrollUp READ lr2ScrollUp CONSTANT)
    Q_PROPERTY(int lr2ScrollDown READ lr2ScrollDown CONSTANT)

  public:
    explicit Lr2SelectNavigationController(QObject* parent = nullptr);

    Lr2SelectVisualState* visualState() const;
    void setVisualState(Lr2SelectVisualState* state);

    Lr2SelectStateCache* stateCache() const;
    void setStateCache(Lr2SelectStateCache* cache);

    int logicalCount() const;
    void setLogicalCount(int count);

    int focusedIndex() const;
    void setFocusedIndex(int index);

    QVariant focusedItem() const;
    void setFocusedItem(const QVariant& item);

    int currentIndex() const;
    void setCurrentIndex(int index);

    int targetIndex() const;
    void setTargetIndex(int index);

    qreal targetVisualIndex() const;
    void setTargetVisualIndex(qreal index);

    int oldBarFixed() const;
    void setOldBarFixed(int fixed);

    int nowBarFixed() const;
    void setNowBarFixed(int fixed);

    int selectedOffset() const;
    void setSelectedOffset(int offset);

    qreal barMoveStartMs() const;
    void setBarMoveStartMs(qreal value);

    qreal barMoveEndMs() const;
    void setBarMoveEndMs(qreal value);

    bool updatesActive() const;
    void setUpdatesActive(bool active);

    bool suppressVisualIndexPublish() const;
    void setSuppressVisualIndexPublish(bool suppress);

    int scrollDirection() const;
    void setScrollDirection(int direction);

    int revision() const;
    void setRevision(int revision);

    int listRevision() const;
    void setListRevision(int revision);

    int selectionRevision() const;
    void setSelectionRevision(int revision);

    int focusRevision() const;
    void setFocusRevision(int revision);

    int scoreRevision() const;
    void setScoreRevision(int revision);

    bool suppressNextSelectionSound() const;
    void setSuppressNextSelectionSound(bool suppress);

    int cachedSyncedCursorBaseIndex() const;
    void setCachedSyncedCursorBaseIndex(int index);

    bool rankingMode() const;
    void setRankingMode(bool enabled);

    QVariant rankingBaseItem() const;
    void setRankingBaseItem(const QVariant& item);

    QVariant selectedScoreState() const;
    void setSelectedScoreState(const QVariant& state);

    QVariant visualChartWrapper() const;
    void setVisualChartWrapper(const QVariant& wrapper);

    QString visualChartContentRevision() const;
    void setVisualChartContentRevision(const QString& revision);

    QString visualStageFileSource() const;
    void setVisualStageFileSource(const QString& source);

    QString visualBackBmpSource() const;
    void setVisualBackBmpSource(const QString& source);

    QString visualBannerSource() const;
    void setVisualBannerSource(const QString& source);

    int lr2SpeedFirst() const;
    int lr2SpeedNext() const;
    int lr2ScrollUp() const;
    int lr2ScrollDown() const;

    Q_INVOKABLE bool refreshFocusedState();
    Q_INVOKABLE bool touchSelection();
    Q_INVOKABLE bool commitLogicalSelection(int index);
    Q_INVOKABLE bool syncCurrentToVisual(int cursorBaseIndex = -1);
    Q_INVOKABLE void applyLr2ScrollDelta(
      qreal entries,
      int durationMs,
      qreal nowMs = -1.0,
      int currentFixed = std::numeric_limits<int>::min());
    Q_INVOKABLE void scrollBy(qreal entries, int durationMs = -1);
    Q_INVOKABLE void scrollByKey(qreal entries, bool repeated);
    Q_INVOKABLE void decrementViewIndex(bool repeated);
    Q_INVOKABLE void incrementViewIndex(bool repeated);

  signals:
    void visualStateChanged();
    void stateCacheChanged();
    void logicalCountChanged();
    void focusedIndexChanged();
    void focusedItemChanged();
    void currentIndexChanged();
    void targetIndexChanged();
    void targetVisualIndexChanged();
    void oldBarFixedChanged();
    void nowBarFixedChanged();
    void selectedOffsetChanged();
    void barMoveStartMsChanged();
    void barMoveEndMsChanged();
    void updatesActiveChanged();
    void suppressVisualIndexPublishChanged();
    void scrollDirectionChanged();
    void revisionChanged();
    void listRevisionChanged();
    void selectionRevisionChanged();
    void focusRevisionChanged();
    void scoreRevisionChanged();
    void suppressNextSelectionSoundChanged();
    void cachedSyncedCursorBaseIndexChanged();
    void rankingModeChanged();
    void rankingBaseItemChanged();
    void selectedScoreStateChanged();
    void visualChartWrapperChanged();
    void visualChartContentRevisionChanged();
    void visualStageFileSourceChanged();
    void visualBackBmpSourceChanged();
    void visualBannerSourceChanged();
    void entryChangeSoundsRequested(int count);

  private:
    template<typename T>
    bool setValue(T& field,
                  T value,
                  void (Lr2SelectNavigationController::*changed)())
    {
        if (field == value) {
            return false;
        }
        field = std::move(value);
        (this->*changed)();
        return true;
    }

    int normalizeIndex(int index) const;
    int animatedTopbarFixed() const;
    qreal nearestVisualIndex(int index, qreal anchor) const;
    bool visualMoveActive() const;
    void emitEntryChangeSoundsRequested(int count);
    bool beginVisualMove(int durationMs, qreal nowMs);
    bool publishCursorBaseIndex(bool force);
    bool refreshSelectedScoreState();
    void applySelectedScoreState(const QVariantMap& state);
    QString chartAssetUrl(const QVariant& chartData,
                          const QVariant& fileName) const;

    QPointer<Lr2SelectVisualState> m_visualState;
    QPointer<Lr2SelectStateCache> m_stateCache;

    int m_logicalCount = 0;
    int m_focusedIndex = 0;
    QVariant m_focusedItem;
    int m_currentIndex = 0;
    int m_targetIndex = 0;
    qreal m_targetVisualIndex = 0.0;
    int m_oldBarFixed = 0;
    int m_nowBarFixed = 0;
    int m_selectedOffset = 0;
    qreal m_barMoveStartMs = 0.0;
    qreal m_barMoveEndMs = 0.0;
    bool m_updatesActive = true;
    bool m_suppressVisualIndexPublish = false;
    int m_scrollDirection = 0;
    int m_revision = 0;
    int m_listRevision = 0;
    int m_selectionRevision = 0;
    int m_focusRevision = 0;
    int m_scoreRevision = 0;
    bool m_suppressNextSelectionSound = false;
    int m_cachedSyncedCursorBaseIndex = -1;
    bool m_rankingMode = false;
    QVariant m_rankingBaseItem;
    QVariant m_selectedScoreState;
    QVariant m_visualChartWrapper;
    QString m_visualChartContentRevision;
    QString m_visualStageFileSource;
    QString m_visualBackBmpSource;
    QString m_visualBannerSource;

    int m_refreshedFocusedIndex = -1;
    QVariant m_refreshedFocusedItem;
    int m_refreshedFocusedScoreRevision = -1;
    int m_refreshedFocusedListRevision = -1;
    bool m_refreshedFocusedRankingMode = false;
    QVariant m_refreshedFocusedRankingBaseItem;
};
