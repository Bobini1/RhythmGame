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

class OnlineScores : public QObject
{
    Q_OBJECT
    QML_ELEMENT
  public:
    explicit OnlineScores(QNetworkAccessManager* manager,
                          QObject* parent = nullptr);
    Q_INVOKABLE
    QIfPendingReply<gameplay_logic::BmsScore*> getScoreByGuid(
      const QString& webApiUrl,
      const QString& guid) const;

  private:
    QNetworkAccessManager* networkManager;
    mutable QThreadPool threadPool;
};

} // namespace qml_components

#endif // RHYTHMGAME_ONLINESCORES_H