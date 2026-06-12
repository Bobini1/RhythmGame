#pragma once

#include <QMetaObject>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <initializer_list>

#include "Lr2SkinRuntime.h"
#include "gameplay_logic/lr2_skin/Lr2SkinModel.h"

class Lr2SelectUpdateController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(gameplay_logic::lr2_skin::Lr2SkinModel* skinModel READ skinModel WRITE setSkinModel NOTIFY skinModelChanged)
    Q_PROPERTY(Lr2SkinRuntime* skinRuntime READ skinRuntime WRITE setSkinRuntime NOTIFY skinRuntimeChanged)
    Q_PROPERTY(bool screenUpdatesActive READ screenUpdatesActive WRITE setScreenUpdatesActive NOTIFY screenUpdatesActiveChanged)
    Q_PROPERTY(QString effectiveScreenKey READ effectiveScreenKey WRITE setEffectiveScreenKey NOTIFY effectiveScreenKeyChanged)
    Q_PROPERTY(bool componentReady READ componentReady WRITE setComponentReady NOTIFY componentReadyChanged)
    Q_PROPERTY(bool rankingMode READ rankingMode WRITE setRankingMode NOTIFY rankingModeChanged)
    Q_PROPERTY(int selectPanel READ selectPanel WRITE setSelectPanel NOTIFY selectPanelChanged)
    Q_PROPERTY(QList<int> parseActiveOptions READ parseActiveOptions WRITE setParseActiveOptions NOTIFY parseActiveOptionsChanged)
    Q_PROPERTY(QList<int> runtimeActiveOptions READ runtimeActiveOptions WRITE setRuntimeActiveOptions NOTIFY runtimeActiveOptionsChanged)
    Q_PROPERTY(QList<int> barActiveOptions READ barActiveOptions WRITE setBarActiveOptions NOTIFY barActiveOptionsChanged)
    Q_PROPERTY(QList<int> baseActiveOptions READ baseActiveOptions WRITE setBaseActiveOptions NOTIFY baseActiveOptionsChanged)
    Q_PROPERTY(QList<int> selectCommonActiveOptions READ selectCommonActiveOptions WRITE setSelectCommonActiveOptions NOTIFY selectCommonActiveOptionsChanged)
    Q_PROPERTY(bool selectCommonActiveOptionsReady READ selectCommonActiveOptionsReady WRITE setSelectCommonActiveOptionsReady NOTIFY selectCommonActiveOptionsReadyChanged)
    Q_PROPERTY(QList<int> selectRuntimeActiveOptions READ selectRuntimeActiveOptions WRITE setSelectRuntimeActiveOptions NOTIFY selectRuntimeActiveOptionsChanged)
    Q_PROPERTY(QList<int> gameplayRuntimeActiveOptions READ gameplayRuntimeActiveOptions WRITE setGameplayRuntimeActiveOptions NOTIFY gameplayRuntimeActiveOptionsChanged)
    Q_PROPERTY(QList<int> barRuntimeActiveOptions READ barRuntimeActiveOptions WRITE setBarRuntimeActiveOptions NOTIFY barRuntimeActiveOptionsChanged)
    Q_PROPERTY(QList<int> baseRuntimeActiveOptions READ baseRuntimeActiveOptions WRITE setBaseRuntimeActiveOptions NOTIFY baseRuntimeActiveOptionsChanged)
    Q_PROPERTY(QList<int> selectCommonRuntimeActiveOptions READ selectCommonRuntimeActiveOptions WRITE setSelectCommonRuntimeActiveOptions NOTIFY selectCommonRuntimeActiveOptionsChanged)
    Q_PROPERTY(QList<int> selectRequiredRuntimeActiveOptions READ selectRequiredRuntimeActiveOptions WRITE setSelectRequiredRuntimeActiveOptions NOTIFY selectRequiredRuntimeActiveOptionsChanged)
    Q_PROPERTY(QList<int> selectRuntimeGeneratedActiveOptions READ selectRuntimeGeneratedActiveOptions WRITE setSelectRuntimeGeneratedActiveOptions NOTIFY selectRuntimeGeneratedActiveOptionsChanged)
    Q_PROPERTY(QList<int> selectDetailRuntimeActiveOptions READ selectDetailRuntimeActiveOptions WRITE setSelectDetailRuntimeActiveOptions NOTIFY selectDetailRuntimeActiveOptionsChanged)
    Q_PROPERTY(QList<int> screenRuntimeActiveOptions READ screenRuntimeActiveOptions WRITE setScreenRuntimeActiveOptions NOTIFY screenRuntimeActiveOptionsChanged)
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

    gameplay_logic::lr2_skin::Lr2SkinModel* skinModel() const;
    void setSkinModel(gameplay_logic::lr2_skin::Lr2SkinModel* model);

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

    QList<int> parseActiveOptions() const;
    void setParseActiveOptions(const QList<int>& options);

    QList<int> runtimeActiveOptions() const;
    void setRuntimeActiveOptions(const QList<int>& options);

    QList<int> barActiveOptions() const;
    void setBarActiveOptions(const QList<int>& options);

    QList<int> baseActiveOptions() const;
    void setBaseActiveOptions(const QList<int>& options);

    QList<int> selectCommonActiveOptions() const;
    void setSelectCommonActiveOptions(const QList<int>& options);

    bool selectCommonActiveOptionsReady() const;
    void setSelectCommonActiveOptionsReady(bool ready);

    QList<int> selectRuntimeActiveOptions() const;
    void setSelectRuntimeActiveOptions(const QList<int>& options);

    QList<int> gameplayRuntimeActiveOptions() const;
    void setGameplayRuntimeActiveOptions(const QList<int>& options);

    QList<int> barRuntimeActiveOptions() const;
    void setBarRuntimeActiveOptions(const QList<int>& options);

    QList<int> baseRuntimeActiveOptions() const;
    void setBaseRuntimeActiveOptions(const QList<int>& options);

    QList<int> selectCommonRuntimeActiveOptions() const;
    void setSelectCommonRuntimeActiveOptions(const QList<int>& options);

    QList<int> selectRequiredRuntimeActiveOptions() const;
    void setSelectRequiredRuntimeActiveOptions(const QList<int>& options);

    QList<int> selectRuntimeGeneratedActiveOptions() const;
    void setSelectRuntimeGeneratedActiveOptions(const QList<int>& options);

    QList<int> selectDetailRuntimeActiveOptions() const;
    void setSelectDetailRuntimeActiveOptions(const QList<int>& options);

    QList<int> screenRuntimeActiveOptions() const;
    void setScreenRuntimeActiveOptions(const QList<int>& options);

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
    Q_INVOKABLE bool updateSelectRuntimeGeneratedActiveOptions(const QList<int>& options);
    Q_INVOKABLE bool updateSelectDetailRuntimeActiveOptions(const QList<int>& options);

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
    void applyRuntimeActiveOptions(const QList<int>& value);
    void applyChangedRuntimeActiveOptions(const QList<int>& value);
    void refreshCurrentRuntimeActiveOptions();
    QList<int> mergedNumberArray(const QList<int>& first, const QList<int>& second) const;
    QList<int> selectStaticOptions(bool includeRankingOption, bool includePanelOption) const;
    bool skinUsesOption(int option) const;
    bool skinUsesAnyOption(std::initializer_list<int> options) const;
    bool skinUsesOptionRange(int first, int last) const;
    bool skinUsesAnySelectElementOption(std::initializer_list<int> options) const;
    bool skinUsesSelectElementOptionRange(int first, int last) const;
    void appendUniqueOption(QList<int>& options, int option) const;
    void appendUniqueSkinOption(QList<int>& options, int option) const;
    void appendDefaultOptionIfMissing(QList<int>& options, const QList<int>& choices, int fallback) const;
    QList<int> intList(const QVariantList& values) const;

    QPointer<gameplay_logic::lr2_skin::Lr2SkinModel> m_skinModel;
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
    QList<int> m_parseActiveOptions;
    QList<int> m_runtimeActiveOptions;
    QList<int> m_barActiveOptions;
    QList<int> m_baseActiveOptions;
    QList<int> m_selectCommonActiveOptions;
    bool m_selectCommonActiveOptionsReady = false;
    bool m_selectRuntimeActiveOptionsDirty = true;
    QList<int> m_selectRuntimeActiveOptions;
    QList<int> m_gameplayRuntimeActiveOptions;
    QList<int> m_barRuntimeActiveOptions;
    QList<int> m_baseRuntimeActiveOptions;
    QList<int> m_selectCommonRuntimeActiveOptions;
    QList<int> m_selectRequiredRuntimeActiveOptions;
    QList<int> m_selectRuntimeGeneratedActiveOptions;
    QList<int> m_selectDetailRuntimeActiveOptions;
    QList<int> m_screenRuntimeActiveOptions;
    bool m_gameplayScreen = false;
};
