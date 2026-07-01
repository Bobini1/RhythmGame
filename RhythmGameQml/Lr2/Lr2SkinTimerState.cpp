#include "Lr2SkinTimerState.h"

#include "Lr2SkinClock.h"

#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace {
int toInt(const QVariant& value, int fallback = 0) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const int result = value.toInt(&ok);
    return ok ? result : fallback;
}

QHash<int, int> timerHashFromVariant(const QVariant& source) {
    QHash<int, int> result;
    if (!source.isValid() || source.isNull()) {
        return result;
    }

    const QVariantMap map = source.toMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        bool ok = false;
        const int key = it.key().toInt(&ok);
        if (ok) {
            result.insert(key, toInt(it.value(), -1));
        }
    }
    return result;
}

QVariantMap variantMapFromTimerHash(const QHash<int, int>& values) {
    QVariantMap result;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        result.insert(QString::number(it.key()), it.value());
    }
    return result;
}
}

Lr2SkinTimerState::Lr2SkinTimerState(QObject* parent) : QObject(parent) {
    m_cacheValues.fill(-1);
    m_cacheValid.fill(false);
}

Lr2SkinClock* Lr2SkinTimerState::clock() const { return m_clock; }
void Lr2SkinTimerState::setClock(Lr2SkinClock* clock) {
    if (m_clock == clock) {
        return;
    }
    if (m_clockSelectInfoConnection) {
        disconnect(m_clockSelectInfoConnection);
    }

    m_clock = clock;
    if (m_clock) {
        m_clockSelectInfoConnection = connect(
            m_clock,
            &Lr2SkinClock::selectInfoElapsedChanged,
            this,
            &Lr2SkinTimerState::syncSelectInfoElapsedFromClock);
        syncSelectInfoElapsedFromClock();
    } else {
        m_clockSelectInfoConnection = {};
    }
    emit clockChanged();
}

bool Lr2SkinTimerState::selectVisualMoveActive() const { return m_selectVisualMoveActive; }
void Lr2SkinTimerState::setSelectVisualMoveActive(bool active) {
    if (m_selectVisualMoveActive == active) {
        return;
    }
    m_selectVisualMoveActive = active;
    emit selectVisualMoveActiveChanged();
    clearSelectTimerFireCache();
}

bool Lr2SkinTimerState::selectScrollFixedPointDragging() const { return m_selectScrollFixedPointDragging; }
void Lr2SkinTimerState::setSelectScrollFixedPointDragging(bool dragging) {
    if (m_selectScrollFixedPointDragging == dragging) {
        return;
    }
    m_selectScrollFixedPointDragging = dragging;
    emit selectScrollFixedPointDraggingChanged();
    clearSelectTimerFireCache();
}

int Lr2SkinTimerState::selectScrollDirection() const { return m_selectScrollDirection; }
void Lr2SkinTimerState::setSelectScrollDirection(int direction) {
    if (m_selectScrollDirection == direction) {
        return;
    }
    m_selectScrollDirection = direction;
    emit selectScrollDirectionChanged();
    clearSelectTimerFireCache();
}

int Lr2SkinTimerState::selectScrollUp() const { return m_selectScrollUp; }
void Lr2SkinTimerState::setSelectScrollUp(int value) {
    if (m_selectScrollUp == value) {
        return;
    }
    m_selectScrollUp = value;
    emit selectScrollUpChanged();
    clearSelectTimerFireCache();
}

int Lr2SkinTimerState::selectScrollDown() const { return m_selectScrollDown; }
void Lr2SkinTimerState::setSelectScrollDown(int value) {
    if (m_selectScrollDown == value) {
        return;
    }
    m_selectScrollDown = value;
    emit selectScrollDownChanged();
    clearSelectTimerFireCache();
}

int Lr2SkinTimerState::gameplayRhythmTimerSkinTime() const { return m_gameplayRhythmTimerSkinTime; }
void Lr2SkinTimerState::setGameplayRhythmTimerSkinTime(int skinTime) {
    if (m_gameplayRhythmTimerSkinTime == skinTime) {
        return;
    }
    m_gameplayRhythmTimerSkinTime = skinTime;
    emit gameplayRhythmTimerSkinTimeChanged();
    notifyTimerFireTimesChanged();
}

QVariant Lr2SkinTimerState::selectHeldButtonTimerStarts() const {
    return variantMapFromTimerHash(m_selectHeldButtonTimerStarts);
}
void Lr2SkinTimerState::setSelectHeldButtonTimerStarts(const QVariant& values) {
    const QHash<int, int> next = timerHashFromVariant(values);
    if (m_selectHeldButtonTimerStarts == next) {
        return;
    }
    m_selectHeldButtonTimerStarts = next;
    emit selectHeldButtonTimerStartsChanged();
    clearSelectTimerFireCache();
}

QString Lr2SkinTimerState::screenKey() const { return m_screenKey; }
void Lr2SkinTimerState::setScreenKey(const QString& value) {
    if (m_screenKey == value) {
        return;
    }
    m_screenKey = value;
    emit screenKeyChanged();
    clearSelectTimerFireCache();
}

bool Lr2SkinTimerState::isGameplayScreen() const { return m_gameplayScreen; }
void Lr2SkinTimerState::setGameplayScreen(bool value) {
    if (m_gameplayScreen == value) {
        return;
    }
    m_gameplayScreen = value;
    emit gameplayScreenChanged();
    notifyTimerFireTimesChanged();
}

bool Lr2SkinTimerState::isResultScreen() const { return m_resultScreen; }
void Lr2SkinTimerState::setResultScreen(bool value) {
    if (m_resultScreen == value) {
        return;
    }
    m_resultScreen = value;
    emit resultScreenChanged();
    notifyTimerFireTimesChanged();
}

bool Lr2SkinTimerState::acceptsInput() const { return m_acceptsInput; }
void Lr2SkinTimerState::setAcceptsInput(bool value) {
    if (m_acceptsInput == value) {
        return;
    }
    m_acceptsInput = value;
    emit acceptsInputChanged();
    clearSelectTimerFireCache();
}

int Lr2SkinTimerState::startInput() const { return m_startInput; }
void Lr2SkinTimerState::setStartInput(int value) {
    if (setInt(m_startInput, value)) {
        emit startInputChanged();
        notifyTimerFireTimesChanged();
    }
}

int Lr2SkinTimerState::globalSkinTime() const { return m_globalSkinTime; }
void Lr2SkinTimerState::setGlobalSkinTime(int value) {
    if (setInt(m_globalSkinTime, value)) {
        emit globalSkinTimeChanged();
    }
}

int Lr2SkinTimerState::renderSkinTime() const { return m_renderSkinTime; }
void Lr2SkinTimerState::setRenderSkinTime(int value) {
    if (setInt(m_renderSkinTime, value)) {
        emit renderSkinTimeChanged();
        if (m_resultScreen || m_screenKey == QStringLiteral("select")) {
            notifyTimerFireTimesChanged();
        }
    }
}

int Lr2SkinTimerState::selectSourceSkinTime() const { return m_selectSourceSkinTime; }
void Lr2SkinTimerState::setSelectSourceSkinTime(int value) {
    if (setInt(m_selectSourceSkinTime, value)) {
        emit selectSourceSkinTimeChanged();
    }
}

int Lr2SkinTimerState::selectLiveSkinTime() const { return m_selectLiveSkinTime; }
void Lr2SkinTimerState::setSelectLiveSkinTime(int value) {
    if (setInt(m_selectLiveSkinTime, value)) {
        emit selectLiveSkinTimeChanged();
    }
}

int Lr2SkinTimerState::selectInfoElapsed() const { return m_selectInfoElapsed; }
void Lr2SkinTimerState::setSelectInfoElapsed(int value) {
    if (setInt(m_selectInfoElapsed, value)) {
        emit selectInfoElapsedChanged();
        ++m_cacheEpoch;
        notifySelectInfoTimerFireTimesChanged();
    }
}

void Lr2SkinTimerState::syncSelectInfoElapsedFromClock() {
    if (m_clock) {
        setSelectInfoElapsed(m_clock->selectInfoElapsed());
    }
}

int Lr2SkinTimerState::selectScrollStartSkinTime() const { return m_selectScrollStartSkinTime; }
void Lr2SkinTimerState::setSelectScrollStartSkinTime(int value) {
    if (setInt(m_selectScrollStartSkinTime, value)) {
        emit selectScrollStartSkinTimeChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::selectNoScrollStartSkinTime() const { return m_selectNoScrollStartSkinTime; }
void Lr2SkinTimerState::setSelectNoScrollStartSkinTime(int value) {
    if (setInt(m_selectNoScrollStartSkinTime, value)) {
        emit selectNoScrollStartSkinTimeChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::selectDatabaseLoadedSkinTime() const { return m_selectDatabaseLoadedSkinTime; }
void Lr2SkinTimerState::setSelectDatabaseLoadedSkinTime(int value) {
    if (setInt(m_selectDatabaseLoadedSkinTime, value)) {
        emit selectDatabaseLoadedSkinTimeChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::selectPanel() const { return m_selectPanel; }
void Lr2SkinTimerState::setSelectPanel(int value) {
    if (setInt(m_selectPanel, value)) {
        emit selectPanelChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::selectPanelStartSkinTime() const { return m_selectPanelStartSkinTime; }
void Lr2SkinTimerState::setSelectPanelStartSkinTime(int value) {
    if (setInt(m_selectPanelStartSkinTime, value)) {
        emit selectPanelStartSkinTimeChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::selectPanelClosing() const { return m_selectPanelClosing; }
void Lr2SkinTimerState::setSelectPanelClosing(int value) {
    if (setInt(m_selectPanelClosing, value)) {
        emit selectPanelClosingChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::selectPanelCloseStartSkinTime() const { return m_selectPanelCloseStartSkinTime; }
void Lr2SkinTimerState::setSelectPanelCloseStartSkinTime(int value) {
    if (setInt(m_selectPanelCloseStartSkinTime, value)) {
        emit selectPanelCloseStartSkinTimeChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::selectPanelHoldTime() const { return m_selectPanelHoldTime; }
void Lr2SkinTimerState::setSelectPanelHoldTime(int value) {
    if (setInt(m_selectPanelHoldTime, value)) {
        emit selectPanelHoldTimeChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::lr2ReadmeMode() const { return m_lr2ReadmeMode; }
void Lr2SkinTimerState::setLr2ReadmeMode(int value) {
    if (setInt(m_lr2ReadmeMode, value)) {
        emit lr2ReadmeModeChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::lr2ReadmeElapsed() const { return m_lr2ReadmeElapsed; }
void Lr2SkinTimerState::setLr2ReadmeElapsed(int value) {
    if (setInt(m_lr2ReadmeElapsed, value)) {
        emit lr2ReadmeElapsedChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::lr2RankingTransitionPhase() const { return m_lr2RankingTransitionPhase; }
void Lr2SkinTimerState::setLr2RankingTransitionPhase(int value) {
    if (setInt(m_lr2RankingTransitionPhase, value)) {
        emit lr2RankingTransitionPhaseChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::lr2RankingTransitionElapsed() const { return m_lr2RankingTransitionElapsed; }
void Lr2SkinTimerState::setLr2RankingTransitionElapsed(int value) {
    if (setInt(m_lr2RankingTransitionElapsed, value)) {
        emit lr2RankingTransitionElapsedChanged();
        clearSelectTimerFireCache();
    }
}

int Lr2SkinTimerState::resultTimer151SkinTime() const { return m_resultTimer151SkinTime; }
void Lr2SkinTimerState::setResultTimer151SkinTime(int value) {
    if (setInt(m_resultTimer151SkinTime, value)) {
        emit resultTimer151SkinTimeChanged();
        notifyTimerFireTimesChanged();
    }
}

int Lr2SkinTimerState::resultTimer152SkinTime() const { return m_resultTimer152SkinTime; }
void Lr2SkinTimerState::setResultTimer152SkinTime(int value) {
    if (setInt(m_resultTimer152SkinTime, value)) {
        emit resultTimer152SkinTimeChanged();
        notifyTimerFireTimesChanged();
    }
}

int Lr2SkinTimerState::resultGraphStartSkinTime() const { return m_resultGraphStartSkinTime; }
void Lr2SkinTimerState::setResultGraphStartSkinTime(int value) {
    if (setInt(m_resultGraphStartSkinTime, value)) {
        emit resultGraphStartSkinTimeChanged();
        notifyTimerFireTimesChanged();
    }
}

int Lr2SkinTimerState::resultGraphEndSkinTime() const { return m_resultGraphEndSkinTime; }
void Lr2SkinTimerState::setResultGraphEndSkinTime(int value) {
    if (setInt(m_resultGraphEndSkinTime, value)) {
        emit resultGraphEndSkinTimeChanged();
        notifyTimerFireTimesChanged();
    }
}

int Lr2SkinTimerState::gameplayTimerFireTime(int timer) const {
    if (timer == 1) {
        return startInputTimerFireTime();
    }
    if (timer == 140) {
        return m_gameplayRhythmTimerSkinTime;
    }
    return gameplayTimerValue(timer);
}

int Lr2SkinTimerState::resultTimerFireTime(int timer) const {
    if (timer == 0) {
        return 0;
    }
    if (timer == 1) {
        return startInputTimerFireTime();
    }
    if (timer == 150) {
        return m_renderSkinTime >= m_resultGraphStartSkinTime ? m_resultGraphStartSkinTime : -1;
    }
    if (timer == 151) {
        if (m_resultTimer151SkinTime >= 0) {
            return m_resultTimer151SkinTime;
        }
        return m_renderSkinTime >= m_resultGraphEndSkinTime ? m_resultGraphEndSkinTime : -1;
    }
    if (timer == 152) {
        return m_resultTimer152SkinTime >= 0 ? m_resultTimer152SkinTime : -1;
    }
    return -1;
}

int Lr2SkinTimerState::selectTimerFireTime(int timer, bool liveClock) const {
    const int baseTime = selectTimerBaseTime(liveClock);
    if (timer == 0) {
        return 0;
    }
    if (timer == 1) {
        return startInputTimerFireTime();
    }
    if (timer == 171) {
        return m_selectDatabaseLoadedSkinTime;
    }
    if (timer == 11) {
        return baseTime - m_selectInfoElapsed;
    }
    if (timer == 14 && m_acceptsInput) {
        return m_selectNoScrollStartSkinTime;
    }

    if (m_selectVisualMoveActive || m_selectScrollFixedPointDragging) {
        if (timer == 10) {
            return m_selectScrollStartSkinTime;
        }
        if (timer == 12 && m_selectScrollDirection == m_selectScrollUp) {
            return m_selectScrollStartSkinTime;
        }
        if (timer == 13 && m_selectScrollDirection == m_selectScrollDown) {
            return m_selectScrollStartSkinTime;
        }
    }

    if (timer >= 21 && timer <= 29) {
        const int panel = timer - 20;
        return m_selectPanel == panel
            ? baseTime - selectElapsedSince(m_selectPanelStartSkinTime)
            : -1;
    }
    if (timer >= 31 && timer <= 39) {
        const int panel = timer - 30;
        const int elapsed = selectElapsedSince(m_selectPanelCloseStartSkinTime);
        return m_selectPanelClosing == panel && elapsed < m_selectPanelHoldTime
            ? baseTime - elapsed
            : -1;
    }
    if (timer == 15 && m_lr2ReadmeMode == 1) {
        return baseTime - m_lr2ReadmeElapsed;
    }
    if (timer == 16 && m_lr2ReadmeMode == 2) {
        return baseTime - m_lr2ReadmeElapsed;
    }
    if (m_lr2RankingTransitionPhase != 0 && timer == m_lr2RankingTransitionPhase) {
        return baseTime - m_lr2RankingTransitionElapsed;
    }
    if (!isSelectHeldButtonTimerId(timer)) {
        return -1;
    }
    const auto it = m_selectHeldButtonTimerStarts.constFind(timer);
    if (it == m_selectHeldButtonTimerStarts.constEnd()) {
        return -1;
    }
    const int liveStart = it.value();
    return liveClock
        ? liveStart
        : m_renderSkinTime - std::max(0, m_selectLiveSkinTime - liveStart);
}

bool Lr2SkinTimerState::selectTimerCanFire(int timer) const {
    if (timer == 0 || timer == 1 || timer == 10 || timer == 11
            || timer == 12 || timer == 13 || timer == 14
            || timer == 15 || timer == 16 || timer == 171
            || timer == 175 || timer == 176) {
        return true;
    }
    if (timer >= 21 && timer <= 29) {
        return true;
    }
    if (timer >= 31 && timer <= 39) {
        return true;
    }
    return isSelectHeldButtonTimerId(timer);
}

bool Lr2SkinTimerState::skinTimerCanFire(int timer) const {
    if (timer == 0) {
        return true;
    }
    if (m_gameplayScreen) {
        return true;
    }
    if (m_resultScreen) {
        return timer == 1 || timer == 150 || timer == 151 || timer == 152;
    }
    if (m_screenKey == QStringLiteral("decide")) {
        return timer == 1;
    }
    if (m_screenKey == QStringLiteral("select")) {
        return selectTimerCanFire(timer);
    }
    return false;
}

int Lr2SkinTimerState::skinTimerFireTime(int timer, bool liveClock) {
    if (timer == 0) {
        return 0;
    }
    if (m_gameplayScreen) {
        return gameplayTimerFireTime(timer);
    }
    if (m_resultScreen) {
        return resultTimerFireTime(timer);
    }
    if (m_screenKey == QStringLiteral("decide")) {
        return timer == 1 ? startInputTimerFireTime() : -1;
    }
    if (m_screenKey == QStringLiteral("select")) {
        return canCacheSelectTimerFire(timer)
            ? cachedSelectTimerFireTime(timer, liveClock)
            : selectTimerFireTime(timer, liveClock);
    }
    return -1;
}

bool Lr2SkinTimerState::isSelectHeldButtonTimer(int timer) const {
    return isSelectHeldButtonTimerId(timer);
}

bool Lr2SkinTimerState::setGameplayTimerValue(int timer, int skinTime) {
    if (timer == 0 || skinTime < 0) {
        return false;
    }
    const auto it = m_gameplayTimerValues.constFind(timer);
    if (it != m_gameplayTimerValues.constEnd() && it.value() == skinTime) {
        return false;
    }
    m_gameplayTimerValues.insert(timer, skinTime);
    m_gameplayTimerValuesDirty = true;
    m_pendingGameplayTimerChanges.insert(timer);
    return true;
}

bool Lr2SkinTimerState::clearGameplayTimerValue(int timer) {
    if (timer == 0 || !m_gameplayTimerValues.contains(timer)) {
        return false;
    }
    m_gameplayTimerValues.remove(timer);
    m_gameplayTimerValuesDirty = true;
    m_pendingGameplayTimerChanges.insert(timer);
    return true;
}

bool Lr2SkinTimerState::resetGameplayTimerValues() {
    const QHash<int, int> next = initialGameplayTimerValues();
    if (m_gameplayTimerValues == next) {
        return false;
    }
    m_gameplayTimerValues = next;
    m_gameplayTimerValuesDirty = true;
    m_pendingGameplayTimerChanges.clear();
    m_pendingGameplayTimerFullRefresh = true;
    return commitGameplayTimerChanges();
}

bool Lr2SkinTimerState::commitGameplayTimerChanges() {
    if (!m_gameplayTimerValuesDirty) {
        return false;
    }
    m_committedGameplayTimerChanges = m_pendingGameplayTimerChanges;
    m_committedGameplayTimerFullRefresh = m_pendingGameplayTimerFullRefresh;
    m_pendingGameplayTimerChanges.clear();
    m_pendingGameplayTimerFullRefresh = false;
    m_gameplayTimerValuesDirty = false;
    emit gameplayTimerValuesCommitted();
    return true;
}

QSet<int> Lr2SkinTimerState::takeCommittedGameplayTimerChanges(bool* fullRefresh) {
    if (fullRefresh) {
        *fullRefresh = m_committedGameplayTimerFullRefresh;
    }
    m_committedGameplayTimerFullRefresh = false;
    QSet<int> result = m_committedGameplayTimerChanges;
    m_committedGameplayTimerChanges.clear();
    return result;
}

void Lr2SkinTimerState::clearSelectTimerFireCache() {
    ++m_cacheEpoch;
    notifyTimerFireTimesChanged();
}

int Lr2SkinTimerState::selectTimerBaseTime(bool liveClock) const {
    return liveClock ? m_selectSourceSkinTime : m_renderSkinTime;
}

int Lr2SkinTimerState::startInputTimerFireTime() const {
    return m_acceptsInput ? std::min(m_renderSkinTime, m_startInput) : -1;
}

int Lr2SkinTimerState::selectElapsedSince(int startSkinTime) const {
    return std::max(0, m_selectLiveSkinTime - startSkinTime);
}

bool Lr2SkinTimerState::canCacheSelectTimerFire(int timer) const {
    return timer == 10 || timer == 11 || timer == 12 || timer == 13 || timer == 171;
}

int Lr2SkinTimerState::cachedSelectTimerFireTime(int timer, bool liveClock) {
    const int cacheFrame = selectTimerCacheFrame(liveClock);
    if (m_cacheFrame != cacheFrame || m_cacheEpochSnapshot != m_cacheEpoch) {
        resetFrameCache(cacheFrame);
    }
    const int cacheIndex = cacheIndexForTimer(timer, liveClock);
    if (cacheIndex < 0) {
        return selectTimerFireTime(timer, liveClock);
    }
    const auto index = static_cast<std::array<int, 10>::size_type>(cacheIndex);
    if (m_cacheValid[index]) {
        return m_cacheValues[index];
    }
    const int value = selectTimerFireTime(timer, liveClock);
    m_cacheValues[index] = value;
    m_cacheValid[index] = true;
    return value;
}

bool Lr2SkinTimerState::isSelectHeldButtonTimerId(int timer) const {
    return (timer >= 101 && timer <= 107) || (timer >= 111 && timer <= 117);
}

int Lr2SkinTimerState::gameplayTimerValue(int timer) const {
    const auto it = m_gameplayTimerValues.constFind(timer);
    return it == m_gameplayTimerValues.constEnd() ? -1 : it.value();
}

QHash<int, int> Lr2SkinTimerState::initialGameplayTimerValues() const {
    QHash<int, int> result;
    result.insert(0, 0);
    for (int timer = 120; timer <= 127; ++timer) {
        result.insert(timer, 0);
    }
    for (int timer = 130; timer <= 137; ++timer) {
        result.insert(timer, 0);
    }
    return result;
}

int Lr2SkinTimerState::selectTimerCacheFrame(bool liveClock) const {
    return liveClock ? m_selectSourceSkinTime : m_renderSkinTime;
}

int Lr2SkinTimerState::cacheIndexForTimer(int timer, bool liveClock) const {
    int timerOffset = -1;
    switch (timer) {
    case 10:
        timerOffset = 0;
        break;
    case 11:
        timerOffset = 1;
        break;
    case 12:
        timerOffset = 2;
        break;
    case 13:
        timerOffset = 3;
        break;
    case 171:
        timerOffset = 4;
        break;
    default:
        return -1;
    }
    return (liveClock ? 5 : 0) + timerOffset;
}

void Lr2SkinTimerState::resetFrameCache(int cacheFrame) const {
    m_cacheFrame = cacheFrame;
    m_cacheEpochSnapshot = m_cacheEpoch;
    m_cacheValid.fill(false);
}

void Lr2SkinTimerState::notifyTimerFireTimesChanged() {
    emit timerFireTimesChanged();
}

void Lr2SkinTimerState::notifySelectInfoTimerFireTimesChanged() {
    emit selectInfoTimerFireTimesChanged();
}

bool Lr2SkinTimerState::setInt(int& field, int value) {
    if (field == value) {
        return false;
    }
    field = value;
    return true;
}
