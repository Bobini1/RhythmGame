#include "OnlineRankingModel.h"

#include "OnlineScores.h"
#include "ProfileList.h"
#include "gameplay_logic/Judgement.h"
#include "support/ConvertTachiClearType.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QUrl>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <functional>
#include <memory>

namespace qml_components {
namespace {
auto
judgementCount(const QJsonArray& counts, gameplay_logic::Judgement judgement)
  -> int
{
    const auto index = static_cast<int>(judgement);
    return index >= 0 && index < counts.size() ? counts.at(index).toInt() : 0;
}

void
setJudgementCountsFromArray(RankingEntry& entry, const QJsonArray& counts)
{
    entry.bestPerfect =
      judgementCount(counts, gameplay_logic::Judgement::Perfect);
    entry.bestGreat = judgementCount(counts, gameplay_logic::Judgement::Great);
    entry.bestGood = judgementCount(counts, gameplay_logic::Judgement::Good);
    entry.bestBad = judgementCount(counts, gameplay_logic::Judgement::Bad);
    entry.bestPoor = judgementCount(counts, gameplay_logic::Judgement::Poor);
    entry.bestEmptyPoor =
      judgementCount(counts, gameplay_logic::Judgement::EmptyPoor);
}

void
setJudgementCountsFromJson(RankingEntry& entry, const QJsonObject& obj)
{
    entry.bestPerfect = obj.value(QStringLiteral("perfect")).toInt();
    if (entry.bestPerfect == 0) {
        entry.bestPerfect = obj.value(QStringLiteral("pgreat")).toInt();
    }
    if (entry.bestPerfect == 0) {
        entry.bestPerfect = obj.value(QStringLiteral("pg")).toInt();
    }

    entry.bestGreat = obj.value(QStringLiteral("great")).toInt();
    if (entry.bestGreat == 0) {
        entry.bestGreat = obj.value(QStringLiteral("gr")).toInt();
    }

    entry.bestGood = obj.value(QStringLiteral("good")).toInt();
    if (entry.bestGood == 0) {
        entry.bestGood = obj.value(QStringLiteral("gd")).toInt();
    }

    entry.bestBad = obj.value(QStringLiteral("bad")).toInt();
    if (entry.bestBad == 0) {
        entry.bestBad = obj.value(QStringLiteral("bd")).toInt();
    }

    entry.bestPoor = obj.value(QStringLiteral("poor")).toInt();
    entry.bestEmptyPoor = obj.value(QStringLiteral("emptyPoor")).toInt();
    if (entry.bestEmptyPoor == 0) {
        entry.bestEmptyPoor = obj.value(QStringLiteral("empty_poor")).toInt();
    }
    if (entry.bestEmptyPoor == 0) {
        entry.bestEmptyPoor = obj.value(QStringLiteral("miss")).toInt();
    }
    if (entry.bestEmptyPoor == 0 && entry.bestComboBreaks > 0) {
        entry.bestEmptyPoor =
          std::max(0,
                   entry.bestComboBreaks - entry.bestBad - entry.bestPoor);
    }
}
}

auto
rhythmGameRankingEntryFromJson(const QJsonObject& obj) -> RankingEntry
{
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
    entry.bestClearTypeGuid = obj.value("bestClearTypeGuid").toString();
    entry.bestComboBreaks = obj.value("bestComboBreaks").toInt();
    setJudgementCountsFromArray(
      entry, obj.value("bestPointsJudgementCounts").toArray());
    entry.bestComboBreaksGuid =
      obj.value("bestComboBreaksGuid").toString();
    entry.latestDate = obj.value("latestDate").toInteger();
    entry.latestDateGuid = obj.value("latestDateGuid").toString();
    entry.scoreCount = obj.value("scoreCount").toInt();
    if (entry.scoreCount <= 0 && !entry.bestPointsGuid.isEmpty()) {
        entry.scoreCount = 1;
    }
    return entry;
}

void
OnlineRankingModel::performJsonGet(
  const QString& url,
  const std::function<void(const QJsonDocument&)> onSuccess,
  const std::function<void(const QString&)> onError)
{
    const auto request = networkRequestFactory.createRequest(url);

    QNetworkReply* reply = networkManager->get(request);
    reply->setParent(this);

    connect(this, &OnlineRankingModel::cancelPendingRequested, reply, [reply] {
        reply->abort();
    });

    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, onSuccess, onError]() {
                reply->deleteLater();

                if (reply->error() == QNetworkReply::OperationCanceledError) {
                    return;
                }
                if (reply->error() == QNetworkReply::ContentNotFoundError) {
                    setLoading(false);
                    return;
                }
                if (reply->error() != QNetworkReply::NoError) {
                    onError(reply->errorString());
                    return;
                }
                const QByteArray data = reply->readAll();

                QJsonParseError parseErr;
                const QJsonDocument doc =
                  QJsonDocument::fromJson(data, &parseErr);
                if (parseErr.error != QJsonParseError::NoError) {
                    onError(QString::fromLatin1("JSON parse error: %1")
                              .arg(parseErr.errorString()));
                    return;
                }

                onSuccess(doc);
            });
}

void
OnlineRankingModel::handleTachiReply(int startRanking,
                                     QString tachiGame,
                                     int noteCount,
                                     QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        setLoading(false);
        return;
    }
    if (reply->error() == QNetworkReply::ContentNotFoundError) {
        setLoading(false);
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        spdlog::error("OnlineRankingModel fetchTachi pbs fetch failed: {}",
                      reply->errorString().toStdString());
        setLoading(false);
        return;
    }

    const QByteArray pbsData = reply->readAll();

    QJsonParseError pbsErr;
    const QJsonDocument pbsDoc = QJsonDocument::fromJson(pbsData, &pbsErr);
    if (pbsErr.error != QJsonParseError::NoError || !pbsDoc.isObject()) {
        spdlog::error("OnlineRankingModel fetchTachi: pbs JSON parse error: {}",
                      pbsErr.errorString().toStdString());
        setLoading(false);
        return;
    }

    const QJsonObject pbsRoot = pbsDoc.object();
    const QJsonObject pbsBody = pbsRoot.value("body").toObject();
    const QJsonArray pbsArr = pbsBody.value("pbs").toArray();
    const QJsonArray usersArr = pbsBody.value("users").toArray();

    if (pbsArr.isEmpty()) {
        if (currentSortBy != SortableColumn::None &&
            currentSortDir != SortDirection::None &&
            !(currentSortBy == SortableColumn::ScorePct &&
              currentSortDir == SortDirection::Desc) &&
            (currentComboLte > 0 || currentComboGte > 0 ||
             currentMissCountGte > 0 || currentMissCountLte > 0 ||
             currentScorePctGte > 0.0 || currentScorePctLte > 0.0)) {
            setEntries(sortFilterLocal(std::move(entries)));
        }
        setLoading(false);
        return;
    }
    if (entries.size() + pbsArr.size() != playerCount) {
        const int pageSize = 100;
        startRanking += pageSize;
        const auto pbsUrlStr =
          QString("https://boku.tachi.ac/api/v1/games/%1/charts/%2/"
                  "pbs?startRanking=%3")
            .arg(tachiGame)
            .arg(chartId)
            .arg(startRanking);
        auto pbsReq = QNetworkRequest(QUrl(pbsUrlStr));
        QNetworkReply* pbsReply = networkManager->get(pbsReq);
        reply->setParent(this);
        connect(this,
                &OnlineRankingModel::cancelPendingRequested,
                pbsReply,
                [pbsReply] { pbsReply->abort(); });
        connect(pbsReply,
                &QNetworkReply::finished,
                this,
                [this, startRanking, noteCount, pbsReply, tachiGame]() {
                    handleTachiReply(
                      startRanking, tachiGame, noteCount, pbsReply);
                });
    }

    auto usersMap = QHash<int, QJsonObject>();
    for (const auto& userv : usersArr) {
        if (!userv.isObject()) {
            continue;
        }
        const QJsonObject userObj = userv.toObject();
        const int userId = userObj.value("id").toInt();
        usersMap[userId] = userObj;
    }

    auto newPbs = QList<RankingEntry>();
    for (const auto& pbv : pbsArr) {
        if (!pbv.isObject()) {
            continue;
        }
        const QJsonObject pb = pbv.toObject();
        RankingEntry r;
        r.userId = pb.value("userID").toInt();
        const auto uobj = usersMap[r.userId];
        r.userName = uobj.value("username").toString();
        r.userImage = QString("https://cdn-boku.tachi.ac/api/v1/users/%1/pfp")
                        .arg(r.userId);

        const QJsonObject scoreData = pb.value("scoreData").toObject();
        r.bestPoints = scoreData.value("score").toDouble();
        r.maxPoints = noteCount * 2;

        auto judgements = scoreData.value("judgements").toObject();

        r.bestClearType = support::convertTachiClearType(
          scoreData["enumIndexes"].toObject()["lamp"].toInt());
        if (r.bestClearType == "FC") {
            if (!judgements["good"].isNull() &&
                judgements["good"].toInt() == 0) {
                r.bestClearType = "PERFECT";
                if (!judgements["great"].isNull() &&
                    judgements["great"].toInt() == 0) {
                    r.bestClearType = "MAX";
                }
            }
            if (r.bestPoints == r.maxPoints) {
                r.bestClearType = "MAX";
            }
        }

        const QJsonObject optionalObj = scoreData.value("optional").toObject();
        if (optionalObj.contains("maxCombo") &&
            optionalObj.value("maxCombo").isDouble()) {
            r.bestCombo = optionalObj.value("maxCombo").toInt();
        }
        r.maxHits = noteCount;

        if (optionalObj.contains("bp") && optionalObj.value("bp").isDouble()) {
            r.bestComboBreaks = optionalObj.value("bp").toInt();
        } else {
            r.bestComboBreaks =
              judgements["bad"].toInt() + judgements["poor"].toInt();
        }
        setJudgementCountsFromJson(r, judgements);

        const auto composedFrom = pb.value("composedFrom").toArray();
        for (const auto& cf : composedFrom) {
            if (!cf.isObject()) {
                continue;
            }
            const QJsonObject cfObj = cf.toObject();
            if (cfObj["name"] == "Best Lamp") {
                auto value = cfObj.value("scoreID").toString();
                r.bestClearTypeGuid = value;
                if (r.bestClearTypeGuid.isEmpty()) {
                    r.bestClearTypeGuid = value;
                }
                if (r.bestClearTypeGuid.isEmpty()) {
                    r.bestClearTypeGuid = value;
                }
            } else if (cfObj["name"] == "BEST SCORE") {
                r.bestPointsGuid = cfObj.value("scoreID").toString();
            } else if (cfObj["name"] == "LOWEST BP") {
                r.bestComboBreaksGuid = cfObj.value("scoreID").toString();
            }
        }

        // This is the time the pb was achieved,
        // not the last time the score was played.
        // But we don't have other data to use.
        if (pb.contains("timeAchieved") &&
            pb.value("timeAchieved").isDouble()) {
            const auto ms = pb.value("timeAchieved").toInteger();
            r.latestDate = ms / 1000;
        }

        r.scoreCount = 1;

        r.owner =
          "https://boku.tachi.ac/api/v1/users/" + QString::number(r.userId);
        newPbs.append(r);
    }

    if (newPbs.size() + entries.size() == playerCount) {
        auto oldEntries = entries;
        oldEntries.append(newPbs);
        if (currentSortBy != SortableColumn::None &&
            currentSortDir != SortDirection::None &&
            !(currentSortBy == SortableColumn::ScorePct &&
              currentSortDir == SortDirection::Desc) &&
            (currentComboLte > 0 || currentComboGte > 0 ||
             currentMissCountGte > 0 || currentMissCountLte > 0 ||
             currentScorePctGte > 0.0 || currentScorePctLte > 0.0)) {
            setEntries(sortFilterLocal(std::move(oldEntries)));
        } else {
            setEntries(std::move(oldEntries));
        }
    } else {
        appendEntries(newPbs);
    }
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

    const auto offset = std::max(0, currentOffset);
    const auto available =
      std::max(0, static_cast<int>(entries.size()) - offset);
    return std::min(available,
                    currentLimit > 0 ? currentLimit
                                     : std::numeric_limits<int>::max());
}

auto
OnlineRankingModel::data(const QModelIndex& index, int role) const -> QVariant
{
    const auto entryIndex = index.row() + std::max(0, currentOffset);
    if (!index.isValid() || index.row() < 0 || entryIndex < 0 ||
        static_cast<qsizetype>(entryIndex) >= entries.size()) {
        return {};
    }

    const auto& e = entries.at(entryIndex);
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
        case BestPerfectRole:
            return e.bestPerfect;
        case BestGreatRole:
            return e.bestGreat;
        case BestGoodRole:
            return e.bestGood;
        case BestBadRole:
            return e.bestBad;
        case BestPoorRole:
            return e.bestPoor;
        case BestEmptyPoorRole:
            return e.bestEmptyPoor;
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
            return entryIndex + 1;
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
        { BestPerfectRole, "bestPerfect" },
        { BestGreatRole, "bestGreat" },
        { BestGoodRole, "bestGood" },
        { BestBadRole, "bestBad" },
        { BestPoorRole, "bestPoor" },
        { BestEmptyPoorRole, "bestEmptyPoor" },
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
              auto entry = rhythmGameRankingEntryFromJson(obj);
              clearTypeCounts[entry.bestClearType]++;
              scoreCount += entry.scoreCount;
              newEntries.append(std::move(entry));
          }
          setScoreCount(scoreCount);
          setPlayerCount(playerCount);
          setEntries(std::move(newEntries));

          setLoading(false);
      },
      [this](const QString& err) {
          spdlog::error("OnlineRankingModel fetch failed: {}",
                        err.toStdString());
          setLoading(false);
      });
}

namespace {
constexpr int lr2irPageSize = 100;
struct Lr2irFetchState
{
    QList<RankingEntry> entries;
    QVariantMap clearCounts;
    int targetPages{ 1 };
};

auto
lr2irApiUrl(const QString& md5, int page) -> QUrl
{
    auto url = QUrl(
      QStringLiteral("https://lr2ir.com/api/charts/%1").arg(md5.toLower()));
    if (page > 1) {
        auto query = QUrlQuery{};
        query.addQueryItem(QStringLiteral("page"), QString::number(page));
        url.setQuery(query);
    }
    return url;
}

auto
lr2irClearType(QString clearType, double points, double maxPoints, int greats)
  -> QString
{
    clearType = clearType.trimmed().toUpper();
    clearType.remove(QChar(0x2605));

    if (clearType.contains(QStringLiteral("FULLCOMBO"))) {
        if (points == maxPoints && maxPoints > 0) {
            return QStringLiteral("MAX");
        }
        if (greats == 0) {
            return QStringLiteral("PERFECT");
        }
        return QStringLiteral("FC");
    }
    if (clearType == QStringLiteral("CLEAR")) {
        return QStringLiteral("NORMAL");
    }
    if (clearType == QStringLiteral("FAILED")) {
        return QStringLiteral("FAILED");
    }
    if (clearType == QStringLiteral("EASY")) {
        return QStringLiteral("EASY");
    }
    if (clearType == QStringLiteral("HARD")) {
        return QStringLiteral("HARD");
    }
    if (clearType == QStringLiteral("NORMAL")) {
        return QStringLiteral("NORMAL");
    }
    return QStringLiteral("NOPLAY");
}

auto
lr2irEntryFromJson(const QJsonObject& obj) -> RankingEntry
{
    auto entry = RankingEntry{};
    entry.userId = obj.value(QStringLiteral("player_id")).toInteger();
    entry.userName = obj.value(QStringLiteral("player_name")).toString();
    entry.bestPoints = obj.value(QStringLiteral("score")).toDouble();
    entry.maxPoints = obj.value(QStringLiteral("score_max")).toDouble();
    entry.bestCombo = obj.value(QStringLiteral("combo")).toInt();
    entry.maxHits = obj.value(QStringLiteral("combo_max")).toInt();
    if (entry.maxHits <= 0 && entry.maxPoints > 0) {
        entry.maxHits = static_cast<int>(entry.maxPoints / 2);
    }
    entry.bestComboBreaks = obj.value(QStringLiteral("bad_poor")).toInt();
    setJudgementCountsFromJson(entry, obj);
    entry.bestClearType =
      lr2irClearType(obj.value(QStringLiteral("clear_type")).toString(),
                     entry.bestPoints,
                     entry.maxPoints,
                     obj.value(QStringLiteral("great")).toInt());
    entry.owner =
      QStringLiteral("https://lr2ir.com/players/%1").arg(entry.userId);
    entry.scoreCount = 1;
    return entry;
}

auto
lr2irClearCountsFromChart(const QJsonObject& chart) -> QVariantMap
{
    auto counts = QVariantMap{};
    counts.insert(QStringLiteral("FAILED"),
                  chart.value(QStringLiteral("failed_count")).toInt());
    counts.insert(QStringLiteral("EASY"),
                  chart.value(QStringLiteral("easy_count")).toInt());
    counts.insert(QStringLiteral("NORMAL"),
                  chart.value(QStringLiteral("normal_count")).toInt());
    counts.insert(QStringLiteral("HARD"),
                  chart.value(QStringLiteral("hard_count")).toInt());
    counts.insert(QStringLiteral("FC"),
                  chart.value(QStringLiteral("fc_count")).toInt());
    return counts;
}

}
auto
OnlineRankingModel::sortFilterLocal(QList<RankingEntry> entriesUnfiltered) const
  -> QList<RankingEntry>
{

    QVector<RankingEntry> filtered;
    filtered.reserve(entriesUnfiltered.size());
    for (auto& wp : entriesUnfiltered) {
        bool ok = true;
        if (currentScorePctGte >= 0.0 &&
            (wp.maxPoints > 0
               ? (wp.bestPoints / wp.maxPoints) < currentScorePctGte
               : true))
            ok = false;
        if (currentScorePctLte >= 0.0 &&
            (wp.maxPoints > 0
               ? (wp.bestPoints / wp.maxPoints) > currentScorePctLte
               : false))
            ok = false;
        if (currentComboGte >= 0 && wp.bestCombo < currentComboGte)
            ok = false;
        if (currentComboLte >= 0 && wp.bestCombo > currentComboLte)
            ok = false;
        if (currentMissCountGte >= 0 &&
            wp.bestComboBreaks < currentMissCountGte)
            ok = false;
        if (currentMissCountLte >= 0 &&
            wp.bestComboBreaks > currentMissCountLte)
            ok = false;
        if (ok) {
            filtered.append(std::move(wp));
        }
    }

    // Sorting: comparator returns true if a should come before b
    // Use explicit priority ordering for clear types
    static const QStringList clearTypePriorities = {
        "NOPLAY", "FAILED", "AEASY", "EASY",    "NORMAL",
        "HARD",   "EXHARD", "FC",    "PERFECT", "MAX",
    };

    auto comparator = [this](const RankingEntry& a, const RankingEntry& b) {
        std::partial_ordering ord = std::weak_ordering::equivalent;
        switch (currentSortBy) {
            case SortableColumn::Player: {
                ord = a.userName <=> b.userName;
                if (ord == std::partial_ordering::equivalent) {
                    ord = (a.maxPoints > 0) ? (a.bestPoints / a.maxPoints) <=>
                                                (b.bestPoints / b.maxPoints)
                                            : std::partial_ordering::equivalent;
                }
            } break;
            case SortableColumn::Grade: {
                ord = (a.maxPoints > 0) ? (a.bestPoints / a.maxPoints) <=>
                                            (b.bestPoints / b.maxPoints)
                                        : std::partial_ordering::equivalent;
            } break;
            case SortableColumn::Combo:
                ord = a.bestCombo <=> b.bestCombo;
                if (ord == std::partial_ordering::equivalent) {
                    ord = (a.maxPoints > 0) ? (a.bestPoints / a.maxPoints) <=>
                                                (b.bestPoints / b.maxPoints)
                                            : std::partial_ordering::equivalent;
                }
                break;
            case SortableColumn::ComboBreaks:
                ord = a.bestComboBreaks <=> b.bestComboBreaks;
                if (ord == std::partial_ordering::equivalent) {
                    ord = (a.maxPoints > 0) ? (a.bestPoints / a.maxPoints) <=>
                                                (b.bestPoints / b.maxPoints)
                                            : std::partial_ordering::equivalent;
                }
                break;
            case SortableColumn::ClearType: {
                const int ia = clearTypePriorities.indexOf(a.bestClearType);
                const int ib = clearTypePriorities.indexOf(b.bestClearType);
                ord = ia <=> ib;
                if (ord == std::partial_ordering::equivalent) {
                    ord = (a.maxPoints > 0) ? (a.bestPoints / a.maxPoints) <=>
                                                (b.bestPoints / b.maxPoints)
                                            : std::partial_ordering::equivalent;
                }
                break;
            }
            case SortableColumn::ScorePct:
            case SortableColumn::Date:
            case SortableColumn::PlayCount:
            case SortableColumn::None:
            default:
                ord = (a.maxPoints > 0) ? (a.bestPoints / a.maxPoints) <=>
                                            (b.bestPoints / b.maxPoints)
                                        : std::partial_ordering::equivalent;
                if (ord == std::partial_ordering::equivalent) {
                    const int ia = clearTypePriorities.indexOf(a.bestClearType);
                    const int ib = clearTypePriorities.indexOf(b.bestClearType);
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
      filtered,
      [&comparator](const RankingEntry& a, const RankingEntry& b) -> bool {
          return comparator(a, b);
      });
    return filtered;
}
void
OnlineRankingModel::fetchLR2IR()
{
    const auto md5 = currentMd5;
    const auto fetchGeneration = currentFetchGeneration;
    const auto usesLocalSortOrFilter =
      currentSortBy != SortableColumn::ScorePct ||
      currentSortDir != SortDirection::Desc || currentComboLte >= 0 ||
      currentComboGte >= 0 || currentMissCountGte >= 0 ||
      currentMissCountLte >= 0 || currentScorePctGte >= 0.0 ||
      currentScorePctLte >= 0.0;
    const auto state = std::make_shared<Lr2irFetchState>();
    const auto requestPage = std::make_shared<std::function<void(int)>>();

    *requestPage = [this,
                    md5,
                    fetchGeneration,
                    usesLocalSortOrFilter,
                    state,
                    requestPage](int page) {
        if (fetchGeneration != currentFetchGeneration) {
            return;
        }
        auto request = QNetworkRequest(lr2irApiUrl(md5, page));
        QNetworkReply* reply = networkManager->get(request);
        reply->setParent(this);

        connect(this,
                &OnlineRankingModel::cancelPendingRequested,
                reply,
                [reply] { reply->abort(); });

        connect(
          reply,
          &QNetworkReply::finished,
          this,
          [this,
           reply,
           page,
           fetchGeneration,
           usesLocalSortOrFilter,
           state,
           requestPage] {
              reply->deleteLater();

              if (fetchGeneration != currentFetchGeneration) {
                  return;
              }
              if (reply->error() == QNetworkReply::OperationCanceledError) {
                  return;
              }
              if (reply->error() == QNetworkReply::ContentNotFoundError) {
                  setLoading(false);
                  return;
              }
              if (reply->error() != QNetworkReply::NoError) {
                  spdlog::debug("OnlineRankingModel fetchLR2IR failed: {}",
                                reply->errorString().toStdString());
                  setLoading(false);
                  return;
              }

              QJsonParseError parseError;
              const auto doc =
                QJsonDocument::fromJson(reply->readAll(), &parseError);
              if (parseError.error != QJsonParseError::NoError ||
                  !doc.isObject()) {
                  spdlog::debug(
                    "OnlineRankingModel fetchLR2IR JSON parse failed: {}",
                    parseError.errorString().toStdString());
                  setLoading(false);
                  return;
              }

              const auto root = doc.object();
              const auto chart = root.value(QStringLiteral("chart")).toObject();
              const auto leaderboard =
                root.value(QStringLiteral("leaderboard")).toArray();

              if (page == 1) {
                  const auto totalPages =
                    root.value(QStringLiteral("total_pages")).toInt(1);
                  const auto totalRows =
                    root.value(QStringLiteral("total_rows"))
                      .toInt(static_cast<int>(leaderboard.size()));
                  setPlayerCount(chart.value(QStringLiteral("play_people"))
                                   .toInt(totalRows));
                  setScoreCount(
                    chart.value(QStringLiteral("play_count")).toInt(totalRows));
                  state->clearCounts = lr2irClearCountsFromChart(chart);
                  state->targetPages = std::max(1, totalPages);
                  state->entries.reserve(totalRows);
              }

              for (const auto& value : leaderboard) {
                  if (value.isObject()) {
                      state->entries.append(
                        lr2irEntryFromJson(value.toObject()));
                  }
              }

              if (page < state->targetPages) {
                  (*requestPage)(page + 1);
                  return;
              }

              auto loadedEntries = state->entries;
              if (usesLocalSortOrFilter) {
                  loadedEntries = sortFilterLocal(std::move(loadedEntries));
              }
              setEntries(std::move(loadedEntries));
              setClearCounts(state->clearCounts);
              setLoading(false);
          });
    };

    (*requestPage)(1);
}
void
OnlineRankingModel::fetchTachi()
{
    auto* handle = onlineScores->resolveTachiChartId(currentMd5.toLower());
    handle->setParent(this);

    connect(handle,
            &TachiResolveHandle::resolved,
            this,
            [this, handle](
              const QString& chartID, const QString& tachiGame, int noteCount) {
                handle->deleteLater();
                setChartId(chartID);

                const auto pbsUrlStr =
                  QString("https://boku.tachi.ac/api/v1/games/%1/charts/%2/"
                          "pbs?startRanking=1")
                    .arg(tachiGame)
                    .arg(chartID);

                QNetworkReply* pbsReply =
                  networkManager->get(QNetworkRequest(QUrl(pbsUrlStr)));
                pbsReply->setParent(this);
                connect(this,
                        &OnlineRankingModel::cancelPendingRequested,
                        pbsReply,
                        [pbsReply] { pbsReply->abort(); });
                connect(pbsReply,
                        &QNetworkReply::finished,
                        this,
                        [this, tachiGame, noteCount, pbsReply]() {
                            handleTachiReply(1, tachiGame, noteCount, pbsReply);
                        });
            });

    connect(handle,
            &TachiResolveHandle::failed,
            this,
            [this, handle](const QString& err) {
                handle->deleteLater();
                spdlog::debug(
                  "OnlineRankingModel fetchTachi resolve failed: {}",
                  err.toStdString());
                setLoading(false);
            });

    // Cancellation is now just a signal connection — no manual bookkeeping.
    connect(this,
            &OnlineRankingModel::cancelPendingRequested,
            handle,
            &TachiResolveHandle::cancel);
}
void
OnlineRankingModel::fetch()
{
    ++currentFetchGeneration;
    emit cancelPendingRequested();

    if (currentMd5.isEmpty() || (!networkRequestFactory.baseUrl().isValid() &&
                                 currentProvider == Provider::RhythmGame)) {
        setPlayerCount(0);
        setScoreCount(0);
        setEntries({});
        setClearCounts({});
        setLoading(false);
        return;
    }
    setLoading(true);
    setPlayerCount(0);
    setScoreCount(0);
    setEntries({});
    setClearCounts({});

    switch (currentProvider) {
        case Provider::RhythmGame:
            fetchRhythmGame();
            break;
        case Provider::LR2IR:
            fetchLR2IR();
            break;
        case Provider::Tachi:
            fetchTachi();
            break;
        default:
            // unknown provider
            setLoading(false);
            break;
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
    if (currentProvider != Provider::Tachi) {
        setChartId(md5);
    } else {
        setChartId("");
    }
    fetch();
    emit md5Changed();
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
void
OnlineRankingModel::setChartId(const QString& chartId)
{
    if (this->chartId == chartId) {
        return;
    }
    this->chartId = chartId;
    emit chartIdChanged();
}
void
OnlineRankingModel::setEntries(QList<RankingEntry> entries)
{
    if (this->entries == entries) {
        return;
    }
    beginResetModel();
    this->entries = std::move(entries);
    emit rankingEntriesChanged();
    endResetModel();
    auto map = QHash<QString, int>{};
    for (const auto& entry : entries) {
        map[entry.bestClearType]++;
    }
    auto variantMap = QVariantMap{};
    for (const auto& key : map.keys()) {
        variantMap[key] = map[key];
    }
    setClearCounts(std::move(variantMap));
}
void
OnlineRankingModel::appendEntries(QList<RankingEntry> entries)
{
    if (entries.isEmpty()) {
        return;
    }
    const int oldSize = this->entries.size();
    beginInsertRows(QModelIndex(), oldSize, oldSize + entries.size() - 1);
    this->entries.append(std::move(entries));
    emit rankingEntriesChanged();
    endInsertRows();
    auto clearCounts = getClearCounts();
    auto newClearCounts = QHash<QString, int>{};
    for (const auto& entry : entries) {
        newClearCounts[entry.bestClearType]++;
    }
    for (const auto& key : newClearCounts.keys()) {
        clearCounts[key] =
          clearCounts.value(key, 0).toInt() + newClearCounts[key];
    }
    setClearCounts(std::move(clearCounts));
}

auto
OnlineRankingModel::getLimit() const -> int
{
    return currentLimit;
}
void
OnlineRankingModel::setLimit(int limit)
{
    limit = std::max(0, limit);
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
    offset = std::max(0, offset);
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
    if (provider == Provider::Tachi) {
        setChartId("");
    } else {
        setChartId(currentMd5);
    }
    fetch();
    emit providerChanged();
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
    ++currentFetchGeneration;
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
auto
OnlineRankingModel::getChartId() const -> QString
{
    return chartId;
}

} // namespace qml_components
