#ifndef RHYTHMGAME_ONLINERANKINGMODEL_H
#define RHYTHMGAME_ONLINERANKINGMODEL_H

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequestFactory>
#include <qqmlintegration.h>
#include <stop_token>

namespace qml_components {
class ProfileList;

class OnlineRankingModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
  public:
    struct RankingEntry
    {
        Q_GADGET
        Q_PROPERTY(qint64 userId MEMBER userId CONSTANT)
        Q_PROPERTY(QString userName MEMBER userName CONSTANT)
        Q_PROPERTY(QString userImage MEMBER userImage CONSTANT)
        Q_PROPERTY(double bestPoints MEMBER bestPoints CONSTANT)
        Q_PROPERTY(double maxPoints MEMBER maxPoints CONSTANT)
        Q_PROPERTY(int bestCombo MEMBER bestCombo CONSTANT)
        Q_PROPERTY(int maxHits MEMBER maxHits CONSTANT)
        Q_PROPERTY(QString bestClearType MEMBER bestClearType CONSTANT)
        Q_PROPERTY(int bestComboBreaks MEMBER bestComboBreaks CONSTANT)
        Q_PROPERTY(qint64 latestDate MEMBER latestDate CONSTANT)
        Q_PROPERTY(QString bestPointsGuid MEMBER bestPointsGuid CONSTANT)
        Q_PROPERTY(QString bestComboGuid MEMBER bestComboGuid CONSTANT)
        Q_PROPERTY(
          QString bestComboBreaksGuid MEMBER bestComboBreaksGuid CONSTANT)
        Q_PROPERTY(QString bestClearTypeGuid MEMBER bestClearTypeGuid CONSTANT)
        Q_PROPERTY(QString latestDateGuid MEMBER latestDateGuid CONSTANT)
        Q_PROPERTY(qint64 scoreCount MEMBER scoreCount CONSTANT)
      public:
        qint64 userId;
        QString userName;
        QString userImage;
        double bestPoints{};
        double maxPoints{};
        int bestCombo{};
        int maxHits{};
        QString bestClearType;
        int bestComboBreaks{};
        qint64 latestDate{};
        QString bestPointsGuid;
        QString bestComboGuid;
        QString bestComboBreaksGuid;
        QString bestClearTypeGuid;
        QString latestDateGuid;
        int scoreCount{};
    };

  private:
    Q_PROPERTY(
      QString md5 READ getMd5 WRITE setMd5 NOTIFY md5Changed RESET resetMd5)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int limit READ getLimit WRITE setLimit NOTIFY limitChanged)
    Q_PROPERTY(int offset READ getOffset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(
      SortableColumn sortBy READ getSortBy WRITE setSortBy NOTIFY sortByChanged)
    Q_PROPERTY(SortDirection sortDir READ getSortDir WRITE setSortDir NOTIFY
                 sortDirChanged)
    Q_PROPERTY(
      QString search READ getSearch WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(
      QVariantMap clearCounts READ getClearCounts NOTIFY clearCountsChanged)
    Q_PROPERTY(int scoreCount READ getScoreCount NOTIFY scoreCountChanged)
    Q_PROPERTY(int playerCount READ getPlayerCount NOTIFY playerCountChanged)
    Q_PROPERTY(QString webApiUrl READ getWebApiUrl WRITE setWebApiUrl NOTIFY
                 webApiUrlChanged)
    Q_PROPERTY(QList<RankingEntry> rankingEntries READ getRankingEntries NOTIFY
                 rankingEntriesChanged)

  public:
    enum Roles
    {
        UserIdRole = Qt::UserRole + 1,
        UserNameRole,
        UserImageRole,
        BestPointsRole,
        MaxPointsRole,
        BestComboRole,
        MaxHitsRole,
        BestPointsGuidRole,
        BestComboGuidRole,
        BestComboBreaksGuidRole,
        BestClearTypeGuidRole,
        LatestDateGuidRole,
        BestClearTypeRole,
        BestComboBreaksRole,
        LatestDateRole,
        ScoreCountRole,
        RankRole,
    };
    Q_ENUM(Roles)

    enum class SortableColumn
    {
        None = 0,
        Player,
        ScorePct,
        Grade,
        Combo,
        ComboBreaks,
        ClearType,
        Date,
        PlayCount,
    };
    Q_ENUM(SortableColumn)

    enum class SortDirection
    {
        None = 0,
        Asc,
        Desc,
    };
    Q_ENUM(SortDirection)

    explicit OnlineRankingModel(QObject* parent = nullptr);

    // QAbstractListModel interface
    [[nodiscard]] auto rowCount(const QModelIndex& parent = {}) const
      -> int override;
    [[nodiscard]] auto data(const QModelIndex& index,
                            int role = Qt::DisplayRole) const
      -> QVariant override;
    [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;

    [[nodiscard]] auto getMd5() const -> QString;
    void setMd5(const QString& md5);

    void resetMd5();

    [[nodiscard]] auto isLoading() const -> bool;

    [[nodiscard]] auto getLimit() const -> int;
    void setLimit(int limit);

    [[nodiscard]] auto getOffset() const -> int;
    void setOffset(int offset);

    [[nodiscard]] auto getSortBy() const -> SortableColumn;
    void setSortBy(SortableColumn sortBy);

    [[nodiscard]] auto getSortDir() const -> SortDirection;
    void setSortDir(SortDirection sortDir);

    [[nodiscard]] auto getSearch() const -> QString;
    void setSearch(const QString& search);

    [[nodiscard]] auto getClearCounts() const -> QVariantMap;
    [[nodiscard]] auto getScoreCount() const -> int;
    [[nodiscard]] auto getPlayerCount() const -> int;
    auto getRankingEntries() const -> const QList<RankingEntry>&;

    Q_INVOKABLE void cancelPending();

    void setWebApiUrl(const QString& baseUrl);
    auto getWebApiUrl() const -> QString;

    inline static QNetworkAccessManager* networkManager = nullptr;
    inline static ProfileList* profileList = nullptr;

  signals:
    void md5Changed();
    void loadingChanged();
    void limitChanged();
    void offsetChanged();
    void sortByChanged();
    void sortDirChanged();
    void searchChanged();
    void clearCountsChanged();
    void scoreCountChanged();
    void playerCountChanged();
    void webApiUrlChanged();
    void rankingEntriesChanged();

    void cancelPendingRequested();

  private:
    void fetch();
    void setLoading(bool loading);
    void setPlayerCount(int count);
    void setScoreCount(int count);
    void setClearCounts(QVariantMap counts);
    [[nodiscard]] auto buildUrl() const -> QUrl;
    void performJsonGet(const QString& url,
                        std::function<void(const QJsonDocument&)> onSuccess,
                        std::function<void(const QString&)> onError);

    QNetworkRequestFactory networkRequestFactory;

    QString currentMd5;
    bool currentlyLoading{ false };
    int currentLimit{ 10 };
    int currentOffset{ 0 };
    SortableColumn currentSortBy{ SortableColumn::None };
    SortDirection currentSortDir{ SortDirection::Desc };
    QString currentSearch;

    QList<RankingEntry> entries;
    QList<QNetworkReply*> pendingReplies;
    QVariantMap clearCounts;
    int scoreCount{ 0 };
    int playerCount{ 0 };
};

} // namespace qml_components

#endif // RHYTHMGAME_ONLINERANKINGMODEL_H
