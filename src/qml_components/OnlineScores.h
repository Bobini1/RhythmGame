#ifndef RHYTHMGAME_ONLINESCORES_H
#define RHYTHMGAME_ONLINESCORES_H

#include "OnlineRankingModel.h"
#include "gameplay_logic/BmsScore.h"
#include "support/PendingReply.h"

#include <QObject>
#include <QThreadPool>

#include <functional>

class QNetworkAccessManager;

namespace qml_components {

class OnlineScoresTestAccess;

class TachiResolveHandle : public QObject
{
    Q_OBJECT
  public:
    explicit TachiResolveHandle(QObject* parent = nullptr);

  signals:
    void resolved(const QString& chartID,
                  const QString& tachiGame,
                  int noteCount);
    void failed(const QString& error);
    void cancel();
};
class OnlineScores : public QObject
{
    Q_OBJECT
    QML_ELEMENT
  public:
    explicit OnlineScores(QNetworkAccessManager* manager,
                          QObject* parent = nullptr);
    ~OnlineScores() override;
    auto resolveTachiChartId(const QString& md5) const -> TachiResolveHandle*;

    Q_INVOKABLE support::PendingReply* getScoreByGuid(const QString& webApiUrl,
                                                      const QString& guid);
    /**
     *
     * @param userId  The user ID to get the ranking entry for.
     * @param md5 The MD5 hash of the chart to get the ranking entry for.
     * @param timestamp The timestamp to get the ranking entry at.
     * If not provided, the current timestamp will be used.
     * @param provider The provider to get the ranking entry for. Does not work
     * for LR2IR.
     * @return RankingEntry or null
     */
    Q_INVOKABLE support::PendingReply* getRankingEntryAtTimestamp(
      QString webApiUrl,
      qint64 userId,
      QString md5,
      qint64 timestamp = QDateTime::currentSecsSinceEpoch(),
      OnlineRankingModel::Provider provider =
        OnlineRankingModel::Provider::RhythmGame);

  private:
    friend class OnlineScoresTestAccess;

    QNetworkAccessManager* networkManager;
    QThreadPool threadPool;
    std::function<void()> parserDeliveryQueuedHook;
    bool stopping = false;
};

} // namespace qml_components

#endif // RHYTHMGAME_ONLINESCORES_H
