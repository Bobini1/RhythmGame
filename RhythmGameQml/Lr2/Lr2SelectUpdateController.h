#pragma once

#include <QMetaObject>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <initializer_list>

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
    Q_PROPERTY(int selectPanel READ selectPanel WRITE setSelectPanel NOTIFY selectPanelChanged)
    Q_PROPERTY(QVariant parseActiveOptions READ parseActiveOptions WRITE setParseActiveOptions NOTIFY parseActiveOptionsChanged)
    Q_PROPERTY(QVariant runtimeActiveOptions READ runtimeActiveOptions WRITE setRuntimeActiveOptions NOTIFY runtimeActiveOptionsChanged)
    Q_PROPERTY(QVariant barActiveOptions READ barActiveOptions WRITE setBarActiveOptions NOTIFY barActiveOptionsChanged)
    Q_PROPERTY(QVariant baseActiveOptions READ baseActiveOptions WRITE setBaseActiveOptions NOTIFY baseActiveOptionsChanged)
    Q_PROPERTY(QVariant selectCommonActiveOptions READ selectCommonActiveOptions WRITE setSelectCommonActiveOptions NOTIFY selectCommonActiveOptionsChanged)
    Q_PROPERTY(bool selectCommonActiveOptionsReady READ selectCommonActiveOptionsReady WRITE setSelectCommonActiveOptionsReady NOTIFY selectCommonActiveOptionsReadyChanged)
    Q_PROPERTY(QVariant selectRuntimeActiveOptions READ selectRuntimeActiveOptions WRITE setSelectRuntimeActiveOptions NOTIFY selectRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant gameplayRuntimeActiveOptions READ gameplayRuntimeActiveOptions WRITE setGameplayRuntimeActiveOptions NOTIFY gameplayRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant barRuntimeActiveOptions READ barRuntimeActiveOptions WRITE setBarRuntimeActiveOptions NOTIFY barRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant baseRuntimeActiveOptions READ baseRuntimeActiveOptions WRITE setBaseRuntimeActiveOptions NOTIFY baseRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant selectCommonRuntimeActiveOptions READ selectCommonRuntimeActiveOptions WRITE setSelectCommonRuntimeActiveOptions NOTIFY selectCommonRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant selectRequiredRuntimeActiveOptions READ selectRequiredRuntimeActiveOptions WRITE setSelectRequiredRuntimeActiveOptions NOTIFY selectRequiredRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant selectRuntimeGeneratedActiveOptions READ selectRuntimeGeneratedActiveOptions WRITE setSelectRuntimeGeneratedActiveOptions NOTIFY selectRuntimeGeneratedActiveOptionsChanged)
    Q_PROPERTY(QVariant selectDetailRuntimeActiveOptions READ selectDetailRuntimeActiveOptions WRITE setSelectDetailRuntimeActiveOptions NOTIFY selectDetailRuntimeActiveOptionsChanged)
    Q_PROPERTY(QVariant screenRuntimeActiveOptions READ screenRuntimeActiveOptions WRITE setScreenRuntimeActiveOptions NOTIFY screenRuntimeActiveOptionsChanged)
    Q_PROPERTY(bool gameplayScreen READ gameplayScreen WRITE setGameplayScreen NOTIFY gameplayScreenChanged)
    Q_PROPERTY(bool selectReplayOptionsUsed READ selectReplayOptionsUsed NOTIFY selectOptionUsageChanged)
    Q_PROPERTY(bool selectScoreOptionIdsUsed READ selectScoreOptionIdsUsed NOTIFY selectOptionUsageChanged)
    Q_PROPERTY(bool selectEntryStatusOptionsUsed READ selectEntryStatusOptionsUsed NOTIFY selectOptionUsageChanged)
    Q_PROPERTY(bool selectDifficultyBarOptionsUsed READ selectDifficultyBarOptionsUsed NOTIFY selectOptionUsageChanged)
    Q_PROPERTY(bool selectDifficultyLampOptionsUsed READ selectDifficultyLampOptionsUsed NOTIFY selectOptionUsageChanged)
    Q_PROPERTY(bool selectDifficultyStateUsed READ selectDifficultyStateUsed NOTIFY selectOptionUsageChanged)
    Q_PROPERTY(bool selectCourseDetailOptionsUsed READ selectCourseDetailOptionsUsed NOTIFY selectOptionUsageChanged)
    Q_PROPERTY(bool selectRankingStatusOptionsUsed READ selectRankingStatusOptionsUsed NOTIFY selectOptionUsageChanged)

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

    int selectPanel() const;
    void setSelectPanel(int panel);

    QVariant parseActiveOptions() const;
    void setParseActiveOptions(const QVariant& options);

    QVariant runtimeActiveOptions() const;
    void setRuntimeActiveOptions(const QVariant& options);

    QVariant barActiveOptions() const;
    void setBarActiveOptions(const QVariant& options);

    QVariant baseActiveOptions() const;
    void setBaseActiveOptions(const QVariant& options);

    QVariant selectCommonActiveOptions() const;
    void setSelectCommonActiveOptions(const QVariant& options);

    bool selectCommonActiveOptionsReady() const;
    void setSelectCommonActiveOptionsReady(bool ready);

    QVariant selectRuntimeActiveOptions() const;
    void setSelectRuntimeActiveOptions(const QVariant& options);

    QVariant gameplayRuntimeActiveOptions() const;
    void setGameplayRuntimeActiveOptions(const QVariant& options);

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

    QVariant selectDetailRuntimeActiveOptions() const;
    void setSelectDetailRuntimeActiveOptions(const QVariant& options);

    QVariant screenRuntimeActiveOptions() const;
    void setScreenRuntimeActiveOptions(const QVariant& options);

    bool gameplayScreen() const;
    void setGameplayScreen(bool active);

    bool selectReplayOptionsUsed() const;
    bool selectScoreOptionIdsUsed() const;
    bool selectEntryStatusOptionsUsed() const;
    bool selectDifficultyBarOptionsUsed() const;
    bool selectDifficultyLampOptionsUsed() const;
    bool selectDifficultyStateUsed() const;
    bool selectCourseDetailOptionsUsed() const;
    bool selectRankingStatusOptionsUsed() const;

    Q_INVOKABLE bool refreshBaseActiveOptions();
    Q_INVOKABLE bool refreshSelectRuntimeActiveOptions();
    Q_INVOKABLE bool refreshGameplayRuntimeActiveOptions();

signals:
    void skinModelChanged();
    void skinRuntimeChanged();
    void screenUpdatesActiveChanged();
    void effectiveScreenKeyChanged();
    void componentReadyChanged();
    void rankingModeChanged();
    void selectPanelChanged();
    void parseActiveOptionsChanged();
    void runtimeActiveOptionsChanged();
    void barActiveOptionsChanged();
    void baseActiveOptionsChanged();
    void selectCommonActiveOptionsChanged();
    void selectCommonActiveOptionsReadyChanged();
    void selectRuntimeActiveOptionsChanged();
    void gameplayRuntimeActiveOptionsChanged();
    void barRuntimeActiveOptionsChanged();
    void baseRuntimeActiveOptionsChanged();
    void selectCommonRuntimeActiveOptionsChanged();
    void selectRequiredRuntimeActiveOptionsChanged();
    void selectRuntimeGeneratedActiveOptionsChanged();
    void selectDetailRuntimeActiveOptionsChanged();
    void screenRuntimeActiveOptionsChanged();
    void gameplayScreenChanged();
    void selectOptionUsageChanged();

private slots:
    void refreshSkinOptionUsage();

private:
    void applyRuntimeActiveOptions(const QVariant& value);
    void refreshCurrentRuntimeActiveOptions();
    bool sameNumberArray(const QVariant& lhs, const QVariant& rhs) const;
    QVariant normalizedNumberArray(const QVariant& values) const;
    QVariantList mergedNumberArray(const QVariant& first, const QVariant& second) const;
    QVariantList selectStaticOptions(bool includeRankingOption, bool includePanelOption) const;
    QObject* skinModelObject() const;
    bool skinUsesOption(int option) const;
    bool skinUsesAnyOption(std::initializer_list<int> options) const;
    bool skinUsesOptionRange(int first, int last) const;
    bool skinUsesAnySelectElementOption(std::initializer_list<int> options) const;
    bool skinUsesSelectElementOptionRange(int first, int last) const;
    void appendUniqueOption(QVariantList& options, int option) const;
    void appendUniqueSkinOption(QVariantList& options, int option) const;
    void appendDefaultOptionIfMissing(QVariantList& options, const QList<int>& choices, int fallback) const;
    QList<int> numberList(const QVariant& values) const;

    QPointer<QObject> m_skinModel;
    QPointer<Lr2SkinRuntime> m_skinRuntime;
    bool m_screenUpdatesActive = false;
    QString m_effectiveScreenKey;
    bool m_componentReady = false;
    bool m_rankingMode = false;
    int m_selectPanel = 0;
    QMetaObject::Connection m_skinMetadataConnection;
    QSet<int> m_usedSkinOptions;
    QSet<int> m_usedSelectElementOptions;
    QList<int> m_effectiveSkinActiveOptions;
    bool m_skinUsesSelectDifficultySource = false;
    bool m_skinOptionFilterActive = false;
    bool m_selectElementOptionsAvailable = false;
    bool m_selectReplayOptionsUsed = true;
    bool m_selectScoreOptionIdsUsed = true;
    bool m_selectEntryStatusOptionsUsed = true;
    bool m_selectDifficultyBarOptionsUsed = true;
    bool m_selectDifficultyLampOptionsUsed = true;
    bool m_selectDifficultyStateUsed = true;
    bool m_selectCourseDetailOptionsUsed = true;
    bool m_selectRankingStatusOptionsUsed = true;
    QVariant m_parseActiveOptions;
    QVariant m_runtimeActiveOptions;
    QVariant m_barActiveOptions;
    QVariant m_baseActiveOptions;
    QVariant m_selectCommonActiveOptions;
    bool m_selectCommonActiveOptionsReady = false;
    QVariant m_selectRuntimeActiveOptions;
    QVariant m_gameplayRuntimeActiveOptions;
    QVariant m_barRuntimeActiveOptions;
    QVariant m_baseRuntimeActiveOptions;
    QVariant m_selectCommonRuntimeActiveOptions;
    QVariant m_selectRequiredRuntimeActiveOptions;
    QVariant m_selectRuntimeGeneratedActiveOptions;
    QVariant m_selectDetailRuntimeActiveOptions;
    QVariant m_screenRuntimeActiveOptions;
    bool m_gameplayScreen = false;
};
