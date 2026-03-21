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

OnlineScores::OnlineScores(QNetworkAccessManager* manager, QObject* parent)
  : QObject(parent)
  , networkManager(manager)
{
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

        // Parse and construct BmsScore objects on the thread pool to avoid
        // blocking the main thread.
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

} // namespace qml_components
