#include "OnlineRankingModel.h"

#include "ProfileList.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QUrl>
#include <spdlog/spdlog.h>
#include <functional>
#include <QXmlStreamReader>
#include <compare>
#include <type_traits>
#include <optional>
#include <string>
#include <string_view>
#include <iconv.h>

namespace qml_components {

static auto
convertToUtf8(const char* encoding,
              const std::string_view& str,
              const bool ignoreErrors = false) -> std::optional<std::string>
{
    const auto* targetEncoding = "UTF-8";
    if (ignoreErrors) {
        targetEncoding = "UTF-8//IGNORE";
    }

    iconv_t cd = iconv_open(targetEncoding, encoding);
    if (cd == (iconv_t)-1) {
        return std::nullopt;
    }

    auto* srcPtr = const_cast<char*>(str.data());
    auto srcLeft = str.size();

    const auto dstSize = str.size() * 4;
    std::vector<char> dstBuf(dstSize + 1);
    auto* dstPtr = dstBuf.data();
    auto dstLeft = dstSize;

    auto result = iconv(cd, &srcPtr, &srcLeft, &dstPtr, &dstLeft);
    iconv_close(cd);

    if (result == static_cast<size_t>(-1) && !ignoreErrors) {
        return std::nullopt;
    }

    *dstPtr = '\0';
    const auto result_size = dstSize - dstLeft;
    return std::string(dstBuf.data(), result_size);
}

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
      reply, &QNetworkReply::finished, this, [reply, onSuccess, onError]() {
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
}

auto
OnlineRankingModel::rowCount(const QModelIndex& parent) const -> int
{
    if (parent.isValid())
        return 0;
    return std::clamp(static_cast<int>(entries.size()) - currentOffset,
                      0,
                      currentLimit ? currentLimit
                                   : std::numeric_limits<int>::max());
}

auto
OnlineRankingModel::data(const QModelIndex& index, int role) const -> QVariant
{
    if (!index.isValid() || index.row() < 0 || index.row() >= entries.size()) {
        return {};
    }

    const auto& e = entries.at(index.row() - currentOffset);
    switch (role) {
        case UserIdRole:
            return e.userId;
        case UserNameRole:
            return e.userName;
        case UserImageRole:
            return e.userImage;
        case BestPointsRole:
            return e.bestPoints;
        case BestPointsGuidRole:
            return e.bestPointsGuid;
        case MaxPointsRole:
            return e.maxPoints;
        case BestComboRole:
            return e.bestCombo;
        case BestComboGuidRole:
            return e.bestComboGuid;
        case MaxHitsRole:
            return e.maxHits;
        case BestClearTypeRole:
            return e.bestClearType;
        case BestClearTypeGuidRole:
            return e.bestClearTypeGuid;
        case BestComboBreaksRole:
            return e.bestComboBreaks;
        case BestComboBreaksGuidRole:
            return e.bestComboBreaksGuid;
        case LatestDateRole:
            return QVariant::fromValue(e.latestDate);
        case LatestDateGuidRole:
            return e.latestDateGuid;
        case ScoreCountRole:
            return e.scoreCount;
        case OwnerRole:
            return e.owner;
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
        { BestPointsGuidRole, "bestPointsGuid" },
        { BestComboGuidRole, "bestComboGuid" },
        { BestComboBreaksGuidRole, "bestComboBreaksGuid" },
        { BestClearTypeGuidRole, "bestClearTypeGuid" },
        { LatestDateGuidRole, "latestDateGuid" },
        { BestClearTypeRole, "bestClearType" },
        { BestComboBreaksRole, "bestComboBreaks" },
        { LatestDateRole, "latestDate" },
        { ScoreCountRole, "scoreCount" },
        { OwnerRole, "owner" },
        { RankRole, "rank" },
    };
}

auto
OnlineRankingModel::buildUrl() const -> QUrl
{
    auto url = QUrl("score-summaries");
    QUrlQuery q;
    q.addQueryItem("md5", currentMd5);
    // map enum to server's sortBy string
    if (currentSortBy != SortableColumn::None) {
        QString sortByStr;
        switch (currentSortBy) {
            case SortableColumn::ScorePct:
                sortByStr = "score_pct";
                break;
            case SortableColumn::Player:
                sortByStr = "player";
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
    url.setQuery(q);
    return url;
}

void
OnlineRankingModel::fetchRhythmGame()
{
    performJsonGet(
      buildUrl().toString(),
      [this](const QJsonDocument& doc) {
          if (!doc.isArray()) {
              spdlog::error("OnlineRankingModel: response is not a JSON array");
              setLoading(false);
              return;
          }

          QList<RankingEntry> newEntries;
          auto array = doc.array();
          auto playerCount = array.size();
          auto scoreCount = 0;
          newEntries.reserve(array.size());
          auto clearTypeCounts = QHash<QString, int>{};
          for (const auto& item : array) {
              if (!item.isObject())
                  continue;
              const auto obj = item.toObject();
              RankingEntry entry;
              entry.userId = obj.value("userId").toInt();
              entry.userName = obj.value("userName").toString();
              entry.userImage = obj.value("userImage").toString();
              entry.bestPoints = obj.value("bestPoints").toDouble();
              entry.maxPoints = obj.value("maxPoints").toDouble();
              entry.bestPointsGuid = obj.value("bestPointsGuid").toString();
              entry.bestCombo = obj.value("bestCombo").toInt();
              entry.maxHits = obj.value("maxHits").toInt();
              entry.bestComboGuid = obj.value("bestComboGuid").toString();
              entry.bestClearType = obj.value("bestClearType").toString();
              clearTypeCounts[entry.bestClearType]++;
              entry.bestClearTypeGuid =
                obj.value("bestClearTypeGuid").toString();
              entry.bestComboBreaks = obj.value("bestComboBreaks").toInt();
              entry.bestComboBreaksGuid =
                obj.value("bestComboBreaksGuid").toString();
              entry.latestDate = obj.value("latestDate").toInteger();
              entry.latestDateGuid = obj.value("latestDateGuid").toString();
              entry.scoreCount = obj.value("scoreCount").toInt();
              scoreCount += entry.scoreCount;
              newEntries.append(std::move(entry));
          }
          auto map = QVariantMap{};
          for (const auto& key : clearTypeCounts.keys()) {
              map[key] = clearTypeCounts[key];
          }
          setClearCounts(std::move(map));
          setScoreCount(scoreCount);
          setPlayerCount(playerCount);

          beginResetModel();
          entries = std::move(newEntries);
          emit rankingEntriesChanged();
          endResetModel();
          setLoading(false);
      },
      [this](const QString& err) {
          spdlog::error("OnlineRankingModel fetch failed: {}",
                        err.toStdString());
          setLoading(false);
      });
}

namespace {
struct TmpEntry
{
    QString name;
    int id{ 0 };
    int clear{ 0 };
    int notes{ 0 };
    int combo{ 0 };
    int pg{ 0 };
    int gr{ 0 };
    int minbp{ 0 };
};

auto
parseLr2Scores(QXmlStreamReader& xr) -> QList<TmpEntry>
{
    auto out = QList<TmpEntry>{};
    while (!xr.atEnd()) {
        xr.readNext();
        if (xr.isStartElement() && xr.name() == QLatin1String("score")) {
            TmpEntry e;
            // parse children until </score>
            while (!xr.atEnd()) {
                xr.readNext();
                if (xr.isStartElement()) {
                    const auto nm = xr.name().toString();
                    const auto text = xr.readElementText();
                    if (nm == QLatin1String("name")) {
                        e.name = text;
                    } else if (nm == QLatin1String("id")) {
                        e.id = text.toInt();
                    } else if (nm == QLatin1String("clear")) {
                        e.clear = text.toInt();
                    } else if (nm == QLatin1String("notes")) {
                        e.notes = text.toInt();
                    } else if (nm == QLatin1String("combo")) {
                        e.combo = text.toInt();
                    } else if (nm == QLatin1String("pg")) {
                        e.pg = text.toInt();
                    } else if (nm == QLatin1String("gr")) {
                        e.gr = text.toInt();
                    } else if (nm == QLatin1String("minbp")) {
                        e.minbp = text.toInt();
                    }
                } else if (xr.isEndElement() &&
                           xr.name() == QLatin1String("score")) {
                    break;
                }
            }
            out.append(std::move(e));
        }
    }
    return out;
}
}
void
OnlineRankingModel::fetchLR2IR()
{
    const QUrl url(
      "http://www.dream-pro.info/~lavalse/LR2IR/2/getrankingxml.cgi");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    const QByteArray postData =
      QString("songmd5=%1&id=1").arg(currentMd5).toUtf8();

    QNetworkReply* reply = networkManager->post(request, postData);

    connect(this, &OnlineRankingModel::cancelPendingRequested, reply, [reply] {
        reply->abort();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::OperationCanceledError) {
            return;
        }
        if (reply->error() == QNetworkReply::ContentNotFoundError) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("OnlineRankingModel fetchLR2IR failed: {}",
                          reply->errorString().toStdString());
            setLoading(false);
            return;
        }

        const QByteArray raw = reply->readAll();

        // Some LR2IR responses start with a leading '#' or other junk before
        // the XML declaration (e.g. "#<?xml ... encoding=\"shift_jis\"?>").
        // Strip any bytes before the first '<' so the XML declaration is at
        // the start and QXmlStreamReader can detect the declared encoding.
        QByteArray xmlBytes = raw;
        const int firstLt = xmlBytes.indexOf('<');
        if (firstLt > 0) {
            xmlBytes = xmlBytes.mid(firstLt);
        }

        QXmlStreamReader xr(xmlBytes);
        auto tmpList = parseLr2Scores(xr);

        QList<RankingEntry> newEntries;
        newEntries.reserve(tmpList.size());
        QHash<QString, int> clearTypeCounts;
        int totalScoreCount = 0;

        struct WithPct
        {
            RankingEntry entry;
            double scorePct;
        };
        QVector<WithPct> withPct;

        for (const auto& t : tmpList) {
            RankingEntry r;
            r.userId = t.id;
            r.userName = t.name;
            r.userImage = QString();
            const double points = t.pg * 2 + t.gr;
            const double maxPoints = t.notes * 2;
            r.bestPoints = points;
            r.maxPoints = maxPoints;
            r.bestPointsGuid.clear();
            r.bestCombo = t.combo;
            r.maxHits = t.notes;
            r.bestComboGuid.clear();
            r.bestClearType = [&] {
                switch (t.clear) {
                    case 0:
                        return "NOPLAY";
                    case 1:
                        return "FAILED";
                    case 2:
                        return "EASY";
                    case 3:
                        return "NORMAL";
                    case 4:
                        return "HARD";
                    default: {
                        if (r.bestPoints == r.maxPoints) {
                            return "MAX";
                        }
                        if (t.gr + t.pg == r.maxHits) {
                            return "PERFECT";
                        }
                        return "FC";
                    }
                }
            }();
            clearTypeCounts[r.bestClearType]++;
            r.bestClearTypeGuid.clear();
            r.bestComboBreaks = t.minbp;
            r.bestComboBreaksGuid.clear();
            r.latestDate = 0;
            r.latestDateGuid.clear();
            r.scoreCount = 0;
            r.owner.clear();

            totalScoreCount += r.scoreCount;

            const double scorePct =
              (maxPoints > 0.0) ? (points / maxPoints) : 0.0;
            withPct.append({ std::move(r), scorePct });
        }

        QVector<WithPct> filtered;
        filtered.reserve(withPct.size());
        for (auto& wp : withPct) {
            bool ok = true;
            if (currentScorePctGte >= 0.0 && wp.scorePct < currentScorePctGte)
                ok = false;
            if (currentScorePctLte >= 0.0 && wp.scorePct > currentScorePctLte)
                ok = false;
            if (currentComboGte >= 0 && wp.entry.bestCombo < currentComboGte)
                ok = false;
            if (currentComboLte >= 0 && wp.entry.bestCombo > currentComboLte)
                ok = false;
            if (currentMissCountGte >= 0 &&
                wp.entry.bestComboBreaks < currentMissCountGte)
                ok = false;
            if (currentMissCountLte >= 0 &&
                wp.entry.bestComboBreaks > currentMissCountLte)
                ok = false;
            if (ok)
                filtered.append(std::move(wp));
        }

        // Sorting: comparator returns true if a should come before b
        // Use explicit priority ordering for clear types
        static const QStringList clearTypePriorities = {
            "NOPLAY", "FAILED", "AEASY", "EASY",    "NORMAL",
            "HARD",   "EXHARD", "FC",    "PERFECT", "MAX",
        };

        auto comparator = [this](const WithPct& a, const WithPct& b) {
            std::partial_ordering ord = std::weak_ordering::equivalent;
            switch (currentSortBy) {
                case SortableColumn::Player: {
                    ord = a.entry.userName <=> b.entry.userName;
                    if (ord == std::partial_ordering::equivalent) {
                        ord = a.scorePct <=> b.scorePct;
                    }
                } break;
                case SortableColumn::Grade: {
                    ord = a.scorePct <=> b.scorePct;
                } break;
                case SortableColumn::Combo:
                    ord = a.entry.bestCombo <=> b.entry.bestCombo;
                    if (ord == std::partial_ordering::equivalent) {
                        ord = a.scorePct <=> b.scorePct;
                    }
                    break;
                case SortableColumn::ComboBreaks:
                    ord = a.entry.bestComboBreaks <=> b.entry.bestComboBreaks;
                    if (ord == std::partial_ordering::equivalent) {
                        ord = a.scorePct <=> b.scorePct;
                    }
                    break;
                case SortableColumn::ClearType: {
                    const int ia =
                      clearTypePriorities.indexOf(a.entry.bestClearType);
                    const int ib =
                      clearTypePriorities.indexOf(b.entry.bestClearType);
                    ord = ia <=> ib;
                    if (ord == std::partial_ordering::equivalent) {
                        ord = a.scorePct <=> b.scorePct;
                    }
                    break;
                }
                case SortableColumn::ScorePct:
                case SortableColumn::Date:
                case SortableColumn::PlayCount:
                case SortableColumn::None:
                default:
                    ord = a.scorePct <=> b.scorePct;
                    if (ord == std::partial_ordering::equivalent) {
                        const int ia =
                          clearTypePriorities.indexOf(a.entry.bestClearType);
                        const int ib =
                          clearTypePriorities.indexOf(b.entry.bestClearType);
                        ord = ia <=> ib;
                    }
                    break;
            }

            if (ord == std::weak_ordering::less)
                return currentSortDir == SortDirection::Asc;
            if (ord == std::weak_ordering::greater)
                return currentSortDir == SortDirection::Desc;
            return false;
        };

        std::ranges::sort(
          filtered, [&comparator](const WithPct& a, const WithPct& b) -> bool {
              return comparator(a, b);
          });

        QList<RankingEntry> finalEntries;
        for (auto& entry : filtered) {
            finalEntries.append(std::move(entry.entry));
        }

        // clearCounts -> convert to QVariantMap
        QVariantMap map;
        for (const auto& key : clearTypeCounts.keys()) {
            map[key] = clearTypeCounts[key];
        }

        setClearCounts(std::move(map));
        setPlayerCount(withPct.size());

        beginResetModel();
        entries = std::move(finalEntries);
        emit rankingEntriesChanged();
        endResetModel();
        setLoading(false);
    });
}
void
OnlineRankingModel::fetch()
{
    setPlayerCount(0);
    setScoreCount(0);
    setClearCounts({});
    beginResetModel();
    entries.clear();
    emit rankingEntriesChanged();
    endResetModel();
    cancelPending();
    if (currentMd5.isEmpty() || !networkRequestFactory.baseUrl().isValid() &&
                                  currentProvider == Provider::RhythmGame) {
        setLoading(false);
        return;
    }
    setLoading(true);

    switch (currentProvider) {
        case Provider::RhythmGame:
            fetchRhythmGame();
        case Provider::LR2IR:
            fetchLR2IR();
    }
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
    if (currentlyLoading == loading) {
        return;
    }
    currentlyLoading = loading;
    emit loadingChanged();
}
void
OnlineRankingModel::setPlayerCount(int count)
{
    if (playerCount == count) {
        return;
    }
    playerCount = count;
    emit playerCountChanged();
}
void
OnlineRankingModel::setScoreCount(int count)
{
    if (scoreCount == count) {
        return;
    }
    scoreCount = count;
    emit scoreCountChanged();
}
void
OnlineRankingModel::setClearCounts(QVariantMap counts)
{
    if (clearCounts == counts) {
        return;
    }
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
    if (currentLimit == limit) {
        return;
    }
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
    if (currentOffset == offset) {
        return;
    }
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
    if (currentSortBy == sortBy) {
        return;
    }
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
    if (currentSortDir == sortDir) {
        return;
    }
    currentSortDir = sortDir;
    emit sortDirChanged();
    fetch();
}

auto
OnlineRankingModel::getLastPlayedGte() const -> qint64
{
    return currentLastPlayedGte;
}
void
OnlineRankingModel::setLastPlayedGte(qint64 val)
{
    if (currentLastPlayedGte == val)
        return;
    currentLastPlayedGte = val;
    emit lastPlayedGteChanged();
    fetch();
}

auto
OnlineRankingModel::getLastPlayedLte() const -> qint64
{
    return currentLastPlayedLte;
}
void
OnlineRankingModel::setLastPlayedLte(qint64 val)
{
    if (currentLastPlayedLte == val)
        return;
    currentLastPlayedLte = val;
    emit lastPlayedLteChanged();
    fetch();
}

auto
OnlineRankingModel::getDateGte() const -> qint64
{
    return currentDateGte;
}
void
OnlineRankingModel::setDateGte(qint64 val)
{
    if (currentDateGte == val)
        return;
    currentDateGte = val;
    emit dateGteChanged();
    fetch();
}

auto
OnlineRankingModel::getDateLte() const -> qint64
{
    return currentDateLte;
}
void
OnlineRankingModel::setDateLte(qint64 val)
{
    if (currentDateLte == val)
        return;
    currentDateLte = val;
    emit dateLteChanged();
    fetch();
}

auto
OnlineRankingModel::getScorePctGte() const -> double
{
    return currentScorePctGte;
}
void
OnlineRankingModel::setScorePctGte(double val)
{
    if (qFuzzyCompare(currentScorePctGte + 1.0, val + 1.0))
        return;
    currentScorePctGte = val;
    emit scorePctGteChanged();
    fetch();
}

auto
OnlineRankingModel::getScorePctLte() const -> double
{
    return currentScorePctLte;
}
void
OnlineRankingModel::setScorePctLte(double val)
{
    if (qFuzzyCompare(currentScorePctLte + 1.0, val + 1.0))
        return;
    currentScorePctLte = val;
    emit scorePctLteChanged();
    fetch();
}

auto
OnlineRankingModel::getComboGte() const -> int
{
    return currentComboGte;
}
void
OnlineRankingModel::setComboGte(int val)
{
    if (currentComboGte == val)
        return;
    currentComboGte = val;
    emit comboGteChanged();
    fetch();
}

auto
OnlineRankingModel::getComboLte() const -> int
{
    return currentComboLte;
}
void
OnlineRankingModel::setComboLte(int val)
{
    if (currentComboLte == val)
        return;
    currentComboLte = val;
    emit comboLteChanged();
    fetch();
}

auto
OnlineRankingModel::getMissCountGte() const -> int
{
    return currentMissCountGte;
}
void
OnlineRankingModel::setMissCountGte(int val)
{
    if (currentMissCountGte == val)
        return;
    currentMissCountGte = val;
    emit missCountGteChanged();
    fetch();
}

auto
OnlineRankingModel::getMissCountLte() const -> int
{
    return currentMissCountLte;
}
void
OnlineRankingModel::setMissCountLte(int val)
{
    if (currentMissCountLte == val)
        return;
    currentMissCountLte = val;
    emit missCountLteChanged();
    fetch();
}
auto
OnlineRankingModel::getProvider() const -> Provider
{
    return currentProvider;
}
void
OnlineRankingModel::setProvider(Provider provider)
{
    if (currentProvider == provider) {
        return;
    }
    currentProvider = provider;
    emit providerChanged();
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
auto
OnlineRankingModel::getRankingEntries() const -> const QList<RankingEntry>&
{
    return entries;
}

void
OnlineRankingModel::cancelPending()
{
    emit cancelPendingRequested();
    setLoading(false);
}
void
OnlineRankingModel::refresh()
{
    fetch();
}

void
OnlineRankingModel::setWebApiUrl(const QString& baseUrl)
{
    if (networkRequestFactory.baseUrl() == baseUrl) {
        return;
    }
    networkRequestFactory.setBaseUrl(baseUrl);
    fetch();
    emit webApiUrlChanged();
}
auto
OnlineRankingModel::getWebApiUrl() const -> QString
{
    return networkRequestFactory.baseUrl().toString();
}

} // namespace qml_components
