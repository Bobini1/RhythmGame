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
    Q_PROPERTY(QString bestComboBreaksGuid MEMBER bestComboBreaksGuid CONSTANT)
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
class OnlineRankingModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

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
    Q_PROPERTY(qint64 lastPlayedGte READ getLastPlayedGte WRITE setLastPlayedGte
                 NOTIFY lastPlayedGteChanged)
    Q_PROPERTY(qint64 lastPlayedLte READ getLastPlayedLte WRITE setLastPlayedLte
                 NOTIFY lastPlayedLteChanged)

    Q_PROPERTY(
      qint64 dateGte READ getDateGte WRITE setDateGte NOTIFY dateGteChanged)
    Q_PROPERTY(
      qint64 dateLte READ getDateLte WRITE setDateLte NOTIFY dateLteChanged)

    Q_PROPERTY(double scorePctGte READ getScorePctGte WRITE setScorePctGte
                 NOTIFY scorePctGteChanged)
    Q_PROPERTY(double scorePctLte READ getScorePctLte WRITE setScorePctLte
                 NOTIFY scorePctLteChanged)

    Q_PROPERTY(
      int comboGte READ getComboGte WRITE setComboGte NOTIFY comboGteChanged)
    Q_PROPERTY(
      int comboLte READ getComboLte WRITE setComboLte NOTIFY comboLteChanged)

    Q_PROPERTY(int missCountGte READ getMissCountGte WRITE setMissCountGte
                 NOTIFY missCountGteChanged)
    Q_PROPERTY(int missCountLte READ getMissCountLte WRITE setMissCountLte
                 NOTIFY missCountLteChanged)

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
    auto rowCount(const QModelIndex& parent = {}) const -> int override;
    auto data(const QModelIndex& index, int role = Qt::DisplayRole) const
      -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

    auto getMd5() const -> QString;
    void setMd5(const QString& md5);

    void resetMd5();

    auto isLoading() const -> bool;

    auto getLimit() const -> int;
    void setLimit(int limit);

    auto getOffset() const -> int;
    void setOffset(int offset);

    auto getSortBy() const -> SortableColumn;
    void setSortBy(SortableColumn sortBy);

    auto getSortDir() const -> SortDirection;
    void setSortDir(SortDirection sortDir);

    auto getSearch() const -> QString;
    void setSearch(const QString& search);

    auto getLastPlayedGte() const -> qint64;
    void setLastPlayedGte(qint64 val);

    auto getLastPlayedLte() const -> qint64;
    void setLastPlayedLte(qint64 val);

    auto getDateGte() const -> qint64;
    void setDateGte(qint64 val);

    auto getDateLte() const -> qint64;
    void setDateLte(qint64 val);

    auto getScorePctGte() const -> double;
    void setScorePctGte(double val);

    auto getScorePctLte() const -> double;
    void setScorePctLte(double val);

    auto getComboGte() const -> int;
    void setComboGte(int val);

    auto getComboLte() const -> int;
    void setComboLte(int val);

    auto getMissCountGte() const -> int;
    void setMissCountGte(int val);

    auto getMissCountLte() const -> int;
    void setMissCountLte(int val);

    auto getClearCounts() const -> QVariantMap;
    auto getScoreCount() const -> int;
    auto getPlayerCount() const -> int;
    auto getRankingEntries() const -> const QList<RankingEntry>&;

    Q_INVOKABLE void cancelPending();
    Q_INVOKABLE void refresh();

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
    void lastPlayedGteChanged();
    void lastPlayedLteChanged();

    void dateGteChanged();
    void dateLteChanged();

    void scorePctGteChanged();
    void scorePctLteChanged();

    void comboGteChanged();
    void comboLteChanged();

    void missCountGteChanged();
    void missCountLteChanged();
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
    auto buildUrl() const -> QUrl;
    void performJsonGet(const QString& url,
                        std::function<void(const QJsonDocument&)> onSuccess,
                        std::function<void(const QString&)> onError);

    QNetworkRequestFactory networkRequestFactory;

    QString currentMd5;
    bool currentlyLoading{ false };
    int currentLimit{ 0 };
    int currentOffset{ 0 };
    SortableColumn currentSortBy{ SortableColumn::None };
    SortDirection currentSortDir{ SortDirection::Desc };
    QString currentSearch;

    // Filter parameters
    qint64 currentLastPlayedGte{ -1 };
    qint64 currentLastPlayedLte{ -1 };

    qint64 currentDateGte{ -1 };
    qint64 currentDateLte{ -1 };

    double currentScorePctGte{ -1.0 };
    double currentScorePctLte{ -1.0 };

    int currentComboGte{ -1 };
    int currentComboLte{ -1 };

    int currentMissCountGte{ -1 };
    int currentMissCountLte{ -1 };

    QList<RankingEntry> entries;
    QList<QNetworkReply*> pendingReplies;
    QVariantMap clearCounts;
    int scoreCount{ 0 };
    int playerCount{ 0 };
};

} // namespace qml_components

#endif // RHYTHMGAME_ONLINERANKINGMODEL_H
