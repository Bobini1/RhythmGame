#include "OnlineRankingModel.h"

#include "ProfileList.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace qml_components {

OnlineRankingModel::OnlineRankingModel(QObject* parent)
  : QAbstractListModel(parent)
{
    networkRequestFactory.setBaseUrl(profileList->getMainProfile()
                                       ->getVars()
                                       ->getGeneralVars()
                                       ->getWebApiUri());
    connect(profileList, &ProfileList::mainProfileChanged, this, [this]() {
        networkRequestFactory.setBaseUrl(profileList->getMainProfile()
                                           ->getVars()
                                           ->getGeneralVars()
                                           ->getWebApiUri());
        cancelPending();
        fetch();
    });
}

// ---------------------------------------------------------------------------
// QAbstractListModel interface
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// URL builder
// ---------------------------------------------------------------------------

auto
OnlineRankingModel::buildUrl() const -> QUrl
{
    auto url = networkRequestFactory.createRequest("scores").url();
    QUrlQuery q;
    q.addQueryItem("limit", QString::number(currentLimit));
    q.addQueryItem("offset", QString::number(currentOffset));
    q.addQueryItem("chart", currentMd5);
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

// ---------------------------------------------------------------------------
// Fetch
// ---------------------------------------------------------------------------

void
OnlineRankingModel::fetch()
{
    if (currentMd5.isEmpty())
        return;

    cancelPending();
    setLoading(true);

    auto req = QNetworkRequest(buildUrl());
    auto* reply = networkManager->get(req);
    connect(
      this, &OnlineRankingModel::cancelPendingRequested, reply, [reply]() {
          if (reply->isRunning()) {
              reply->abort();
          }
      });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            setLoading(false);
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("OnlineRankingModel fetch failed: {} - {}",
                          magic_enum::enum_name(reply->error()),
                          reply->errorString().toStdString());
            setLoading(false);
            return;
        }

        const auto doc = QJsonDocument::fromJson(reply->readAll());
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
            entry.userId = obj.value("userId").toInteger();
            entry.userName = obj.value("userName").toString();
            entry.userImage = obj.value("userImage").toString();
            entry.bestPoints = obj.value("bestPoints").toDouble();
            entry.maxPoints = obj.value("maxPoints").toDouble();
            entry.bestCombo = obj.value("bestCombo").toInt();
            entry.maxHits = obj.value("maxHits").toInt();
            entry.bestClearType = obj.value("bestClearType").toString();
            entry.bestComboBreaks = obj.value("bestComboBreaks").toInt();
            entry.latestDate =
              static_cast<int64_t>(obj.value("latestDate").toDouble());
            entry.scoreCount = obj.value("scoreCount").toInt();
            newEntries.append(std::move(entry));
        }

        beginResetModel();
        entries = std::move(newEntries);
        endResetModel();
        setLoading(false);
    });
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Cancellation / base URL
// ---------------------------------------------------------------------------

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
