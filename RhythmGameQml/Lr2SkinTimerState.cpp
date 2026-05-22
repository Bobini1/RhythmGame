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

bool toBool(const QVariant& value) {
    return value.isValid() && !value.isNull() && value.toBool();
}

QVariant objectProperty(QObject* object, const char* name) {
    return object ? object->property(name) : QVariant();
}

QHash<int, int> gameplayTimerHashFromVariant(const QVariant& source) {
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

QVariantMap variantMapFromGameplayTimerHash(const QHash<int, int>& values) {
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

QObject* Lr2SkinTimerState::host() const { return m_host; }
void Lr2SkinTimerState::setHost(QObject* host) {
    if (m_host == host) {
        return;
    }
    m_host = host;
    emit hostChanged();
    clearSelectTimerFireCache();
    bumpRevision();
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

QObject* Lr2SkinTimerState::selectContext() const { return m_selectContext; }
void Lr2SkinTimerState::setSelectContext(QObject* context) {
    if (m_selectContext == context) {
        return;
    }

    if (m_visualMoveConnection) {
        disconnect(m_visualMoveConnection);
    }
    if (m_dragConnection) {
        disconnect(m_dragConnection);
    }
    if (m_directionConnection) {
        disconnect(m_directionConnection);
    }

    m_selectContext = context;
    if (m_selectContext) {
        m_visualMoveConnection = connect(
            m_selectContext,
            SIGNAL(visualMoveActiveChanged()),
            this,
            SLOT(clearSelectTimerFireCache()));
        m_dragConnection = connect(
            m_selectContext,
            SIGNAL(scrollFixedPointDraggingChanged()),
            this,
            SLOT(clearSelectTimerFireCache()));
        m_directionConnection = connect(
            m_selectContext,
            SIGNAL(scrollDirectionChanged()),
            this,
            SLOT(clearSelectTimerFireCache()));
    } else {
        m_visualMoveConnection = {};
        m_dragConnection = {};
        m_directionConnection = {};
    }

    emit selectContextChanged();
    clearSelectTimerFireCache();
    bumpRevision();
}

QString Lr2SkinTimerState::screenKey() const { return m_screenKey; }
void Lr2SkinTimerState::setScreenKey(const QString& value) {
    if (m_screenKey == value) {
        return;
    }
    m_screenKey = value;
    emit screenKeyChanged();
    clearSelectTimerFireCache();
    bumpRevision();
}

bool Lr2SkinTimerState::isGameplayScreen() const { return m_gameplayScreen; }
void Lr2SkinTimerState::setGameplayScreen(bool value) {
    if (m_gameplayScreen == value) {
        return;
    }
    m_gameplayScreen = value;
    emit gameplayScreenChanged();
    bumpRevision();
}

bool Lr2SkinTimerState::isResultScreen() const { return m_resultScreen; }
void Lr2SkinTimerState::setResultScreen(bool value) {
    if (m_resultScreen == value) {
        return;
    }
    m_resultScreen = value;
    emit resultScreenChanged();
    bumpRevision();
}

bool Lr2SkinTimerState::acceptsInput() const { return m_acceptsInput; }
void Lr2SkinTimerState::setAcceptsInput(bool value) {
    if (m_acceptsInput == value) {
        return;
    }
    m_acceptsInput = value;
    emit acceptsInputChanged();
    clearSelectTimerFireCache();
    bumpRevision();
}

int Lr2SkinTimerState::startInput() const { return m_startInput; }
void Lr2SkinTimerState::setStartInput(int value) {
    if (setInt(m_startInput, value)) {
        emit startInputChanged();
        bumpRevision();
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
            bumpRevision();
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
        bumpSelectInfoRevision();
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
        bumpRevision();
    }
}

int Lr2SkinTimerState::resultTimer152SkinTime() const { return m_resultTimer152SkinTime; }
void Lr2SkinTimerState::setResultTimer152SkinTime(int value) {
    if (setInt(m_resultTimer152SkinTime, value)) {
        emit resultTimer152SkinTimeChanged();
        bumpRevision();
    }
}

int Lr2SkinTimerState::resultGraphStartSkinTime() const { return m_resultGraphStartSkinTime; }
void Lr2SkinTimerState::setResultGraphStartSkinTime(int value) {
    if (setInt(m_resultGraphStartSkinTime, value)) {
        emit resultGraphStartSkinTimeChanged();
        bumpRevision();
    }
}

int Lr2SkinTimerState::resultGraphEndSkinTime() const { return m_resultGraphEndSkinTime; }
void Lr2SkinTimerState::setResultGraphEndSkinTime(int value) {
    if (setInt(m_resultGraphEndSkinTime, value)) {
        emit resultGraphEndSkinTimeChanged();
        bumpRevision();
    }
}

int Lr2SkinTimerState::gameplayTimerRevision() const { return m_gameplayTimerRevision; }
void Lr2SkinTimerState::setGameplayTimerRevision(int value) {
    if (setInt(m_gameplayTimerRevision, value)) {
        emit gameplayTimerRevisionChanged();
        bumpRevision();
    }
}

QVariant Lr2SkinTimerState::gameplayTimerValues() const {
    return variantMapFromGameplayTimerHash(m_gameplayTimerValues);
}
void Lr2SkinTimerState::setGameplayTimerValues(const QVariant& values) {
    const QHash<int, int> next = gameplayTimerHashFromVariant(values);
    if (m_gameplayTimerValues == next) {
        return;
    }
    m_gameplayTimerValues = next;
    m_gameplayTimerValuesDirty = false;
    emit gameplayTimerValuesChanged();
    bumpRevision();
}

int Lr2SkinTimerState::revision() const { return m_revision; }
int Lr2SkinTimerState::selectInfoRevision() const { return m_selectInfoRevision; }

int Lr2SkinTimerState::gameplayTimerFireTime(const QVariant& timer) const {
    const int idx = timerValue(timer);
    if (!m_host) {
        return -1;
    }
    if (idx == 140) {
        return invokeHostInt("gameplayRhythmTimerSkinTime");
    }
    Q_UNUSED(m_gameplayTimerRevision);
    return gameplayTimerValue(idx);
}

int Lr2SkinTimerState::resultTimerFireTime(const QVariant& timer) const {
    const int idx = timerValue(timer);
    if (!m_host) {
        return -1;
    }
    if (idx == 0) {
        return 0;
    }
    if (idx == 1) {
        return m_acceptsInput ? std::min(m_renderSkinTime, m_startInput) : -1;
    }
    if (idx == 150) {
        return m_renderSkinTime >= m_resultGraphStartSkinTime ? m_resultGraphStartSkinTime : -1;
    }
    if (idx == 151) {
        if (m_resultTimer151SkinTime >= 0) {
            return m_resultTimer151SkinTime;
        }
        return m_renderSkinTime >= m_resultGraphEndSkinTime ? m_resultGraphEndSkinTime : -1;
    }
    if (idx == 152) {
        return m_resultTimer152SkinTime >= 0 ? m_resultTimer152SkinTime : -1;
    }
    return -1;
}

int Lr2SkinTimerState::selectTimerFireTime(const QVariant& timer, bool liveClock) const {
    const int idx = timerValue(timer);
    if (!m_host) {
        return -1;
    }

    const int baseTime = selectTimerBaseTime(liveClock);
    if (idx == 0) {
        return 0;
    }
    if (idx == 1) {
        return m_acceptsInput ? std::min(m_renderSkinTime, m_startInput) : -1;
    }
    if (idx == 171) {
        return m_selectDatabaseLoadedSkinTime;
    }
    if (idx == 11) {
        return baseTime - m_selectInfoElapsed;
    }
    if (idx == 14 && m_acceptsInput) {
        return m_selectNoScrollStartSkinTime;
    }

    if (selectContextBool("visualMoveActive") || selectContextBool("scrollFixedPointDragging")) {
        if (idx == 10) {
            return m_selectScrollStartSkinTime;
        }
        const int scrollDirection = selectContextInt("scrollDirection");
        if (idx == 12 && scrollDirection == selectContextInt("lr2ScrollUp")) {
            return m_selectScrollStartSkinTime;
        }
        if (idx == 13 && scrollDirection == selectContextInt("lr2ScrollDown")) {
            return m_selectScrollStartSkinTime;
        }
    }

    if (idx >= 21 && idx <= 26) {
        const int panel = idx - 20;
        return m_selectPanel == panel
            ? baseTime - selectElapsedSince(m_selectPanelStartSkinTime)
            : -1;
    }
    if (idx >= 31 && idx <= 36) {
        const int panel = idx - 30;
        const int elapsed = selectElapsedSince(m_selectPanelCloseStartSkinTime);
        return m_selectPanelClosing == panel && elapsed < m_selectPanelHoldTime
            ? baseTime - elapsed
            : -1;
    }
    if (idx == 15 && m_lr2ReadmeMode == 1) {
        return baseTime - m_lr2ReadmeElapsed;
    }
    if (idx == 16 && m_lr2ReadmeMode == 2) {
        return baseTime - m_lr2ReadmeElapsed;
    }
    if (m_lr2RankingTransitionPhase != 0 && idx == m_lr2RankingTransitionPhase) {
        return baseTime - m_lr2RankingTransitionElapsed;
    }
    return invokeHostInt("selectHeldButtonTimerFireTime", idx, liveClock);
}

bool Lr2SkinTimerState::selectTimerCanFire(const QVariant& timer) const {
    const int idx = timerValue(timer);
    if (idx == 0 || idx == 1 || idx == 10 || idx == 11
            || idx == 12 || idx == 13 || idx == 14
            || idx == 15 || idx == 16 || idx == 171
            || idx == 175 || idx == 176) {
        return true;
    }
    if (idx >= 21 && idx <= 26) {
        return true;
    }
    if (idx >= 31 && idx <= 36) {
        return true;
    }
    return m_host && invokeHostBool("isSelectHeldButtonTimer", idx);
}

bool Lr2SkinTimerState::skinTimerCanFire(const QVariant& timer) const {
    if (!m_host) {
        return false;
    }
    const int idx = timerValue(timer);
    if (idx == 0) {
        return true;
    }
    if (m_gameplayScreen) {
        return true;
    }
    if (m_resultScreen) {
        return idx == 1 || idx == 150 || idx == 151 || idx == 152;
    }
    if (m_screenKey == QStringLiteral("select")) {
        return selectTimerCanFire(idx);
    }
    return false;
}

int Lr2SkinTimerState::skinTimerFireTime(const QVariant& timer, bool liveClock) {
    if (!m_host) {
        return -1;
    }
    const int idx = timerValue(timer);
    if (idx == 0) {
        return 0;
    }
    if (m_gameplayScreen) {
        return gameplayTimerFireTime(idx);
    }
    if (m_resultScreen) {
        return resultTimerFireTime(idx);
    }
    if (m_screenKey == QStringLiteral("select")) {
        return canCacheSelectTimerFire(idx)
            ? cachedSelectTimerFireTime(idx, liveClock)
            : selectTimerFireTime(idx, liveClock);
    }
    return -1;
}

bool Lr2SkinTimerState::isSelectHeldButtonTimer(const QVariant& timer) const {
    return m_host && invokeHostBool("isSelectHeldButtonTimer", timerValue(timer));
}

bool Lr2SkinTimerState::setGameplayTimerValue(const QVariant& timer, int skinTime) {
    const int idx = timerValue(timer);
    if (idx == 0 || skinTime < 0) {
        return false;
    }
    const auto it = m_gameplayTimerValues.constFind(idx);
    if (it != m_gameplayTimerValues.constEnd() && it.value() == skinTime) {
        return false;
    }
    m_gameplayTimerValues.insert(idx, skinTime);
    m_gameplayTimerValuesDirty = true;
    m_pendingGameplayTimerChanges.insert(idx);
    emit gameplayTimerValuesChanged();
    return true;
}

bool Lr2SkinTimerState::clearGameplayTimerValue(const QVariant& timer) {
    const int idx = timerValue(timer);
    if (idx == 0 || !m_gameplayTimerValues.contains(idx)) {
        return false;
    }
    m_gameplayTimerValues.remove(idx);
    m_gameplayTimerValuesDirty = true;
    m_pendingGameplayTimerChanges.insert(idx);
    emit gameplayTimerValuesChanged();
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
    emit gameplayTimerValuesChanged();
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
    ++m_gameplayTimerRevision;
    emit gameplayTimerRevisionChanged();
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
    bumpRevision();
}

int Lr2SkinTimerState::selectTimerBaseTime(bool liveClock) const {
    return liveClock ? m_selectSourceSkinTime : m_renderSkinTime;
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

int Lr2SkinTimerState::timerValue(const QVariant& timer) const {
    return toInt(timer, 0);
}

int Lr2SkinTimerState::selectContextInt(const char* name, int fallback) const {
    return toInt(objectProperty(m_selectContext, name), fallback);
}

bool Lr2SkinTimerState::selectContextBool(const char* name) const {
    return toBool(objectProperty(m_selectContext, name));
}

bool Lr2SkinTimerState::invokeHostBool(const char* method, const QVariant& arg) const {
    if (!m_host) {
        return false;
    }
    QVariant result;
    const bool ok = QMetaObject::invokeMethod(
        m_host,
        method,
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QVariant, arg));
    return ok && toBool(result);
}

int Lr2SkinTimerState::invokeHostInt(const char* method) const {
    if (!m_host) {
        return -1;
    }
    QVariant result;
    const bool ok = QMetaObject::invokeMethod(
        m_host,
        method,
        Q_RETURN_ARG(QVariant, result));
    return ok ? toInt(result, -1) : -1;
}

int Lr2SkinTimerState::invokeHostInt(const char* method, const QVariant& arg1, const QVariant& arg2) const {
    if (!m_host) {
        return -1;
    }
    QVariant result;
    const bool ok = QMetaObject::invokeMethod(
        m_host,
        method,
        Q_RETURN_ARG(QVariant, result),
        Q_ARG(QVariant, arg1),
        Q_ARG(QVariant, arg2));
    return ok ? toInt(result, -1) : -1;
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

void Lr2SkinTimerState::bumpRevision() {
    ++m_revision;
    emit revisionChanged();
}

void Lr2SkinTimerState::bumpSelectInfoRevision() {
    ++m_selectInfoRevision;
    emit selectInfoRevisionChanged();
}

bool Lr2SkinTimerState::setInt(int& field, int value) {
    if (field == value) {
        return false;
    }
    field = value;
    return true;
}
