#include "OnlineScores.h"

#include "gameplay_logic/BmsScore.h"

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

OnlineScores::OnlineScores(QNetworkAccessManager* manager,
                           const QString& baseUrl,
                           QObject* parent)
  : QObject(parent)
  , networkManager(manager)
{
    networkRequestFactory.setBaseUrl(baseUrl);
}

auto
OnlineScores::getScoresForMd5(const QString& md5) const
  -> QIfPendingReply<QList<OnlineScoreQueryResult>>
{
    auto outer = QIfPendingReply<QList<OnlineScoreQueryResult>>{};

    auto req =
      networkRequestFactory.createRequest(QString("user/scores/%1").arg(md5));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager->get(req);
    connect(reply, &QNetworkReply::finished, [this, reply, outer]() mutable {
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("getScoresForMd5 failed: {} - {}",
                          magic_enum::enum_name(reply->error()),
                          reply->errorString().toStdString());
            outer.setFailed();
            reply->deleteLater();
            return;
        }
        auto data = reply->readAll();
        reply->deleteLater();

        // Parse and construct BmsScore objects on the thread pool to avoid
        // blocking the main thread.
        threadPool.start([outer, data]() mutable {
            try {
                auto doc = QJsonDocument::fromJson(data);
                if (!doc.isArray()) {
                    spdlog::error("getScoresForMd5 returned non-array");
                    QMetaObject::invokeMethod(
                      QCoreApplication::instance(),
                      [outer]() mutable { outer.setFailed(); },
                      Qt::QueuedConnection);
                    return;
                }
                auto* mainThread = QCoreApplication::instance()->thread();
                auto array = doc.array();
                auto list = QList<OnlineScoreQueryResult>{};
                for (const auto& item : array) {
                    if (!item.isObject()) {
                        spdlog::error(
                          "getScoresForMd5 returned array with non-object");
                        continue;
                    }
                    auto obj = item.toObject();
                    OnlineScoreQueryResult result;
                    auto profileObj = obj["profile"].toObject();
                    result.profileInfo.username =
                      profileObj["username"].toString();
                    result.profileInfo.avatarUrl =
                      profileObj["avatarUrl"].toString();
                    result.profileInfo.profileUrl =
                      profileObj["profileUrl"].toString();
                    auto scoresArr = obj["scores"].toArray();
                    for (const auto& scoreVal : scoresArr) {
                        if (!scoreVal.isObject()) {
                            spdlog::error("getScoresForMd5 returned scores "
                                          "array with non-object");
                            continue;
                        }
                        auto scoreObj = scoreVal.toObject();
                        auto scoreDataObj = scoreObj["scoreData"].toObject();
                        auto res =
                          gameplay_logic::BmsResult::fromJson(scoreDataObj);
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
                        score->moveToThread(mainThread);
                        QQmlEngine::setObjectOwnership(
                          score, QQmlEngine::JavaScriptOwnership);
                        result.scores.append(score);
                    }
                    list.append(result);
                }
                QMetaObject::invokeMethod(
                  QCoreApplication::instance(),
                  [outer, list = std::move(list)]() mutable {
                      outer.setSuccess(list);
                  },
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
void
OnlineScores::setBaseUrl(const QString& baseUrl)
{
    networkRequestFactory.setBaseUrl(baseUrl);
}

} // namespace qml_components
