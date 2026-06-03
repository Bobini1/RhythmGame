#pragma once

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include "Lr2SkinRuntime.h"

class Lr2SelectUpdateController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* skinModel READ skinModel WRITE setSkinModel NOTIFY skinModelChanged)
    Q_PROPERTY(Lr2SkinRuntime* skinRuntime READ skinRuntime WRITE setSkinRuntime NOTIFY skinRuntimeChanged)
    Q_PROPERTY(bool screenUpdatesActive READ screenUpdatesActive WRITE setScreenUpdatesActive NOTIFY screenUpdatesActiveChanged)
    Q_PROPERTY(QString effectiveScreenKey READ effectiveScreenKey WRITE setEffectiveScreenKey NOTIFY effectiveScreenKeyChanged)
    Q_PROPERTY(bool componentReady READ componentReady WRITE setComponentReady NOTIFY componentReadyChanged)
    Q_PROPERTY(bool rankingMode READ rankingMode WRITE setRankingMode NOTIFY rankingModeChanged)
    Q_PROPERTY(int scoreRevision READ scoreRevision WRITE setScoreRevision NOTIFY scoreRevisionChanged)
    Q_PROPERTY(int focusRevision READ focusRevision WRITE setFocusRevision NOTIFY focusRevisionChanged)
    Q_PROPERTY(int selectPanel READ selectPanel WRITE setSelectPanel NOTIFY selectPanelChanged)
    Q_PROPERTY(QString lr2RankingMd5 READ lr2RankingMd5 WRITE setLr2RankingMd5 NOTIFY lr2RankingMd5Changed)
    Q_PROPERTY(QString lr2RankingRequestMd5 READ lr2RankingRequestMd5 WRITE setLr2RankingRequestMd5 NOTIFY lr2RankingRequestMd5Changed)
    Q_PROPERTY(QVariant parseActiveOptions READ parseActiveOptions WRITE setParseActiveOptions NOTIFY parseActiveOptionsChanged)
    Q_PROPERTY(QVariant runtimeActiveOptions READ runtimeActiveOptions WRITE setRuntimeActiveOptions NOTIFY runtimeActiveOptionsChanged)
    Q_PROPERTY(QVariant barActiveOptions READ barActiveOptions WRITE setBarActiveOptions NOTIFY barActiveOptionsChanged)
    Q_PROPERTY(QVariant baseActiveOptions READ baseActiveOptions WRITE setBaseActiveOptions NOTIFY baseActiveOptionsChanged)
    Q_PROPERTY(QString baseActiveOptionsKey READ baseActiveOptionsKey WRITE setBaseActiveOptionsKey NOTIFY baseActiveOptionsKeyChanged)
    Q_PROPERTY(QVariant selectCommonActiveOptions READ selectCommonActiveOptions WRITE setSelectCommonActiveOptions NOTIFY selectCommonActiveOptionsChanged)
    Q_PROPERTY(QString selectCommonActiveOptionsKey READ selectCommonActiveOptionsKey WRITE setSelectCommonActiveOptionsKey NOTIFY selectCommonActiveOptionsKeyChanged)
    Q_PROPERTY(bool selectCommonActiveOptionsReady READ selectCommonActiveOptionsReady WRITE setSelectCommonActiveOptionsReady NOTIFY selectCommonActiveOptionsReadyChanged)
    Q_PROPERTY(QVariant selectRuntimeActiveOptions READ selectRuntimeActiveOptions WRITE setSelectRuntimeActiveOptions NOTIFY selectRuntimeActiveOptionsChanged)
    Q_PROPERTY(QString selectRuntimeActiveOptionsKey READ selectRuntimeActiveOptionsKey WRITE setSelectRuntimeActiveOptionsKey NOTIFY selectRuntimeActiveOptionsKeyChanged)
    Q_PROPERTY(QVariant gameplayRuntimeActiveOptions READ gameplayRuntimeActiveOptions WRITE setGameplayRuntimeActiveOptions NOTIFY gameplayRuntimeActiveOptionsChanged)
    Q_PROPERTY(QString gameplayRuntimeActiveOptionsKey READ gameplayRuntimeActiveOptionsKey WRITE setGameplayRuntimeActiveOptionsKey NOTIFY gameplayRuntimeActiveOptionsKeyChanged)
    Q_PROPERTY(QVariant barRuntimeActiveOptions READ barRuntimeActiveOptions WRITE setBarRuntimeActiveOptions NOTIFY barRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant baseRuntimeActiveOptions READ baseRuntimeActiveOptions WRITE setBaseRuntimeActiveOptions NOTIFY baseRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant selectCommonRuntimeActiveOptions READ selectCommonRuntimeActiveOptions WRITE setSelectCommonRuntimeActiveOptions NOTIFY selectCommonRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant selectRequiredRuntimeActiveOptions READ selectRequiredRuntimeActiveOptions WRITE setSelectRequiredRuntimeActiveOptions NOTIFY selectRequiredRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant selectRuntimeGeneratedActiveOptions READ selectRuntimeGeneratedActiveOptions WRITE setSelectRuntimeGeneratedActiveOptions NOTIFY selectRuntimeGeneratedActiveOptionsChanged)
    Q_PROPERTY(QVariant screenRuntimeActiveOptions READ screenRuntimeActiveOptions WRITE setScreenRuntimeActiveOptions NOTIFY screenRuntimeActiveOptionsChanged)
    Q_PROPERTY(bool gameplayScreen READ gameplayScreen WRITE setGameplayScreen NOTIFY gameplayScreenChanged)
    Q_PROPERTY(int selectRevision READ selectRevision NOTIFY selectRevisionChanged)
    Q_PROPERTY(int selectDetailRevision READ selectDetailRevision NOTIFY selectDetailRevisionChanged)

public:
    explicit Lr2SelectUpdateController(QObject* parent = nullptr);

    QObject* skinModel() const;
    void setSkinModel(QObject* model);

    Lr2SkinRuntime* skinRuntime() const;
    void setSkinRuntime(Lr2SkinRuntime* runtime);

    bool screenUpdatesActive() const;
    void setScreenUpdatesActive(bool active);

    QString effectiveScreenKey() const;
    void setEffectiveScreenKey(const QString& key);

    bool componentReady() const;
    void setComponentReady(bool ready);

    bool rankingMode() const;
    void setRankingMode(bool active);

    int scoreRevision() const;
    void setScoreRevision(int revision);

    int focusRevision() const;
    void setFocusRevision(int revision);

    int selectPanel() const;
    void setSelectPanel(int panel);

    QString lr2RankingMd5() const;
    void setLr2RankingMd5(const QString& md5);

    QString lr2RankingRequestMd5() const;
    void setLr2RankingRequestMd5(const QString& md5);

    QVariant parseActiveOptions() const;
    void setParseActiveOptions(const QVariant& options);

    QVariant runtimeActiveOptions() const;
    void setRuntimeActiveOptions(const QVariant& options);

    QVariant barActiveOptions() const;
    void setBarActiveOptions(const QVariant& options);

    QVariant baseActiveOptions() const;
    void setBaseActiveOptions(const QVariant& options);

    QString baseActiveOptionsKey() const;
    void setBaseActiveOptionsKey(const QString& key);

    QVariant selectCommonActiveOptions() const;
    void setSelectCommonActiveOptions(const QVariant& options);

    QString selectCommonActiveOptionsKey() const;
    void setSelectCommonActiveOptionsKey(const QString& key);

    bool selectCommonActiveOptionsReady() const;
    void setSelectCommonActiveOptionsReady(bool ready);

    QVariant selectRuntimeActiveOptions() const;
    void setSelectRuntimeActiveOptions(const QVariant& options);

    QString selectRuntimeActiveOptionsKey() const;
    void setSelectRuntimeActiveOptionsKey(const QString& key);

    QVariant gameplayRuntimeActiveOptions() const;
    void setGameplayRuntimeActiveOptions(const QVariant& options);

    QString gameplayRuntimeActiveOptionsKey() const;
    void setGameplayRuntimeActiveOptionsKey(const QString& key);

    QVariant barRuntimeActiveOptions() const;
    void setBarRuntimeActiveOptions(const QVariant& options);

    QVariant baseRuntimeActiveOptions() const;
    void setBaseRuntimeActiveOptions(const QVariant& options);

    QVariant selectCommonRuntimeActiveOptions() const;
    void setSelectCommonRuntimeActiveOptions(const QVariant& options);

    QVariant selectRequiredRuntimeActiveOptions() const;
    void setSelectRequiredRuntimeActiveOptions(const QVariant& options);

    QVariant selectRuntimeGeneratedActiveOptions() const;
    void setSelectRuntimeGeneratedActiveOptions(const QVariant& options);

    QVariant screenRuntimeActiveOptions() const;
    void setScreenRuntimeActiveOptions(const QVariant& options);

    bool gameplayScreen() const;
    void setGameplayScreen(bool active);

    int selectRevision() const;
    int selectDetailRevision() const;

    Q_INVOKABLE bool refreshBaseActiveOptions();
    Q_INVOKABLE bool refreshSelectRuntimeActiveOptions();
    Q_INVOKABLE bool refreshGameplayRuntimeActiveOptions();
    Q_INVOKABLE void replaceSelectRuntimeGeneratedActiveOptions(const QVariant& options);

signals:
    void skinModelChanged();
    void skinRuntimeChanged();
    void screenUpdatesActiveChanged();
    void effectiveScreenKeyChanged();
    void componentReadyChanged();
    void rankingModeChanged();
    void scoreRevisionChanged();
    void focusRevisionChanged();
    void selectPanelChanged();
    void lr2RankingMd5Changed();
    void lr2RankingRequestMd5Changed();
    void parseActiveOptionsChanged();
    void runtimeActiveOptionsChanged();
    void barActiveOptionsChanged();
    void baseActiveOptionsChanged();
    void baseActiveOptionsKeyChanged();
    void selectCommonActiveOptionsChanged();
    void selectCommonActiveOptionsKeyChanged();
    void selectCommonActiveOptionsReadyChanged();
    void selectRuntimeActiveOptionsChanged();
    void selectRuntimeActiveOptionsKeyChanged();
    void gameplayRuntimeActiveOptionsChanged();
    void gameplayRuntimeActiveOptionsKeyChanged();
    void barRuntimeActiveOptionsChanged();
    void baseRuntimeActiveOptionsChanged();
    void selectCommonRuntimeActiveOptionsChanged();
    void selectRequiredRuntimeActiveOptionsChanged();
    void selectRuntimeGeneratedActiveOptionsChanged();
    void screenRuntimeActiveOptionsChanged();
    void gameplayScreenChanged();
    void selectRevisionChanged();
    void selectDetailRevisionChanged();
    void rankingStatsApplyRequested();
    void selectSideEffectsUpdateRequested();

private:
    void applyRuntimeActiveOptions(const QVariant& value);
    void refreshCurrentRuntimeActiveOptions();
    bool sameNumberArray(const QVariant& lhs, const QVariant& rhs) const;
    QString numberArrayKey(const QVariant& values) const;
    QVariant normalizedNumberArray(const QVariant& values) const;
    QVariantList mergedNumberArray(const QVariant& first, const QVariant& second) const;
    QVariantList selectStaticOptions(bool includeRankingOption, bool includePanelOption) const;
    QObject* skinModelObject() const;
    bool skinUsesOption(int option) const;
    void appendUniqueOption(QVariantList& options, int option) const;
    void appendUniqueSkinOption(QVariantList& options, int option) const;
    void appendDefaultOptionIfMissing(QVariantList& options, const QList<int>& choices, int fallback) const;
    QList<int> numberList(const QVariant& values) const;
    void handleCommittedSelectState();
    void handleSelectRevisionChanged();
    bool updateSelectRevision(bool runSideEffects);
    void setSelectDetailRevision(int revision);

    QPointer<QObject> m_skinModel;
    QPointer<Lr2SkinRuntime> m_skinRuntime;
    bool m_screenUpdatesActive = false;
    QString m_effectiveScreenKey;
    bool m_componentReady = false;
    bool m_rankingMode = false;
    int m_scoreRevision = 0;
    int m_focusRevision = 0;
    int m_selectPanel = 0;
    QString m_lr2RankingMd5;
    QString m_lr2RankingRequestMd5;
    QVariant m_parseActiveOptions;
    QVariant m_runtimeActiveOptions;
    QVariant m_barActiveOptions;
    QVariant m_baseActiveOptions;
    QString m_baseActiveOptionsKey;
    QVariant m_selectCommonActiveOptions;
    QString m_selectCommonActiveOptionsKey;
    bool m_selectCommonActiveOptionsReady = false;
    QVariant m_selectRuntimeActiveOptions;
    QString m_selectRuntimeActiveOptionsKey;
    QVariant m_gameplayRuntimeActiveOptions;
    QString m_gameplayRuntimeActiveOptionsKey;
    QVariant m_barRuntimeActiveOptions;
    QVariant m_baseRuntimeActiveOptions;
    QVariant m_selectCommonRuntimeActiveOptions;
    QVariant m_selectRequiredRuntimeActiveOptions;
    QVariant m_selectRuntimeGeneratedActiveOptions;
    QVariant m_screenRuntimeActiveOptions;
    bool m_gameplayScreen = false;
    int m_selectRevision = 0;
    int m_selectDetailRevision = 0;
};
