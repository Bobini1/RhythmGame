#include "OnlineScores.h"

#include "gameplay_logic/BmsScore.h"
#include "support/ConvertTachiClearType.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPointer>
#include <QQmlEngine>
#include <QThread>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace gameplay_logic {
class BmsScore;
}
namespace qml_components {

namespace {

class BmsScoreDelivery
{
  public:
    explicit BmsScoreDelivery(std::unique_ptr<gameplay_logic::BmsScore> score)
      : score(std::move(score))
    {
        this->score->moveToThread(nullptr);
        if (this->score->thread() != nullptr) {
            throw std::runtime_error(
              "Failed to detach parsed score from its worker thread");
        }
    }

    [[nodiscard]] auto moveToThread(QThread* thread) -> bool
    {
        score->moveToThread(thread);
        return score->thread() == thread;
    }

    [[nodiscard]] auto get() const -> gameplay_logic::BmsScore*
    {
        return score.get();
    }

    void release() { (void)score.release(); }

  private:
    std::unique_ptr<gameplay_logic::BmsScore> score;
};

} // namespace

TachiResolveHandle::TachiResolveHandle(QObject* parent)
  : QObject(parent)
{
}
OnlineScores::OnlineScores(QNetworkAccessManager* manager, QObject* parent)
  : QObject(parent)
  , networkManager(manager)
{
}
OnlineScores::~OnlineScores()
{
    stopping = true;
    const auto replyChildren =
      findChildren<support::PendingReply*>(Qt::FindDirectChildrenOnly);
    auto replies = QList<QPointer<support::PendingReply>>{};
    replies.reserve(replyChildren.size());
    for (auto* reply : replyChildren)
        replies.append(reply);

    for (const auto& reply : replies) {
        if (!reply)
            continue;
        reply->cancel();
        if (reply)
            reply->setParent(this);
    }
    threadPool.clear();
    threadPool.waitForDone();
}
auto
OnlineScores::resolveTachiChartId(const QString& md5) const
  -> TachiResolveHandle*
{
    auto* handle = new TachiResolveHandle();

    struct TachiGame
    {
        const char* game;
    };
    static constexpr std::array<TachiGame, 2> games = { {
      { "bms-7k" },
      { "bms-14k" },
    } };
    auto attemptIndex = std::make_shared<std::size_t>(0);
    auto tryNext = std::make_shared<std::function<void()>>();

    *tryNext = [this, handle, md5, attemptIndex, tryNext]() {
        const auto idx = *attemptIndex;
        if (idx >= games.size()) {
            emit handle->failed(
              "Chart not found on Tachi for any supported BMS game");
            return;
        }

        const QUrl resolveUrl(
          QString("https://boku.tachi.ac/api/v1/games/%1/charts/resolve")
            .arg(games[idx].game));

        QNetworkRequest req(resolveUrl);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject body;
        body.insert("matchType", "bmsChartHash");
        body.insert("identifier", md5.toLower());

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
                chartID, QString(games[*attemptIndex].game), noteCount);
          });
    };

    (*tryNext)();
    return handle;
}

auto
OnlineScores::getScoreByGuid(const QString& webApiUrl, const QString& guid)
  -> support::PendingReply*
{
    auto source =
      support::PendingReplySource<gameplay_logic::BmsScore*>{ this };
    auto* operation = source.reply();
    if (stopping) {
        (void)source.fail();
        return operation;
    }

    auto baseUrl = QUrl(webApiUrl);
    auto endpoint = baseUrl.resolved(QUrl(QString("scores/%1").arg(guid)));
    auto req = QNetworkRequest(endpoint);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* networkReply = networkManager->get(req);
    source.setCancellationHandler(
      [reply = QPointer<QNetworkReply>(networkReply)] {
          if (reply)
              reply->abort();
      });
    connect(
      networkReply,
      &QNetworkReply::finished,
      this,
      [this, networkReply, source]() mutable {
          if (networkReply->error() == QNetworkReply::OperationCanceledError &&
              source.stopToken().stop_requested()) {
              networkReply->deleteLater();
              return;
          }
          if (networkReply->error() != QNetworkReply::NoError) {
              spdlog::error("getScoreByGuid failed: {} - {}",
                            magic_enum::enum_name(networkReply->error()),
                            networkReply->errorString().toStdString());
              (void)source.fail();
              networkReply->deleteLater();
              return;
          }
          auto data = networkReply->readAll();
          networkReply->deleteLater();
          source.setCancellationHandler({});
          auto parserDeliveryQueuedHook = this->parserDeliveryQueuedHook;

          // Parse and construct BmsScore objects on the thread pool to
          // avoid blocking the main thread.
          threadPool.start([source,
                            data = std::move(data),
                            parserDeliveryQueuedHook =
                              std::move(parserDeliveryQueuedHook)]() mutable {
              const auto stopToken = source.stopToken();
              if (stopToken.stop_requested())
                  return;

              try {
                  auto doc = QJsonDocument::fromJson(data);
                  if (!doc.isObject()) {
                      auto* application = QCoreApplication::instance();
                      (void)(application && QMetaObject::invokeMethod(
                                              application,
                                              [source] { (void)source.fail(); },
                                              Qt::QueuedConnection));
                      return;
                  }
                  if (stopToken.stop_requested())
                      return;

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
                  if (stopToken.stop_requested())
                      return;

                  auto score = std::make_unique<gameplay_logic::BmsScore>(
                    std::move(res),
                    std::move(replayData),
                    std::move(gaugeHistory));
                  score->setSubmissionState(
                    gameplay_logic::BmsScore::SubmissionState::Submitted);
                  auto delivery =
                    std::make_shared<BmsScoreDelivery>(std::move(score));
                  auto* application = QCoreApplication::instance();
                  const auto queued =
                    application &&
                    QMetaObject::invokeMethod(
                      application,
                      [source, delivery]() mutable {
                          if (source.stopToken().stop_requested())
                              return;

                          auto* currentApplication =
                            QCoreApplication::instance();
                          if (!currentApplication ||
                              !delivery->moveToThread(
                                currentApplication->thread())) {
                              if (source.fail()) {
                                  spdlog::error(
                                    "Error parsing getScoreByGuid response: "
                                    "failed to transfer parsed score");
                              }
                              return;
                          }

                          auto* score = delivery->get();
                          QQmlEngine::setObjectOwnership(
                            score, QQmlEngine::JavaScriptOwnership);
                          if (source.succeed(score))
                              delivery->release();
                      },
                      Qt::QueuedConnection);
                  if (queued && parserDeliveryQueuedHook)
                      parserDeliveryQueuedHook();
                  if (!queued && !stopToken.stop_requested()) {
                      spdlog::error("Error parsing getScoreByGuid response: "
                                    "failed to queue parsed score");
                  }
              } catch (const std::exception& e) {
                  auto error = std::string(e.what());
                  auto* application = QCoreApplication::instance();
                  const auto queued =
                    application &&
                    QMetaObject::invokeMethod(
                      application,
                      [source, error = std::move(error)] {
                          if (source.fail()) {
                              spdlog::error(
                                "Error parsing getScoreByGuid response: {}",
                                error);
                          }
                      },
                      Qt::QueuedConnection);
                  if (!queued && !stopToken.stop_requested()) {
                      spdlog::error("Error parsing getScoreByGuid response: "
                                    "failed to queue "
                                    "parse failure");
                  }
              }
          });
      });
    return operation;
}
auto
OnlineScores::getRankingEntryAtTimestamp(QString webApiUrl,
                                         qint64 userId,
                                         QString md5,
                                         qint64 timestamp,
                                         OnlineRankingModel::Provider provider)
  -> support::PendingReply*
{
    auto source = support::PendingReplySource<QVariant>{ this };
    auto* operation = source.reply();

    if (stopping || md5.isEmpty()) {
        (void)source.fail();
        return operation;
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
            QNetworkReply* networkReply = networkManager->get(request);
            source.setCancellationHandler(
              [reply = QPointer<QNetworkReply>(networkReply)] {
                  if (reply)
                      reply->abort();
              });

            connect(
              networkReply,
              &QNetworkReply::finished,
              this,
              [source, networkReply, userId]() mutable {
                  networkReply->deleteLater();

                  if (networkReply->error() ==
                        QNetworkReply::OperationCanceledError &&
                      source.stopToken().stop_requested()) {
                      return;
                  }
                  if (networkReply->error() ==
                        QNetworkReply::ContentNotFoundError ||
                      networkReply->error() ==
                        QNetworkReply::OperationCanceledError) {
                      (void)source.succeed(QVariant{});
                      return;
                  }
                  if (networkReply->error() != QNetworkReply::NoError) {
                      spdlog::debug(
                        "getRankingEntryAtTimestamp RhythmGame failed: {}",
                        networkReply->errorString().toStdString());
                      (void)source.fail();
                      return;
                  }

                  QJsonParseError parseErr;
                  const QJsonDocument doc =
                    QJsonDocument::fromJson(networkReply->readAll(), &parseErr);
                  if (parseErr.error != QJsonParseError::NoError ||
                      !doc.isArray()) {
                      (void)source.fail();
                      return;
                  }

                  for (const auto& item : doc.array()) {
                      if (!item.isObject())
                          continue;
                      const auto obj = item.toObject();

                      auto entry = rhythmGameRankingEntryFromJson(obj);
                      if (entry.userId != userId)
                          continue;

                      (void)source.succeed(
                        QVariant::fromValue(std::move(entry)));
                      return;
                  }

                  // userId not found in the result set
                  (void)source.succeed(QVariant{});
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
            handle->setParent(this);
            source.setCancellationHandler(
              [handle = QPointer<TachiResolveHandle>(handle)] {
                  if (!handle)
                      return;
                  emit handle->cancel();
                  handle->deleteLater();
              });

            connect(
              handle,
              &TachiResolveHandle::resolved,
              this,
              [this, handle, source, userId, timestamp](
                const QString& chartID,
                const QString& tachiGame,
                int noteCount) mutable {
                  if (source.stopToken().stop_requested()) {
                      handle->deleteLater();
                      return;
                  }

                  source.setCancellationHandler({});
                  handle->deleteLater();

                  const auto scoresUrlStr =
                    QString("https://boku.tachi.ac/api/v1/users/%1/games/%2/"
                            "scores/%3")
                      .arg(userId)
                      .arg(tachiGame)
                      .arg(chartID);

                  QNetworkReply* scoresReply =
                    networkManager->get(QNetworkRequest(QUrl(scoresUrlStr)));

                  connect(
                    scoresReply,
                    &QNetworkReply::finished,
                    this,
                    [source,
                     scoresReply,
                     userId,
                     noteCount,
                     timestamp]() mutable {
                        scoresReply->deleteLater();

                        if (scoresReply->error() ==
                              QNetworkReply::OperationCanceledError &&
                            source.stopToken().stop_requested()) {
                            return;
                        }
                        if (scoresReply->error() ==
                              QNetworkReply::ContentNotFoundError ||
                            scoresReply->error() ==
                              QNetworkReply::OperationCanceledError) {
                            (void)source.succeed(QVariant{});
                            return;
                        }
                        if (scoresReply->error() != QNetworkReply::NoError) {
                            spdlog::error(
                              "getRankingEntryAtTimestamp Tachi scores "
                              "failed: {}",
                              scoresReply->errorString().toStdString());
                            (void)source.fail();
                            return;
                        }

                        auto replyText = scoresReply->readAll();
                        QJsonParseError perr;
                        const auto doc =
                          QJsonDocument::fromJson(replyText, &perr);
                        if (perr.error != QJsonParseError::NoError ||
                            !doc.isObject()) {
                            (void)source.fail();
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
                            (void)source.succeed(QVariant{});
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
                                        judg["great"].toInt() == 0) {
                                        bestClearType = "MAX";
                                        break;
                                    }
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

                        (void)source.succeed(
                          QVariant::fromValue(std::move(entry)));
                    });
                  source.setCancellationHandler(
                    [reply = QPointer<QNetworkReply>(scoresReply)] {
                        if (reply)
                            reply->abort();
                    });
              });

            connect(handle,
                    &TachiResolveHandle::failed,
                    this,
                    [handle, source](const QString& err) mutable {
                        handle->deleteLater();
                        if (source.succeed(QVariant{})) {
                            spdlog::debug(
                              "getRankingEntryAtTimestamp Tachi resolve "
                              "failed: {}",
                              err.toStdString());
                        }
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
            (void)source.fail();
            break;
    }

    return operation;
}

} // namespace qml_components
