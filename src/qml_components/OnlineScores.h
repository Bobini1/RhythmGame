#ifndef RHYTHMGAME_ONLINESCORES_H
#define RHYTHMGAME_ONLINESCORES_H

#include "OnlineRankingModel.h"
#include "gameplay_logic/BmsScore.h"

#include <QObject>
#include <QIfPendingReply>
#include <QThreadPool>

class QNetworkAccessManager;

namespace qml_components {

class TachiResolveHandle : public QObject
{
    Q_OBJECT
  public:
    explicit TachiResolveHandle(QObject* parent = nullptr);

  signals:
    void resolved(const QString& chartID,
                  const QString& playtype,
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
    auto resolveTachiChartId(const QString& md5) const -> TachiResolveHandle*;

    Q_INVOKABLE QIfPendingReply<gameplay_logic::BmsScore*> getScoreByGuid(
      const QString& webApiUrl,
      const QString& guid) const;
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
    Q_INVOKABLE QIfPendingReply<QVariant> getRankingEntryAtTimestamp(
      QString webApiUrl,
      qint64 userId,
      QString md5,
      qint64 timestamp = QDateTime::currentSecsSinceEpoch(),
      OnlineRankingModel::Provider provider =
        OnlineRankingModel::Provider::RhythmGame) const;

  private:
    QNetworkAccessManager* networkManager;
    mutable QThreadPool threadPool;
};

} // namespace qml_components

#endif // RHYTHMGAME_ONLINESCORES_H