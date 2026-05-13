#pragma once

#include <QHash>
#include <QObject>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class Lr2SelectBarCell;

class Lr2SelectStateCache : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariant scores READ scores WRITE setScores NOTIFY scoresChanged)
    Q_PROPERTY(QVariant chartDifficultyCache READ chartDifficultyCache WRITE setChartDifficultyCache NOTIFY chartDifficultyCacheChanged)
    Q_PROPERTY(QVariant chartGroupCache READ chartGroupCache WRITE setChartGroupCache NOTIFY chartGroupCacheChanged)
    Q_PROPERTY(QVariant folderLampByKey READ folderLampByKey WRITE setFolderLampByKey NOTIFY folderLampByKeyChanged)
    Q_PROPERTY(QVariant folderDistributionByKey READ folderDistributionByKey WRITE setFolderDistributionByKey NOTIFY folderDistributionByKeyChanged)
    Q_PROPERTY(QVariant folderScoreCountsByKey READ folderScoreCountsByKey WRITE setFolderScoreCountsByKey NOTIFY folderScoreCountsByKeyChanged)
    Q_PROPERTY(QVariant historyStack READ historyStack WRITE setHistoryStack NOTIFY historyStackChanged)
    Q_PROPERTY(QVariant playerStats READ playerStats WRITE setPlayerStats NOTIFY playerStatsChanged)
    Q_PROPERTY(int profileOffset READ profileOffset WRITE setProfileOffset NOTIFY profileOffsetChanged)
    Q_PROPERTY(QVariant rankingClearCounts READ rankingClearCounts WRITE setRankingClearCounts NOTIFY rankingClearCountsChanged)
    Q_PROPERTY(QString rankingStatsMd5 READ rankingStatsMd5 WRITE setRankingStatsMd5 NOTIFY rankingStatsMd5Changed)
    Q_PROPERTY(int rankingPlayerRank READ rankingPlayerRank WRITE setRankingPlayerRank NOTIFY rankingPlayerRankChanged)
    Q_PROPERTY(int rankingPlayerCount READ rankingPlayerCount WRITE setRankingPlayerCount NOTIFY rankingPlayerCountChanged)
    Q_PROPERTY(int rankingTotalPlayCount READ rankingTotalPlayCount WRITE setRankingTotalPlayCount NOTIFY rankingTotalPlayCountChanged)

public:
    explicit Lr2SelectStateCache(QObject* parent = nullptr);

    QVariant scores() const;
    void setScores(const QVariant& value);

    QVariant chartDifficultyCache() const;
    void setChartDifficultyCache(const QVariant& value);

    QVariant chartGroupCache() const;
    void setChartGroupCache(const QVariant& value);

    QVariant folderLampByKey() const;
    void setFolderLampByKey(const QVariant& value);

    QVariant folderDistributionByKey() const;
    void setFolderDistributionByKey(const QVariant& value);

    QVariant folderScoreCountsByKey() const;
    void setFolderScoreCountsByKey(const QVariant& value);

    QVariant historyStack() const;
    void setHistoryStack(const QVariant& value);

    QVariant playerStats() const;
    void setPlayerStats(const QVariant& value);

    int profileOffset() const;
    void setProfileOffset(int value);

    QVariant rankingClearCounts() const;
    void setRankingClearCounts(const QVariant& value);

    QString rankingStatsMd5() const;
    void setRankingStatsMd5(const QString& value);

    int rankingPlayerRank() const;
    void setRankingPlayerRank(int value);

    int rankingPlayerCount() const;
    void setRankingPlayerCount(int value);

    int rankingTotalPlayCount() const;
    void setRankingTotalPlayCount(int value);

    Q_INVOKABLE QVariantMap refreshSelectedState(const QVariant& item,
                                                 int focusedIndex,
                                                 int scoreRevision,
                                                 int listRevision,
                                                 bool rankingMode,
                                                 const QVariant& rankingBaseItem);
    Q_INVOKABLE QVariantMap scoreSummaryForItem(const QVariant& item,
                                                int scoreRevision,
                                                int listRevision);
    Q_INVOKABLE QVariantMap scoreLampRankForItem(const QVariant& item,
                                                 int scoreRevision,
                                                 int listRevision);
    Q_INVOKABLE QVariantList scoreOptionIdsForItem(const QVariant& item,
                                                   int scoreRevision,
                                                   int listRevision);
    Q_INVOKABLE QVariantMap folderSummaryFromScores(const QVariant& result) const;
    Q_INVOKABLE QVariantMap difficultyStateForChart(const QVariant& chart,
                                                     int scoreRevision,
                                                     int listRevision);
    Q_INVOKABLE qreal barGraphValue(int type,
                                    int logicalCount,
                                    qreal currentNormalizedVisualIndex) const;
    Q_INVOKABLE QVariant textValue(int st) const;
    Q_INVOKABLE int numberValue(int num) const;
    Q_INVOKABLE QVariantMap barCellCore(const QVariant& item,
                                        const QVariant& barTitleTypes,
                                        int scoreRevision,
                                        int listRevision,
                                        int folderLampRevision);
    Q_INVOKABLE void updateBarCell(QObject* cell,
                                   int row,
                                   const QVariant& item,
                                   const QVariant& barTitleTypes,
                                   int scoreRevision,
                                   int listRevision,
                                   int folderLampRevision);

signals:
    void scoresChanged();
    void chartDifficultyCacheChanged();
    void chartGroupCacheChanged();
    void folderLampByKeyChanged();
    void folderDistributionByKeyChanged();
    void folderScoreCountsByKeyChanged();
    void historyStackChanged();
    void playerStatsChanged();
    void profileOffsetChanged();
    void rankingClearCountsChanged();
    void rankingStatsMd5Changed();
    void rankingPlayerRankChanged();
    void rankingPlayerCountChanged();
    void rankingTotalPlayCountChanged();

private:
    enum class ItemKind {
        Empty,
        Ranking,
        Chart,
        Entry,
        Course,
        Level,
        Table,
        Folder,
        Other,
    };

    struct BarCellCoreData {
        QString key;
        bool valid = false;
        QString text;
        int titleType = 0;
        int bodyType = 1;
        int playLevel = 0;
        int difficulty = 0;
        int keymode = 0;
        bool ranking = false;
        bool chartLike = false;
        bool entryLike = false;
        bool folderLike = false;
        int lamp = 0;
        int rank = 0;
        int labelMask = 0;
        QVariantList graphLamps;
        QVariantList graphRanks;
    };

    void clearScoreCaches();
    void clearDifficultyCache();
    void clearBarCellCache();
    void syncRevisions(int scoreRevision, int listRevision);
    void syncBarCellRevisions(const QVariant& barTitleTypes,
                              int scoreRevision,
                              int listRevision,
                              int folderLampRevision);

    QVariant chartDataForItem(const QVariant& item,
                              bool rankingMode,
                              const QVariant& rankingBaseItem) const;
    QVariantList entryScores(const QVariant& item) const;
    QVariantMap buildScoreSummary(const QVariantList& scoreList) const;
    QVariantMap buildScoreLampRank(const QVariantList& scoreList) const;
    QVariantMap buildDifficultyState(const QVariant& chart,
                                     int scoreRevision,
                                     int listRevision);
    QVariantList buildScoreOptionIds(const QVariantMap& summary) const;
    QVariantMap statsForScore(const QVariant& score) const;

    QVariantMap cachedScoreSummary(const QVariant& item);
    QVariantMap cachedScoreLampRank(const QVariant& item);
    QVariantList cachedScoreOptionIds(const QVariant& item);
    BarCellCoreData cachedBarCellCoreData(const QVariant& item,
                                          const QVariant& barTitleTypes,
                                          int scoreRevision,
                                          int listRevision,
                                          int folderLampRevision);
    QVariantMap barCellCoreMap(const BarCellCoreData& core) const;

    ItemKind itemKind(const QVariant& item) const;
    QString itemTypeName(const QVariant& item) const;
    QString entryIdentifier(const QVariant& item) const;
    QString entrySelectionKey(const QVariant& item, int fallbackIndex) const;
    QString scoreSummaryKey(const QVariant& item) const;
    QString chartDifficultyGroupKey(const QVariant& chart) const;
    QString chartGroupKey(const QVariant& chart) const;
    QString displayName(const QVariant& item) const;
    QString folderLampKey(const QVariant& item) const;
    QString barCellKey(const QVariant& item) const;
    QString barTitleTypesKey(const QVariant& barTitleTypes) const;
    int entryDifficulty(const QVariant& item) const;
    int entryPlayLevel(const QVariant& item) const;
    int entryLamp(const QVariant& item);
    int titleTypeWithFallback(const QVariant& barTitleTypes, int preferred, int fallback) const;
    int rankingEntryRank(const QVariant& item) const;
    int chartLabelMask(const QVariant& item) const;
    QVariantMap folderDistribution(const QVariant& item) const;

    QVariantMap emptyScoreSummary() const;
    QVariantMap emptyScoreCounts() const;
    QVariantMap emptyDifficultyState() const;
    QVariant chartWrapperForData(const QVariant& chartData) const;

    QVariant m_scores;
    QVariant m_chartDifficultyCache;
    QVariant m_chartGroupCache;
    QVariant m_folderLampByKey;
    QVariant m_folderDistributionByKey;
    QVariant m_folderScoreCountsByKey;
    QVariant m_historyStack;
    QVariant m_playerStats;
    int m_profileOffset = 0;
    QVariant m_rankingClearCounts;
    QString m_rankingStatsMd5;
    int m_rankingPlayerRank = 0;
    int m_rankingPlayerCount = 0;
    int m_rankingTotalPlayCount = 0;
    QHash<QString, QVariantMap> m_selectedStateCache;
    QHash<QString, QVariantMap> m_scoreSummaryCache;
    QHash<QString, QVariantMap> m_scoreLampRankCache;
    QHash<QString, QVariantList> m_scoreOptionIdsCache;
    QHash<QString, QVariantMap> m_difficultyStateCache;
    QHash<QString, BarCellCoreData> m_barCellDataCache;
    QVariantMap m_selectedState;
    QString m_selectedStateKey;
    int m_scoreRevision = -1;
    int m_listRevision = -1;
    int m_barCellScoreRevision = -1;
    int m_barCellListRevision = -1;
    int m_barCellFolderLampRevision = -1;
    QString m_barCellTitleTypesKey;
};
