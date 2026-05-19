#include "Lr2SelectNavigationController.h"

#include <QDateTime>
#include <QJSValue>
#include <QMetaProperty>

#include <algorithm>
#include <cmath>

namespace {

QVariant
gadgetProperty(const QVariant& source, const char* name)
{
    const QMetaObject* metaObject = source.metaType().metaObject();
    if (!metaObject) {
        return {};
    }
    const int propertyIndex = metaObject->indexOfProperty(name);
    if (propertyIndex < 0) {
        return {};
    }
    return metaObject->property(propertyIndex).readOnGadget(source.constData());
}

QVariant
valueProperty(const QVariant& source, const char* name)
{
    if (!source.isValid() || source.isNull()) {
        return {};
    }
    if (source.canConvert<QObject*>()) {
        if (QObject* object = source.value<QObject*>()) {
            return object->property(name);
        }
    }
    if (source.canConvert<QVariantMap>()) {
        const QVariantMap map = source.toMap();
        const auto it = map.constFind(QString::fromLatin1(name));
        return it == map.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QVariantHash>()) {
        const QVariantHash hash = source.toHash();
        const auto it = hash.constFind(QString::fromLatin1(name));
        return it == hash.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QJSValue>()) {
        const QJSValue value = source.value<QJSValue>();
        if (value.isObject()) {
            const QJSValue property = value.property(QString::fromLatin1(name));
            return property.isUndefined() || property.isNull()
                     ? QVariant()
                     : property.toVariant();
        }
    }
    return gadgetProperty(source, name);
}

QString
stringValue(const QVariant& value)
{
    return value.isValid() && !value.isNull() ? value.toString() : QString();
}

} // namespace

Lr2SelectNavigationController::Lr2SelectNavigationController(QObject* parent)
  : QObject(parent)
{
}

Lr2SelectVisualState*
Lr2SelectNavigationController::visualState() const
{
    return m_visualState;
}

void
Lr2SelectNavigationController::setVisualState(Lr2SelectVisualState* state)
{
    if (m_visualState == state) {
        return;
    }
    m_visualState = state;
    emit visualStateChanged();
}

Lr2SelectStateCache*
Lr2SelectNavigationController::stateCache() const
{
    return m_stateCache;
}

void
Lr2SelectNavigationController::setStateCache(Lr2SelectStateCache* cache)
{
    if (m_stateCache == cache) {
        return;
    }
    m_stateCache = cache;
    emit stateCacheChanged();
}

int
Lr2SelectNavigationController::logicalCount() const
{
    return m_logicalCount;
}

void
Lr2SelectNavigationController::setLogicalCount(int count)
{
    setValue(m_logicalCount,
             count,
             &Lr2SelectNavigationController::logicalCountChanged);
}

int
Lr2SelectNavigationController::focusedIndex() const
{
    return m_focusedIndex;
}

void
Lr2SelectNavigationController::setFocusedIndex(int index)
{
    setValue(m_focusedIndex,
             index,
             &Lr2SelectNavigationController::focusedIndexChanged);
}

QVariant
Lr2SelectNavigationController::focusedItem() const
{
    return m_focusedItem;
}

void
Lr2SelectNavigationController::setFocusedItem(const QVariant& item)
{
    setValue(
      m_focusedItem, item, &Lr2SelectNavigationController::focusedItemChanged);
}

int
Lr2SelectNavigationController::currentIndex() const
{
    return m_currentIndex;
}

void
Lr2SelectNavigationController::setCurrentIndex(int index)
{
    setValue(m_currentIndex,
             index,
             &Lr2SelectNavigationController::currentIndexChanged);
}

int
Lr2SelectNavigationController::targetIndex() const
{
    return m_targetIndex;
}

void
Lr2SelectNavigationController::setTargetIndex(int index)
{
    setValue(
      m_targetIndex, index, &Lr2SelectNavigationController::targetIndexChanged);
}

qreal
Lr2SelectNavigationController::targetVisualIndex() const
{
    return m_targetVisualIndex;
}

void
Lr2SelectNavigationController::setTargetVisualIndex(qreal index)
{
    setValue(m_targetVisualIndex,
             index,
             &Lr2SelectNavigationController::targetVisualIndexChanged);
}

int
Lr2SelectNavigationController::oldBarFixed() const
{
    return m_oldBarFixed;
}

void
Lr2SelectNavigationController::setOldBarFixed(int value)
{
    setValue(
      m_oldBarFixed, value, &Lr2SelectNavigationController::oldBarFixedChanged);
}

int
Lr2SelectNavigationController::nowBarFixed() const
{
    return m_nowBarFixed;
}

void
Lr2SelectNavigationController::setNowBarFixed(int value)
{
    setValue(
      m_nowBarFixed, value, &Lr2SelectNavigationController::nowBarFixedChanged);
}

int
Lr2SelectNavigationController::selectedOffset() const
{
    return m_selectedOffset;
}

void
Lr2SelectNavigationController::setSelectedOffset(int offset)
{
    setValue(m_selectedOffset,
             offset,
             &Lr2SelectNavigationController::selectedOffsetChanged);
}

qreal
Lr2SelectNavigationController::barMoveStartMs() const
{
    return m_barMoveStartMs;
}

void
Lr2SelectNavigationController::setBarMoveStartMs(qreal value)
{
    setValue(m_barMoveStartMs,
             value,
             &Lr2SelectNavigationController::barMoveStartMsChanged);
}

qreal
Lr2SelectNavigationController::barMoveEndMs() const
{
    return m_barMoveEndMs;
}

void
Lr2SelectNavigationController::setBarMoveEndMs(qreal value)
{
    setValue(m_barMoveEndMs,
             value,
             &Lr2SelectNavigationController::barMoveEndMsChanged);
}

bool
Lr2SelectNavigationController::updatesActive() const
{
    return m_updatesActive;
}

void
Lr2SelectNavigationController::setUpdatesActive(bool active)
{
    setValue(m_updatesActive,
             active,
             &Lr2SelectNavigationController::updatesActiveChanged);
}

bool
Lr2SelectNavigationController::suppressVisualIndexPublish() const
{
    return m_suppressVisualIndexPublish;
}

void
Lr2SelectNavigationController::setSuppressVisualIndexPublish(bool suppress)
{
    setValue(m_suppressVisualIndexPublish,
             suppress,
             &Lr2SelectNavigationController::suppressVisualIndexPublishChanged);
}

int
Lr2SelectNavigationController::scrollDirection() const
{
    return m_scrollDirection;
}

void
Lr2SelectNavigationController::setScrollDirection(int direction)
{
    setValue(m_scrollDirection,
             direction,
             &Lr2SelectNavigationController::scrollDirectionChanged);
}

int
Lr2SelectNavigationController::revision() const
{
    return m_revision;
}

void
Lr2SelectNavigationController::setRevision(int revision)
{
    setValue(
      m_revision, revision, &Lr2SelectNavigationController::revisionChanged);
}

int
Lr2SelectNavigationController::listRevision() const
{
    return m_listRevision;
}

void
Lr2SelectNavigationController::setListRevision(int revision)
{
    setValue(m_listRevision,
             revision,
             &Lr2SelectNavigationController::listRevisionChanged);
}

int
Lr2SelectNavigationController::selectionRevision() const
{
    return m_selectionRevision;
}

void
Lr2SelectNavigationController::setSelectionRevision(int revision)
{
    setValue(m_selectionRevision,
             revision,
             &Lr2SelectNavigationController::selectionRevisionChanged);
}

int
Lr2SelectNavigationController::focusRevision() const
{
    return m_focusRevision;
}

void
Lr2SelectNavigationController::setFocusRevision(int revision)
{
    setValue(m_focusRevision,
             revision,
             &Lr2SelectNavigationController::focusRevisionChanged);
}

int
Lr2SelectNavigationController::scoreRevision() const
{
    return m_scoreRevision;
}

void
Lr2SelectNavigationController::setScoreRevision(int revision)
{
    setValue(m_scoreRevision,
             revision,
             &Lr2SelectNavigationController::scoreRevisionChanged);
}

bool
Lr2SelectNavigationController::suppressNextSelectionSound() const
{
    return m_suppressNextSelectionSound;
}

void
Lr2SelectNavigationController::setSuppressNextSelectionSound(bool suppress)
{
    setValue(m_suppressNextSelectionSound,
             suppress,
             &Lr2SelectNavigationController::suppressNextSelectionSoundChanged);
}

int
Lr2SelectNavigationController::cachedSyncedCursorBaseIndex() const
{
    return m_cachedSyncedCursorBaseIndex;
}

void
Lr2SelectNavigationController::setCachedSyncedCursorBaseIndex(int index)
{
    setValue(
      m_cachedSyncedCursorBaseIndex,
      index,
      &Lr2SelectNavigationController::cachedSyncedCursorBaseIndexChanged);
}

bool
Lr2SelectNavigationController::rankingMode() const
{
    return m_rankingMode;
}

void
Lr2SelectNavigationController::setRankingMode(bool enabled)
{
    setValue(m_rankingMode,
             enabled,
             &Lr2SelectNavigationController::rankingModeChanged);
}

QVariant
Lr2SelectNavigationController::rankingBaseItem() const
{
    return m_rankingBaseItem;
}

void
Lr2SelectNavigationController::setRankingBaseItem(const QVariant& item)
{
    setValue(m_rankingBaseItem,
             item,
             &Lr2SelectNavigationController::rankingBaseItemChanged);
}

QVariant
Lr2SelectNavigationController::selectedScoreState() const
{
    return m_selectedScoreState;
}

void
Lr2SelectNavigationController::setSelectedScoreState(const QVariant& state)
{
    setValue(m_selectedScoreState,
             state,
             &Lr2SelectNavigationController::selectedScoreStateChanged);
}

QVariant
Lr2SelectNavigationController::visualChartWrapper() const
{
    return m_visualChartWrapper;
}

void
Lr2SelectNavigationController::setVisualChartWrapper(const QVariant& wrapper)
{
    setValue(m_visualChartWrapper,
             wrapper,
             &Lr2SelectNavigationController::visualChartWrapperChanged);
}

QString
Lr2SelectNavigationController::visualChartContentRevision() const
{
    return m_visualChartContentRevision;
}

void
Lr2SelectNavigationController::setVisualChartContentRevision(
  const QString& revision)
{
    setValue(m_visualChartContentRevision,
             revision,
             &Lr2SelectNavigationController::visualChartContentRevisionChanged);
}

QString
Lr2SelectNavigationController::visualStageFileSource() const
{
    return m_visualStageFileSource;
}

void
Lr2SelectNavigationController::setVisualStageFileSource(const QString& source)
{
    setValue(m_visualStageFileSource,
             source,
             &Lr2SelectNavigationController::visualStageFileSourceChanged);
}

QString
Lr2SelectNavigationController::visualBackBmpSource() const
{
    return m_visualBackBmpSource;
}

void
Lr2SelectNavigationController::setVisualBackBmpSource(const QString& source)
{
    setValue(m_visualBackBmpSource,
             source,
             &Lr2SelectNavigationController::visualBackBmpSourceChanged);
}

QString
Lr2SelectNavigationController::visualBannerSource() const
{
    return m_visualBannerSource;
}

void
Lr2SelectNavigationController::setVisualBannerSource(const QString& source)
{
    setValue(m_visualBannerSource,
             source,
             &Lr2SelectNavigationController::visualBannerSourceChanged);
}

int
Lr2SelectNavigationController::lr2SpeedFirst() const
{
    return 300;
}

int
Lr2SelectNavigationController::lr2SpeedNext() const
{
    return 70;
}

int
Lr2SelectNavigationController::lr2ScrollUp() const
{
    return 1;
}

int
Lr2SelectNavigationController::lr2ScrollDown() const
{
    return 2;
}

bool
Lr2SelectNavigationController::refreshFocusedState()
{
    if (m_refreshedFocusedIndex == m_focusedIndex &&
        m_refreshedFocusedItem == m_focusedItem &&
        m_refreshedFocusedScoreRevision == m_scoreRevision &&
        m_refreshedFocusedListRevision == m_listRevision &&
        m_refreshedFocusedRankingMode == m_rankingMode &&
        m_refreshedFocusedRankingBaseItem == m_rankingBaseItem) {
        return false;
    }

    m_refreshedFocusedIndex = m_focusedIndex;
    m_refreshedFocusedItem = m_focusedItem;
    m_refreshedFocusedScoreRevision = m_scoreRevision;
    m_refreshedFocusedListRevision = m_listRevision;
    m_refreshedFocusedRankingMode = m_rankingMode;
    m_refreshedFocusedRankingBaseItem = m_rankingBaseItem;

    if (refreshSelectedScoreState()) {
        setFocusRevision(m_focusRevision + 1);
        return true;
    }
    return false;
}

bool
Lr2SelectNavigationController::touchSelection()
{
    refreshFocusedState();
    if (m_suppressNextSelectionSound) {
        setSuppressNextSelectionSound(false);
    } else {
        emitEntryChangeSoundsRequested(1);
    }
    setSelectionRevision(m_selectionRevision + 1);
    setRevision(m_revision + 1);
    return true;
}

bool
Lr2SelectNavigationController::commitLogicalSelection(int index)
{
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

bool
Lr2SelectNavigationController::syncCurrentToVisual(int cursorBaseIndex)
{
    if (!m_visualState || m_logicalCount == 0) {
        return false;
    }

    const int baseIndex =
      cursorBaseIndex >= 0 ? cursorBaseIndex : m_visualState->cursorBaseIndex();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const int nextIndex = (visualMoveActive() || now < m_barMoveEndMs)
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

void
Lr2SelectNavigationController::applyLr2ScrollDelta(qreal entries,
                                                   int durationMs,
                                                   qreal nowMs,
                                                   int currentFixed)
{
    if (!m_visualState || m_logicalCount == 0 || entries == 0.0) {
        return;
    }

    if (nowMs < 0.0) {
        nowMs = QDateTime::currentMSecsSinceEpoch();
    }
    const int topbarFixed = currentFixed == std::numeric_limits<int>::min()
                              ? animatedTopbarFixed()
                              : currentFixed;
    setOldBarFixed(topbarFixed);

    const int roundedEntries = static_cast<int>(std::lround(entries));
    const int nextNowBarFixed =
      m_nowBarFixed + static_cast<int>(std::lround(entries * 1000.0));
    setNowBarFixed(nextNowBarFixed);
    setScrollDirection(entries < 0.0 ? lr2ScrollUp() : lr2ScrollDown());

    const int nextIndex =
      normalizeIndex(static_cast<int>(std::lround(nextNowBarFixed / 1000.0)) +
                     m_selectedOffset);
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

void
Lr2SelectNavigationController::scrollBy(qreal entries, int durationMs)
{
    if (m_logicalCount == 0 || entries == 0.0) {
        return;
    }
    const qreal now = QDateTime::currentMSecsSinceEpoch();
    const int duration = durationMs >= 0 ? durationMs : lr2SpeedFirst();
    applyLr2ScrollDelta(entries, duration, now, animatedTopbarFixed());
}

void
Lr2SelectNavigationController::scrollByKey(qreal entries, bool repeated)
{
    if (m_logicalCount == 0 || entries == 0.0) {
        return;
    }
    scrollBy(entries, repeated ? lr2SpeedNext() : lr2SpeedFirst());
}

void
Lr2SelectNavigationController::decrementViewIndex(bool repeated)
{
    scrollByKey(-1, repeated);
}

void
Lr2SelectNavigationController::incrementViewIndex(bool repeated)
{
    scrollByKey(1, repeated);
}

int
Lr2SelectNavigationController::normalizeIndex(int index) const
{
    return m_logicalCount > 0
             ? (index % m_logicalCount + m_logicalCount) % m_logicalCount
             : 0;
}

int
Lr2SelectNavigationController::animatedTopbarFixed() const
{
    return m_visualState ? m_visualState->rawFixed() : 0;
}

qreal
Lr2SelectNavigationController::nearestVisualIndex(int index, qreal anchor) const
{
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

bool
Lr2SelectNavigationController::visualMoveActive() const
{
    return m_visualState &&
           std::abs(m_nowBarFixed - m_visualState->rawFixed()) > 0;
}

void
Lr2SelectNavigationController::emitEntryChangeSoundsRequested(int count)
{
    if (count > 0) {
        emit entryChangeSoundsRequested(count);
    }
}

bool
Lr2SelectNavigationController::beginVisualMove(int durationMs, qreal nowMs)
{
    if (!m_visualState) {
        return false;
    }

    durationMs = std::max(1, durationMs);
    setBarMoveStartMs(nowMs);
    setBarMoveEndMs(nowMs + durationMs);

    setSuppressVisualIndexPublish(true);
    m_visualState->jumpTo(m_visualState->rawFixed() / 1000.0);
    const qreal targetIndex = m_nowBarFixed / 1000.0;
    setTargetVisualIndex(targetIndex);
    m_visualState->startAnimation(
      m_oldBarFixed / 1000.0, targetIndex, durationMs, nowMs);
    setSuppressVisualIndexPublish(false);
    return publishCursorBaseIndex(false);
}

bool
Lr2SelectNavigationController::publishCursorBaseIndex(bool force)
{
    if (!m_visualState || !m_updatesActive || m_suppressVisualIndexPublish) {
        return false;
    }
    const int cursorBaseIndex = m_visualState->cursorBaseIndex();
    if (force || m_cachedSyncedCursorBaseIndex != cursorBaseIndex) {
        setCachedSyncedCursorBaseIndex(cursorBaseIndex);
        return syncCurrentToVisual(cursorBaseIndex);
    }
    return false;
}

bool
Lr2SelectNavigationController::refreshSelectedScoreState()
{
    if (!m_stateCache) {
        return false;
    }

    const QVariantMap result =
      m_stateCache->refreshSelectedState(m_focusedItem,
                                         m_focusedIndex,
                                         m_scoreRevision,
                                         m_listRevision,
                                         m_rankingMode,
                                         m_rankingBaseItem);
    if (!result.value(QStringLiteral("changed")).toBool()) {
        return false;
    }
    applySelectedScoreState(result.value(QStringLiteral("state")).toMap());
    return true;
}

void
Lr2SelectNavigationController::applySelectedScoreState(const QVariantMap& state)
{
    setSelectedScoreState(state);

    const QVariant chartData = state.value(QStringLiteral("chartData"));
    setVisualChartWrapper(state.value(QStringLiteral("chartWrapper")));
    setVisualStageFileSource(
      chartAssetUrl(chartData, valueProperty(chartData, "stageFile")));
    setVisualBackBmpSource(
      chartAssetUrl(chartData, valueProperty(chartData, "backBmp")));
    setVisualBannerSource(
      chartAssetUrl(chartData, valueProperty(chartData, "banner")));
    setVisualChartContentRevision(
      state.value(QStringLiteral("contentRevision")).toString());
}

QString
Lr2SelectNavigationController::chartAssetUrl(const QVariant& chartData,
                                             const QVariant& fileName) const
{
    const QString file = stringValue(fileName);
    QString dir = stringValue(valueProperty(chartData, "chartDirectory"));
    if (dir.isEmpty() || file.isEmpty()) {
        return {};
    }

    dir.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (!dir.startsWith(QLatin1Char('/'))) {
        dir.prepend(QLatin1Char('/'));
    }

    QString stem = file;
    const int slash = std::max(stem.lastIndexOf(QLatin1Char('/')),
                               stem.lastIndexOf(QLatin1Char('\\')));
    const int dot = stem.lastIndexOf(QLatin1Char('.'));
    if (dot > slash) {
        stem.truncate(dot);
    }
    return QStringLiteral("file://") + dir + stem;
}
