#ifndef RHYTHMGAME_ONLINESCORES_H
#define RHYTHMGAME_ONLINESCORES_H

#include "OnlineProfileInfo.h"
#include "gameplay_logic/BmsScore.h"

#include <QObject>
#include <QIfPendingReply>
#include <QNetworkRequestFactory>
#include <QThreadPool>

class QNetworkAccessManager;

namespace qml_components {

class OnlineScoreQueryResult
{
    Q_GADGET
    Q_PROPERTY(QList<gameplay_logic::BmsScore*> scores MEMBER scores CONSTANT)
    Q_PROPERTY(OnlineProfileInfo profileInfo MEMBER profileInfo CONSTANT)
  public:
    QList<gameplay_logic::BmsScore*> scores;
    OnlineProfileInfo profileInfo;
};

class OnlineScores : public QObject
{
    Q_OBJECT
    QML_ELEMENT
  public:
    explicit OnlineScores(QNetworkAccessManager* manager,
                          const QString& baseUrl,
                          QObject* parent = nullptr);

    Q_INVOKABLE QIfPendingReply<QList<OnlineScoreQueryResult>> getScoresForMd5(
      const QString& md5) const;
    void setBaseUrl(const QString& baseUrl);

  private:
    QNetworkAccessManager* networkManager;
    mutable QNetworkRequestFactory networkRequestFactory;
    mutable QThreadPool threadPool;
};

} // namespace qml_components

#endif // RHYTHMGAME_ONLINESCORES_H