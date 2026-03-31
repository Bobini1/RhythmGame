#include "OnlineScores.h"

#include "gameplay_logic/BmsScore.h"
#include "support/ConvertTachiClearType.h"

#include <QCoreApplication>
#include <QIfPendingReply>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace gameplay_logic {
class BmsScore;
}
namespace qml_components {

TachiResolveHandle::TachiResolveHandle(QObject* parent)
  : QObject(parent)
{
}
OnlineScores::OnlineScores(QNetworkAccessManager* manager, QObject* parent)
  : QObject(parent)
  , networkManager(manager)
{
}
auto
OnlineScores::resolveTachiChartId(const QString& md5) const
  -> TachiResolveHandle*
{
    auto* handle = new TachiResolveHandle();

    static constexpr std::array<const char*, 2> playtypes = { "7K", "14K" };
    auto attemptIndex = std::make_shared<int>(0);
    auto tryNext = std::make_shared<std::function<void()>>();

    *tryNext = [this, handle, md5, attemptIndex, tryNext]() {
        const int idx = *attemptIndex;
        if (idx >= static_cast<int>(playtypes.size())) {
            emit handle->failed(
              "Chart not found on Tachi for any supported playtype");
            return;
        }

        const QUrl resolveUrl(
          QString("https://boku.tachi.ac/api/v1/games/bms/%1/charts/resolve")
            .arg(playtypes[idx]));

        QNetworkRequest req(resolveUrl);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject body;
        body.insert("matchType", "bmsChartHash");
        body.insert("identifier", md5);

        QNetworkReply* reply = networkManager->post(
          req, QJsonDocument(body).toJson(QJsonDocument::Compact));

        // Cancel signal aborts the current in-flight reply.
        connect(handle, &TachiResolveHandle::cancel, reply, [reply] {
            reply->abort();
        });

        connect(
          reply,
          &QNetworkReply::finished,
          this,
          [handle = QPointer(handle), reply, attemptIndex, tryNext]() mutable {
              reply->deleteLater();

              if (reply->error() == QNetworkReply::OperationCanceledError)
                  return; // handle stays alive; caller decides cleanup

              if (reply->error() != QNetworkReply::NoError) {
                  ++(*attemptIndex);
                  (*tryNext)();
                  return;
              }

              QJsonParseError perr;
              const auto doc = QJsonDocument::fromJson(reply->readAll(), &perr);
              if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
                  if (!handle) {
                      return;
                  }
                  emit handle->failed(
                    QString("JSON parse error resolving Tachi chart: %1")
                      .arg(perr.errorString()));
                  return;
              }

              const auto chartObj =
                doc.object()["body"].toObject()["chart"].toObject();
              const auto chartID = chartObj["chartID"].toString();
              if (chartID.isEmpty()) {
                  ++(*attemptIndex);
                  (*tryNext)();
                  return;
              }

              const int noteCount =
                chartObj["data"].toObject()["notecount"].toInt();
              if (!handle) {
                  return;
              }
              emit handle->resolved(
                chartID, QString(playtypes[*attemptIndex]), noteCount);
          });
    };

    (*tryNext)();
    return handle;
}

auto
OnlineScores::getScoreByGuid(const QString& webApiUrl,
                             const QString& guid) const
  -> QIfPendingReply<gameplay_logic::BmsScore*>
{
    auto outer = QIfPendingReply<gameplay_logic::BmsScore*>{};

    auto baseUrl = QUrl(webApiUrl);
    auto endpoint = baseUrl.resolved(QUrl(QString("scores/%1").arg(guid)));
    auto req = QNetworkRequest(endpoint);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager->get(req);
    connect(reply, &QNetworkReply::finished, [this, reply, outer]() mutable {
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("getScoreByGuid failed: {} - {}",
                          magic_enum::enum_name(reply->error()),
                          reply->errorString().toStdString());
            outer.setFailed();
            reply->deleteLater();
            return;
        }
        auto data = reply->readAll();
        reply->deleteLater();

        // Parse and construct BmsScore objects on the thread pool to
        // avoid blocking the main thread.
        threadPool.start([outer, data]() mutable {
            try {
                auto doc = QJsonDocument::fromJson(data);
                auto* mainThread = QCoreApplication::instance()->thread();
                if (!doc.isObject()) {
                    QMetaObject::invokeMethod(
                      QCoreApplication::instance(),
                      [outer]() mutable { outer.setFailed(); },
                      Qt::QueuedConnection);
                }
                auto scoreObj = doc.object();
                auto res = gameplay_logic::BmsResult::fromJson(scoreObj);
                auto replayDataList =
                  gameplay_logic::BmsReplayData::fromJsonArray(
                    scoreObj["replayData"].toArray());
                auto replayData =
                  std::make_unique<gameplay_logic::BmsReplayData>(
                    std::move(replayDataList), res->getGuid());
                auto gaugeHistoryList =
                  gameplay_logic::BmsGaugeHistory::fromJsonArray(
                    scoreObj["gaugeHistory"].toArray());
                auto gaugeHistory =
                  std::make_unique<gameplay_logic::BmsGaugeHistory>(
                    std::move(gaugeHistoryList), res->getGuid());
                auto* score =
                  new gameplay_logic::BmsScore(std::move(res),
                                               std::move(replayData),
                                               std::move(gaugeHistory));
                score->setSubmissionState(
                  gameplay_logic::BmsScore::SubmissionState::Submitted);
                score->moveToThread(mainThread);
                QQmlEngine::setObjectOwnership(score,
                                               QQmlEngine::JavaScriptOwnership);
                QMetaObject::invokeMethod(
                  QCoreApplication::instance(),
                  [outer, score]() mutable { outer.setSuccess(score); },
                  Qt::QueuedConnection);
            } catch (const std::exception& e) {
                spdlog::error("Error parsing getScoresForMd5 response: {}",
                              e.what());
                QMetaObject::invokeMethod(
                  QCoreApplication::instance(),
                  [outer]() mutable { outer.setFailed(); },
                  Qt::QueuedConnection);
            }
        });
    });
    return outer;
}
QIfPendingReply<QVariant>
OnlineScores::getRankingEntryAtTimestamp(
  QString webApiUrl,
  qint64 userId,
  QString md5,
  qint64 timestamp,
  OnlineRankingModel::Provider provider) const
{
    QIfPendingReply<QVariant> pendingReply;

    if (md5.isEmpty()) {
        pendingReply.setFailed();
        return pendingReply;
    }

    switch (provider) {

        // ------------------------------------------------------------------
        // //
        //  RhythmGame //
        // ------------------------------------------------------------------
        // //
        case OnlineRankingModel::Provider::RhythmGame: {
            auto url = QUrl(webApiUrl).resolved(QUrl("score-summaries"));
            QUrlQuery q;
            q.addQueryItem("md5", md5);
            q.addQueryItem("date_lte", QString::number(timestamp));
            q.addQueryItem("user_id", QString::number(userId));
            url.setQuery(q);

            QNetworkRequest request(url);
            QNetworkReply* reply = networkManager->get(request);

            connect(
              reply,
              &QNetworkReply::finished,
              this,
              [pendingReply, reply, userId]() mutable {
                  reply->deleteLater();

                  if (reply->error() == QNetworkReply::OperationCanceledError ||
                      reply->error() == QNetworkReply::ContentNotFoundError) {
                      pendingReply.setSuccess(QVariant{});
                      return;
                  }
                  if (reply->error() != QNetworkReply::NoError) {
                      spdlog::debug(
                        "getRankingEntryAtTimestamp RhythmGame failed: {}",
                        reply->errorString().toStdString());
                      pendingReply.setFailed();
                      return;
                  }

                  QJsonParseError parseErr;
                  const QJsonDocument doc =
                    QJsonDocument::fromJson(reply->readAll(), &parseErr);
                  if (parseErr.error != QJsonParseError::NoError ||
                      !doc.isArray()) {
                      pendingReply.setFailed();
                      return;
                  }

                  for (const auto& item : doc.array()) {
                      if (!item.isObject())
                          continue;
                      const auto obj = item.toObject();
                      if (obj.value("userId").toInt() != userId)
                          continue;

                      RankingEntry entry;
                      entry.userId = obj.value("userId").toInt();
                      entry.userName = obj.value("userName").toString();
                      entry.userImage = obj.value("userImage").toString();
                      entry.bestPoints = obj.value("points").toDouble();
                      entry.maxPoints = obj.value("maxPoints").toDouble();
                      entry.bestPointsGuid =
                        obj.value("bestPointsGuid").toString();
                      entry.bestCombo = obj.value("bestCombo").toInt();
                      entry.maxHits = obj.value("maxHits").toInt();
                      entry.bestComboGuid =
                        obj.value("bestComboGuid").toString();
                      entry.bestClearType =
                        obj.value("bestClearType").toString();
                      entry.bestClearTypeGuid =
                        obj.value("bestClearTypeGuid").toString();
                      entry.bestComboBreaks =
                        obj.value("bestComboBreaks").toInt();
                      entry.bestComboBreaksGuid =
                        obj.value("bestComboBreaksGuid").toString();
                      entry.latestDate = obj.value("latestDate").toInteger();
                      entry.latestDateGuid =
                        obj.value("latestDateGuid").toString();
                      entry.scoreCount = obj.value("scoreCount").toInt();

                      pendingReply.setSuccess(
                        QVariant::fromValue(std::move(entry)));
                      return;
                  }

                  // userId not found in the result set
                  pendingReply.setSuccess(QVariant{});
              });
            break;
        }

        // ------------------------------------------------------------------
        // //
        //  Tachi //
        // ------------------------------------------------------------------
        // //
        case OnlineRankingModel::Provider::Tachi: {
            auto* handle = resolveTachiChartId(md5.toLower());

            connect(
              handle,
              &TachiResolveHandle::resolved,
              this,
              [this, handle, pendingReply, userId, timestamp](
                const QString& chartID,
                const QString& playtype,
                int noteCount) mutable {
                  handle->deleteLater();

                  const auto scoresUrlStr =
                    QString("https://boku.tachi.ac/api/v1/users/%1/games/bms/"
                            "%2/scores/%3")
                      .arg(userId)
                      .arg(playtype)
                      .arg(chartID);

                  QNetworkReply* scoresReply =
                    networkManager->get(QNetworkRequest(QUrl(scoresUrlStr)));

                  connect(
                    scoresReply,
                    &QNetworkReply::finished,
                    this,
                    [pendingReply,
                     scoresReply,
                     userId,
                     noteCount,
                     timestamp]() mutable {
                        scoresReply->deleteLater();

                        if (scoresReply->error() ==
                              QNetworkReply::OperationCanceledError ||
                            scoresReply->error() ==
                              QNetworkReply::ContentNotFoundError) {
                            pendingReply.setSuccess(QVariant{});
                            return;
                        }
                        if (scoresReply->error() != QNetworkReply::NoError) {
                            spdlog::error(
                              "getRankingEntryAtTimestamp Tachi scores "
                              "failed: {}",
                              scoresReply->errorString().toStdString());
                            pendingReply.setFailed();
                            return;
                        }

                        auto replyText = scoresReply->readAll();
                        QJsonParseError perr;
                        const auto doc =
                          QJsonDocument::fromJson(replyText, &perr);
                        if (perr.error != QJsonParseError::NoError ||
                            !doc.isObject()) {
                            pendingReply.setFailed();
                            return;
                        }

                        const QJsonArray scoresArr =
                          doc.object()["body"].toArray();

                        const qint64 timestampMs = timestamp * 1000LL;

                        struct Candidate
                        {
                            double exScore{ 0 };
                            int lamp{ -1 };
                            int bp{ std::numeric_limits<int>::max() };
                            int combo{ 0 };
                            QString scoreId;
                            qint64 timeAchieved{ 0 };
                        };

                        Candidate bestScore, bestLamp, lowestBp, bestCombo,
                          latest;
                        bool anyScore = false;
                        qint64 latestDate = 0;
                        int scoreCount = 0;

                        for (const auto& sv : scoresArr) {
                            if (!sv.isObject())
                                continue;
                            const auto s = sv.toObject();

                            const qint64 ta = s["timeAchieved"].toInteger();
                            if (ta > timestampMs)
                                continue;

                            ++scoreCount;
                            anyScore = true;
                            latestDate = std::max(latestDate, ta / 1000LL);

                            const auto scoreData = s["scoreData"].toObject();
                            const auto judgements =
                              scoreData["judgements"].toObject();
                            const auto optObj =
                              scoreData["optional"].toObject();
                            const auto enumIndexes =
                              s["enumIndexes"].toObject(); // top-level
                            const double ex = scoreData["score"].toDouble();
                            const int lamp = enumIndexes["lamp"].toInt();
                            const QString sid = s["scoreID"].toString();

                            if (ex > bestScore.exScore) {
                                bestScore.exScore = ex;
                                bestScore.scoreId = sid;
                            }

                            if (lamp > bestLamp.lamp) {
                                bestLamp.lamp = lamp;
                                bestLamp.scoreId = sid;
                            }

                            if (ta > latest.timeAchieved) {
                                latest.timeAchieved = ta;
                                latest.scoreId = sid;
                            }

                            const int bp =
                              (optObj.contains("bp") && optObj["bp"].isDouble())
                                ? optObj["bp"].toInt()
                                : judgements["bad"].toInt() +
                                    judgements["poor"].toInt();
                            if (bp < lowestBp.bp) {
                                lowestBp.bp = bp;
                                lowestBp.scoreId = sid;
                            }

                            const int combo = (optObj.contains("maxCombo") &&
                                               optObj["maxCombo"].isDouble())
                                                ? optObj["maxCombo"].toInt()
                                                : 0;
                            if (combo > bestCombo.combo) {
                                bestCombo.combo = combo;
                                bestCombo.scoreId = sid;
                            }
                        }

                        if (!anyScore) {
                            pendingReply.setSuccess(QVariant{});
                            return;
                        }

                        QString bestClearType =
                          support::convertTachiClearType(bestLamp.lamp);
                        if (bestClearType == "FC") {
                            for (const auto& sv : scoresArr) {
                                if (!sv.isObject())
                                    continue;
                                const auto s = sv.toObject();
                                if (s["scoreID"].toString() != bestLamp.scoreId)
                                    continue;
                                const auto judg = s["scoreData"]
                                                    .toObject()["judgements"]
                                                    .toObject();
                                if (!judg["good"].isNull() &&
                                    judg["good"].toInt() == 0) {
                                    bestClearType = "PERFECT";
                                    if (!judg["great"].isNull() &&
                                        judg["great"].toInt() == 0)
                                        bestClearType = "MAX";
                                    break;
                                }
                                if (bestScore.exScore == noteCount * 2) {
                                    bestClearType = "MAX";
                                    break;
                                }
                            }
                        }

                        RankingEntry entry;
                        entry.userId = static_cast<int>(userId);
                        entry.userImage =
                          QString(
                            "https://cdn-boku.tachi.ac/api/v1/users/%1/pfp")
                            .arg(userId);
                        entry.owner = "https://boku.tachi.ac/api/v1/users/" +
                                      QString::number(userId);

                        entry.bestPoints = bestScore.exScore;
                        entry.maxPoints = noteCount * 2;
                        entry.bestPointsGuid = bestScore.scoreId;

                        entry.bestClearType = bestClearType;
                        entry.bestClearTypeGuid = bestLamp.scoreId;

                        entry.bestComboBreaks = lowestBp.bp;
                        entry.bestComboBreaksGuid = lowestBp.scoreId;

                        entry.bestCombo = bestCombo.combo;
                        entry.bestComboGuid = bestCombo.scoreId;

                        entry.latestDate = latestDate;
                        entry.latestDateGuid = bestLamp.scoreId;

                        entry.maxHits = noteCount;
                        entry.scoreCount = scoreCount;

                        pendingReply.setSuccess(
                          QVariant::fromValue(std::move(entry)));
                    });
              });

            connect(handle,
                    &TachiResolveHandle::failed,
                    this,
                    [handle, pendingReply](const QString& err) mutable {
                        handle->deleteLater();
                        spdlog::debug(
                          "getRankingEntryAtTimestamp Tachi resolve failed: {}",
                          err.toStdString());
                        pendingReply.setSuccess(
                          QVariant{}); // chart simply not on Tachi
                    });
            break;
        }

        // ------------------------------------------------------------------
        // //
        //  LR2IR – not supported //
        // ------------------------------------------------------------------
        // //
        case OnlineRankingModel::Provider::LR2IR:
        default:
            spdlog::warn("getRankingEntryAtTimestamp: provider not supported");
            pendingReply.setFailed();
            break;
    }

    return pendingReply;
}

} // namespace qml_components
