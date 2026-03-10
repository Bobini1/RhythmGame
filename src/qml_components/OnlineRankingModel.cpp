#include "OnlineRankingModel.h"

#include "ProfileList.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <functional>

namespace qml_components {

void
OnlineRankingModel::performJsonGet(
  const QString& url,
  const std::function<void(const QJsonDocument&)> onSuccess,
  const std::function<void(const QString&)> onError)
{
    const auto request = networkRequestFactory.createRequest(url);

    QNetworkReply* reply = networkManager->get(request);

    connect(this, &OnlineRankingModel::cancelPendingRequested, reply, [reply] {
        reply->abort();
    });

    connect(
      reply, &QNetworkReply::finished, reply, [reply, onSuccess, onError]() {
          reply->deleteLater();

          if (reply->error() == QNetworkReply::OperationCanceledError) {
              return;
          }
          if (reply->error() == QNetworkReply::ContentNotFoundError) {
              return;
          }
          if (reply->error() != QNetworkReply::NoError) {
              onError(reply->errorString());
              return;
          }
          const QByteArray data = reply->readAll();

          QJsonParseError parseErr;
          const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
          if (parseErr.error != QJsonParseError::NoError) {
              onError(QString::fromLatin1("JSON parse error: %1")
                        .arg(parseErr.errorString()));
              return;
          }

          onSuccess(doc);
      });
}

OnlineRankingModel::OnlineRankingModel(QObject* parent)
  : QAbstractListModel(parent)
{
    networkRequestFactory.setBaseUrl(profileList->getMainProfile()
                                       ->getVars()
                                       ->getGeneralVars()
                                       ->getWebApiUrl());
    connect(profileList, &ProfileList::mainProfileChanged, this, [this]() {
        networkRequestFactory.setBaseUrl(profileList->getMainProfile()
                                           ->getVars()
                                           ->getGeneralVars()
                                           ->getWebApiUrl());
        cancelPending();
        fetch();
    });
}

auto
OnlineRankingModel::rowCount(const QModelIndex& parent) const -> int
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(entries.size());
}

auto
OnlineRankingModel::data(const QModelIndex& index, int role) const -> QVariant
{
    if (!index.isValid() || index.row() < 0 || index.row() >= entries.size())
        return {};

    const auto& e = entries.at(index.row());
    switch (role) {
        case UserIdRole:
            return e.userId;
        case UserNameRole:
            return e.userName;
        case UserImageRole:
            return e.userImage;
        case BestPointsRole:
            return e.bestPoints;
        case MaxPointsRole:
            return e.maxPoints;
        case BestComboRole:
            return e.bestCombo;
        case MaxHitsRole:
            return e.maxHits;
        case BestClearTypeRole:
            return e.bestClearType;
        case BestComboBreaksRole:
            return e.bestComboBreaks;
        case LatestDateRole:
            return QVariant::fromValue(e.latestDate);
        case ScoreCountRole:
            return e.scoreCount;
        case RankRole:
            return currentOffset + index.row() + 1;
        default:
            return {};
    }
}

auto
OnlineRankingModel::roleNames() const -> QHash<int, QByteArray>
{
    return {
        { UserIdRole, "userId" },
        { UserNameRole, "userName" },
        { UserImageRole, "userImage" },
        { BestPointsRole, "bestPoints" },
        { MaxPointsRole, "maxPoints" },
        { BestComboRole, "bestCombo" },
        { MaxHitsRole, "maxHits" },
        { BestClearTypeRole, "bestClearType" },
        { BestComboBreaksRole, "bestComboBreaks" },
        { LatestDateRole, "latestDate" },
        { ScoreCountRole, "scoreCount" },
        { RankRole, "rank" },
    };
}

auto
OnlineRankingModel::buildUrl() const -> QUrl
{
    auto url = QUrl("score-summaries");
    QUrlQuery q;
    q.addQueryItem("limit", QString::number(currentLimit));
    q.addQueryItem("offset", QString::number(currentOffset));
    q.addQueryItem("md5", currentMd5);
    // map enum to server's sortBy string
    if (currentSortBy != SortableColumn::None) {
        QString sortByStr;
        switch (currentSortBy) {
            case SortableColumn::Player:
                sortByStr = "player";
                break;
            case SortableColumn::ScorePct:
                sortByStr = "score_pct";
                break;
            case SortableColumn::Grade:
                sortByStr = "grade";
                break;
            case SortableColumn::Combo:
                sortByStr = "combo";
                break;
            case SortableColumn::ComboBreaks:
                sortByStr = "combo_breaks";
                break;
            case SortableColumn::ClearType:
                sortByStr = "clear_type";
                break;
            case SortableColumn::Date:
                sortByStr = "date";
                break;
            case SortableColumn::PlayCount:
                sortByStr = "play_count";
                break;
            default:
                sortByStr = QString();
                break;
        }
        if (!sortByStr.isEmpty())
            q.addQueryItem("sortBy", sortByStr);
    }
    if (currentSortDir != SortDirection::None)
        q.addQueryItem("sortDir",
                       currentSortDir == SortDirection::Asc ? "asc" : "desc");
    if (!currentSearch.isEmpty())
        q.addQueryItem("search", currentSearch);
    url.setQuery(q);
    return url;
}

void
OnlineRankingModel::fetch()
{
    setPlayerCount(0);
    setScoreCount(0);
    setClearCounts({});
    if (currentMd5.isEmpty()) {
        setLoading(false);
        return;
    }

    cancelPending();
    setLoading(true);

    performJsonGet(
      buildUrl().toString(),
      [this](const QJsonDocument& doc) {
          if (!doc.isArray()) {
              spdlog::error("OnlineRankingModel: response is not a JSON array");
              setLoading(false);
              return;
          }

          QList<RankingEntry> newEntries;
          newEntries.reserve(doc.array().size());
          for (const auto& item : doc.array()) {
              if (!item.isObject())
                  continue;
              const auto obj = item.toObject();
              RankingEntry entry;
              auto user = obj.value("user").toObject();
              entry.userId = user.value("id").toInt();
              entry.userName = user.value("name").toString();
              entry.userImage = user.value("image").toString();
              entry.bestPoints = obj.value("bestPoints").toDouble();
              entry.maxPoints = obj.value("maxPoints").toDouble();
              entry.bestCombo = obj.value("bestCombo").toInt();
              entry.maxHits = obj.value("maxHits").toInt();
              entry.bestClearType = obj.value("bestClearType").toString();
              entry.bestComboBreaks = obj.value("bestComboBreaks").toInt();
              entry.latestDate = obj.value("latestDate").toInteger();
              entry.scoreCount = obj.value("scoreCount").toInt();
              newEntries.append(std::move(entry));
          }

          beginResetModel();
          entries = std::move(newEntries);
          endResetModel();
          setLoading(false);
      },
      [this](const QString& err) {
          spdlog::error("OnlineRankingModel fetch failed: {}",
                        err.toStdString());
          setLoading(false);
      });

    // Player/score counts
    performJsonGet(
      QString("charts/%1?fields=scoreCount,playerCount").arg(currentMd5),
      [this](const QJsonDocument& doc) {
          if (!doc.isObject()) {
              spdlog::error(
                "OnlineRankingModel count response is not a JSON object");
              return;
          }
          const auto obj = doc.object();
          setPlayerCount(obj.value("playerCount").toInt());
          setScoreCount(obj.value("scoreCount").toInt());
      },
      [](const QString& err) {
          spdlog::error("OnlineRankingModel count fetch failed: {}",
                        err.toStdString());
      });

    // Clear-type summary request
    auto clearTypesRequest =
      QNetworkRequest(networkRequestFactory.createRequest());
    performJsonGet(
      QString("score-summaries?md5=%1&fields=bestClearType").arg(currentMd5),
      [this](const QJsonDocument& doc) {
          if (!doc.isArray()) {
              spdlog::error(
                "OnlineRankingModel clear types response is not a JSON array");
              return;
          }
          auto clearTypeCounts = QHash<QString, int>{};
          for (const auto& item : doc.array()) {
              if (!item.isObject())
                  continue;
              const auto obj = item.toObject();
              const auto clearType = obj.value("bestClearType").toString();
              if (clearType.isEmpty()) {
                  continue;
              }
              clearTypeCounts[clearType]++;
          }
          auto map = QVariantMap{};
          for (const auto& key : clearTypeCounts.keys()) {
              map[key] = clearTypeCounts[key];
          }
          setClearCounts(std::move(map));
      },
      [](const QString& err) {
          spdlog::error("OnlineRankingModel clear types fetch failed: {}",
                        err.toStdString());
      });
}

auto
OnlineRankingModel::getMd5() const -> QString
{
    return currentMd5;
}
void
OnlineRankingModel::setMd5(const QString& md5)
{
    if (currentMd5 == md5) {
        return;
    }
    currentMd5 = md5;
    emit md5Changed();
    beginResetModel();
    entries.clear();
    endResetModel();
    fetch();
}

void
OnlineRankingModel::resetMd5()
{
    setMd5(QString());
}

auto
OnlineRankingModel::isLoading() const -> bool
{
    return currentlyLoading;
}
void
OnlineRankingModel::setLoading(bool loading)
{
    if (currentlyLoading == loading)
        return;
    currentlyLoading = loading;
    emit loadingChanged();
}
void
OnlineRankingModel::setPlayerCount(int count)
{
    if (playerCount == count)
        return;
    playerCount = count;
    emit playerCountChanged();
}
void
OnlineRankingModel::setScoreCount(int count)
{
    if (scoreCount == count)
        return;
    scoreCount = count;
    emit scoreCountChanged();
}
void
OnlineRankingModel::setClearCounts(QVariantMap counts)
{
    if (clearCounts == counts)
        return;
    clearCounts = std::move(counts);
    emit clearCountsChanged();
}

auto
OnlineRankingModel::getLimit() const -> int
{
    return currentLimit;
}
void
OnlineRankingModel::setLimit(int limit)
{
    if (currentLimit == limit)
        return;
    currentLimit = limit;
    emit limitChanged();
    fetch();
}

auto
OnlineRankingModel::getOffset() const -> int
{
    return currentOffset;
}
void
OnlineRankingModel::setOffset(int offset)
{
    if (currentOffset == offset)
        return;
    currentOffset = offset;
    emit offsetChanged();
    fetch();
}

auto
OnlineRankingModel::getSortBy() const -> SortableColumn
{
    return currentSortBy;
}
void
OnlineRankingModel::setSortBy(SortableColumn sortBy)
{
    if (currentSortBy == sortBy)
        return;
    currentSortBy = sortBy;
    emit sortByChanged();
    fetch();
}

auto
OnlineRankingModel::getSortDir() const -> SortDirection
{
    return currentSortDir;
}
void
OnlineRankingModel::setSortDir(SortDirection sortDir)
{
    if (currentSortDir == sortDir)
        return;
    currentSortDir = sortDir;
    emit sortDirChanged();
    fetch();
}

auto
OnlineRankingModel::getSearch() const -> QString
{
    return currentSearch;
}
void
OnlineRankingModel::setSearch(const QString& search)
{
    if (currentSearch == search)
        return;
    currentSearch = search;
    emit searchChanged();
    fetch();
}
auto
OnlineRankingModel::getClearCounts() const -> QVariantMap
{
    return clearCounts;
}
auto
OnlineRankingModel::getScoreCount() const -> int
{
    return scoreCount;
}
auto
OnlineRankingModel::getPlayerCount() const -> int
{
    return playerCount;
}

void
OnlineRankingModel::cancelPending()
{
    emit cancelPendingRequested();
    setLoading(false);
}

void
OnlineRankingModel::setBaseUrl(const QString& baseUrl)
{
    networkRequestFactory.setBaseUrl(baseUrl);
}

} // namespace qml_components
