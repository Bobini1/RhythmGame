#pragma once

#include "Lr2SkinClock.h"

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QHash>
#include <QSet>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <array>

class Lr2SkinTimerState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(Lr2SkinClock* clock READ clock WRITE setClock NOTIFY clockChanged)
    Q_PROPERTY(QObject* selectContext READ selectContext WRITE setSelectContext NOTIFY selectContextChanged)
    Q_PROPERTY(QString screenKey READ screenKey WRITE setScreenKey NOTIFY screenKeyChanged)
    Q_PROPERTY(bool gameplayScreen READ isGameplayScreen WRITE setGameplayScreen NOTIFY gameplayScreenChanged)
    Q_PROPERTY(bool resultScreen READ isResultScreen WRITE setResultScreen NOTIFY resultScreenChanged)
    Q_PROPERTY(bool acceptsInput READ acceptsInput WRITE setAcceptsInput NOTIFY acceptsInputChanged)
    Q_PROPERTY(int startInput READ startInput WRITE setStartInput NOTIFY startInputChanged)
    Q_PROPERTY(int globalSkinTime READ globalSkinTime WRITE setGlobalSkinTime NOTIFY globalSkinTimeChanged)
    Q_PROPERTY(int renderSkinTime READ renderSkinTime WRITE setRenderSkinTime NOTIFY renderSkinTimeChanged)
    Q_PROPERTY(int selectSourceSkinTime READ selectSourceSkinTime WRITE setSelectSourceSkinTime NOTIFY selectSourceSkinTimeChanged)
    Q_PROPERTY(int selectLiveSkinTime READ selectLiveSkinTime WRITE setSelectLiveSkinTime NOTIFY selectLiveSkinTimeChanged)
    Q_PROPERTY(int selectInfoElapsed READ selectInfoElapsed WRITE setSelectInfoElapsed NOTIFY selectInfoElapsedChanged)
    Q_PROPERTY(int selectScrollStartSkinTime READ selectScrollStartSkinTime WRITE setSelectScrollStartSkinTime NOTIFY selectScrollStartSkinTimeChanged)
    Q_PROPERTY(int selectNoScrollStartSkinTime READ selectNoScrollStartSkinTime WRITE setSelectNoScrollStartSkinTime NOTIFY selectNoScrollStartSkinTimeChanged)
    Q_PROPERTY(int selectDatabaseLoadedSkinTime READ selectDatabaseLoadedSkinTime WRITE setSelectDatabaseLoadedSkinTime NOTIFY selectDatabaseLoadedSkinTimeChanged)
    Q_PROPERTY(int selectPanel READ selectPanel WRITE setSelectPanel NOTIFY selectPanelChanged)
    Q_PROPERTY(int selectPanelStartSkinTime READ selectPanelStartSkinTime WRITE setSelectPanelStartSkinTime NOTIFY selectPanelStartSkinTimeChanged)
    Q_PROPERTY(int selectPanelClosing READ selectPanelClosing WRITE setSelectPanelClosing NOTIFY selectPanelClosingChanged)
    Q_PROPERTY(int selectPanelCloseStartSkinTime READ selectPanelCloseStartSkinTime WRITE setSelectPanelCloseStartSkinTime NOTIFY selectPanelCloseStartSkinTimeChanged)
    Q_PROPERTY(int selectPanelHoldTime READ selectPanelHoldTime WRITE setSelectPanelHoldTime NOTIFY selectPanelHoldTimeChanged)
    Q_PROPERTY(int lr2ReadmeMode READ lr2ReadmeMode WRITE setLr2ReadmeMode NOTIFY lr2ReadmeModeChanged)
    Q_PROPERTY(int lr2ReadmeElapsed READ lr2ReadmeElapsed WRITE setLr2ReadmeElapsed NOTIFY lr2ReadmeElapsedChanged)
    Q_PROPERTY(int lr2RankingTransitionPhase READ lr2RankingTransitionPhase WRITE setLr2RankingTransitionPhase NOTIFY lr2RankingTransitionPhaseChanged)
    Q_PROPERTY(int lr2RankingTransitionElapsed READ lr2RankingTransitionElapsed WRITE setLr2RankingTransitionElapsed NOTIFY lr2RankingTransitionElapsedChanged)
    Q_PROPERTY(int resultTimer151SkinTime READ resultTimer151SkinTime WRITE setResultTimer151SkinTime NOTIFY resultTimer151SkinTimeChanged)
    Q_PROPERTY(int resultTimer152SkinTime READ resultTimer152SkinTime WRITE setResultTimer152SkinTime NOTIFY resultTimer152SkinTimeChanged)
    Q_PROPERTY(int resultGraphStartSkinTime READ resultGraphStartSkinTime WRITE setResultGraphStartSkinTime NOTIFY resultGraphStartSkinTimeChanged)
    Q_PROPERTY(int resultGraphEndSkinTime READ resultGraphEndSkinTime WRITE setResultGraphEndSkinTime NOTIFY resultGraphEndSkinTimeChanged)
    Q_PROPERTY(int gameplayTimerRevision READ gameplayTimerRevision WRITE setGameplayTimerRevision NOTIFY gameplayTimerRevisionChanged)
    Q_PROPERTY(QVariant gameplayTimerValues READ gameplayTimerValues WRITE setGameplayTimerValues NOTIFY gameplayTimerValuesChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(int selectInfoRevision READ selectInfoRevision NOTIFY selectInfoRevisionChanged)

public:
    explicit Lr2SkinTimerState(QObject* parent = nullptr);

    QObject* host() const;
    void setHost(QObject* host);

    Lr2SkinClock* clock() const;
    void setClock(Lr2SkinClock* clock);

    QObject* selectContext() const;
    void setSelectContext(QObject* context);

    QString screenKey() const;
    void setScreenKey(const QString& value);

    bool isGameplayScreen() const;
    void setGameplayScreen(bool value);

    bool isResultScreen() const;
    void setResultScreen(bool value);

    bool acceptsInput() const;
    void setAcceptsInput(bool value);

    int startInput() const;
    void setStartInput(int value);

    int globalSkinTime() const;
    void setGlobalSkinTime(int value);

    int renderSkinTime() const;
    void setRenderSkinTime(int value);

    int selectSourceSkinTime() const;
    void setSelectSourceSkinTime(int value);

    int selectLiveSkinTime() const;
    void setSelectLiveSkinTime(int value);

    int selectInfoElapsed() const;
    void setSelectInfoElapsed(int value);

    int selectScrollStartSkinTime() const;
    void setSelectScrollStartSkinTime(int value);

    int selectNoScrollStartSkinTime() const;
    void setSelectNoScrollStartSkinTime(int value);

    int selectDatabaseLoadedSkinTime() const;
    void setSelectDatabaseLoadedSkinTime(int value);

    int selectPanel() const;
    void setSelectPanel(int value);

    int selectPanelStartSkinTime() const;
    void setSelectPanelStartSkinTime(int value);

    int selectPanelClosing() const;
    void setSelectPanelClosing(int value);

    int selectPanelCloseStartSkinTime() const;
    void setSelectPanelCloseStartSkinTime(int value);

    int selectPanelHoldTime() const;
    void setSelectPanelHoldTime(int value);

    int lr2ReadmeMode() const;
    void setLr2ReadmeMode(int value);

    int lr2ReadmeElapsed() const;
    void setLr2ReadmeElapsed(int value);

    int lr2RankingTransitionPhase() const;
    void setLr2RankingTransitionPhase(int value);

    int lr2RankingTransitionElapsed() const;
    void setLr2RankingTransitionElapsed(int value);

    int resultTimer151SkinTime() const;
    void setResultTimer151SkinTime(int value);

    int resultTimer152SkinTime() const;
    void setResultTimer152SkinTime(int value);

    int resultGraphStartSkinTime() const;
    void setResultGraphStartSkinTime(int value);

    int resultGraphEndSkinTime() const;
    void setResultGraphEndSkinTime(int value);

    int gameplayTimerRevision() const;
    void setGameplayTimerRevision(int value);

    QVariant gameplayTimerValues() const;
    void setGameplayTimerValues(const QVariant& values);

    int revision() const;
    int selectInfoRevision() const;

    Q_INVOKABLE int gameplayTimerFireTime(const QVariant& timer) const;
    Q_INVOKABLE int resultTimerFireTime(const QVariant& timer) const;
    Q_INVOKABLE int selectTimerFireTime(const QVariant& timer, bool liveClock = false) const;
    Q_INVOKABLE bool selectTimerCanFire(const QVariant& timer) const;
    Q_INVOKABLE bool skinTimerCanFire(const QVariant& timer) const;
    Q_INVOKABLE int skinTimerFireTime(const QVariant& timer, bool liveClock = false);
    Q_INVOKABLE bool isSelectHeldButtonTimer(const QVariant& timer) const;
    Q_INVOKABLE bool setGameplayTimerValue(const QVariant& timer, int skinTime);
    Q_INVOKABLE bool clearGameplayTimerValue(const QVariant& timer);
    Q_INVOKABLE bool resetGameplayTimerValues();
    Q_INVOKABLE bool commitGameplayTimerChanges();
    QSet<int> takeCommittedGameplayTimerChanges(bool* fullRefresh = nullptr);

public slots:
    void clearSelectTimerFireCache();
    void syncSelectInfoElapsedFromClock();

signals:
    void hostChanged();
    void clockChanged();
    void selectContextChanged();
    void screenKeyChanged();
    void gameplayScreenChanged();
    void resultScreenChanged();
    void acceptsInputChanged();
    void startInputChanged();
    void globalSkinTimeChanged();
    void renderSkinTimeChanged();
    void selectSourceSkinTimeChanged();
    void selectLiveSkinTimeChanged();
    void selectInfoElapsedChanged();
    void selectScrollStartSkinTimeChanged();
    void selectNoScrollStartSkinTimeChanged();
    void selectDatabaseLoadedSkinTimeChanged();
    void selectPanelChanged();
    void selectPanelStartSkinTimeChanged();
    void selectPanelClosingChanged();
    void selectPanelCloseStartSkinTimeChanged();
    void selectPanelHoldTimeChanged();
    void lr2ReadmeModeChanged();
    void lr2ReadmeElapsedChanged();
    void lr2RankingTransitionPhaseChanged();
    void lr2RankingTransitionElapsedChanged();
    void resultTimer151SkinTimeChanged();
    void resultTimer152SkinTimeChanged();
    void resultGraphStartSkinTimeChanged();
    void resultGraphEndSkinTimeChanged();
    void gameplayTimerRevisionChanged();
    void gameplayTimerValuesChanged();
    void gameplayTimerValuesCommitted();
    void revisionChanged();
    void selectInfoRevisionChanged();

private:
    int selectTimerBaseTime(bool liveClock) const;
    int selectElapsedSince(int startSkinTime) const;
    bool canCacheSelectTimerFire(int timer) const;
    int cachedSelectTimerFireTime(int timer, bool liveClock);
    int timerValue(const QVariant& timer) const;
    int selectContextInt(const char* name, int fallback = 0) const;
    bool selectContextBool(const char* name) const;
    bool invokeHostBool(const char* method, const QVariant& arg) const;
    int invokeHostInt(const char* method) const;
    int invokeHostInt(const char* method, const QVariant& arg1, const QVariant& arg2) const;
    int gameplayTimerValue(int timer) const;
    QHash<int, int> initialGameplayTimerValues() const;
    int cacheIndexForTimer(int timer, bool liveClock) const;
    void resetFrameCache() const;
    void bumpRevision();
    void bumpSelectInfoRevision();
    bool setInt(int& field, int value);

    QPointer<QObject> m_host;
    QPointer<Lr2SkinClock> m_clock;
    QPointer<QObject> m_selectContext;
    QMetaObject::Connection m_clockSelectInfoConnection;
    QMetaObject::Connection m_visualMoveConnection;
    QMetaObject::Connection m_dragConnection;
    QMetaObject::Connection m_directionConnection;
    QString m_screenKey;
    bool m_gameplayScreen = false;
    bool m_resultScreen = false;
    bool m_acceptsInput = false;
    int m_startInput = 0;
    int m_globalSkinTime = 0;
    int m_renderSkinTime = 0;
    int m_selectSourceSkinTime = 0;
    int m_selectLiveSkinTime = 0;
    int m_selectInfoElapsed = 0;
    int m_selectScrollStartSkinTime = 0;
    int m_selectNoScrollStartSkinTime = 0;
    int m_selectDatabaseLoadedSkinTime = 0;
    int m_selectPanel = 0;
    int m_selectPanelStartSkinTime = 0;
    int m_selectPanelClosing = 0;
    int m_selectPanelCloseStartSkinTime = 0;
    int m_selectPanelHoldTime = 0;
    int m_lr2ReadmeMode = 0;
    int m_lr2ReadmeElapsed = 0;
    int m_lr2RankingTransitionPhase = 0;
    int m_lr2RankingTransitionElapsed = 0;
    int m_resultTimer151SkinTime = -1;
    int m_resultTimer152SkinTime = -1;
    int m_resultGraphStartSkinTime = 0;
    int m_resultGraphEndSkinTime = 0;
    int m_gameplayTimerRevision = 0;
    QHash<int, int> m_gameplayTimerValues;
    bool m_gameplayTimerValuesDirty = false;
    bool m_pendingGameplayTimerFullRefresh = false;
    bool m_committedGameplayTimerFullRefresh = false;
    QSet<int> m_pendingGameplayTimerChanges;
    QSet<int> m_committedGameplayTimerChanges;
    int m_revision = 0;
    int m_selectInfoRevision = 0;
    int m_cacheEpoch = 0;
    mutable int m_cacheFrame = -1;
    mutable int m_cacheEpochSnapshot = -1;
    mutable std::array<int, 10> m_cacheValues;
    mutable std::array<bool, 10> m_cacheValid;
};
