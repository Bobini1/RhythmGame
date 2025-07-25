//
// Created by bobini on 07.10.23.
//

#ifndef RHYTHMGAME_SCOREDB_H
#define RHYTHMGAME_SCOREDB_H

#include <functional>
#include <QIfPendingReply>
#include "db/SqliteCppDb.h"
#include "gameplay_logic/BmsScore.h"
#include "resource_managers/Tables.h"
namespace qml_components {
class ScoreQueryResult
{
    Q_GADGET
    Q_PROPERTY(qint64 unplayed MEMBER unplayed)
    Q_PROPERTY(QVariantMap scores MEMBER scores)
  public:
    qint64 unplayed{};
    QVariantMap scores;
};
class TableScoreQueryResult : public ScoreQueryResult
{
    Q_GADGET
    Q_PROPERTY(qint64 courseUnplayed MEMBER courseUnplayed)
    Q_PROPERTY(QVariantMap courseScores MEMBER courseScores)
public:
    qint64 courseUnplayed{};
    QVariantMap courseScores;
};

class ScoreDb final : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* scoreDb;
    mutable QThreadPool threadPool;
    std::stop_source stopSource;
    auto getScoresForMd5Impl(QList<QString> md5s) const -> ScoreQueryResult;

  public:
    explicit ScoreDb(db::SqliteCppDb* scoreDb);
    Q_INVOKABLE QIfPendingReply<ScoreQueryResult> getScoresForMd5(
      const QList<QString>& md5s) const;
    Q_INVOKABLE QIfPendingReply<ScoreQueryResult> getScores(
      const QString& folder) const;
    Q_INVOKABLE QIfPendingReply<TableScoreQueryResult> getScores(
      const resource_managers::Table& table) const;
    Q_INVOKABLE QIfPendingReply<ScoreQueryResult> getScores(
      const resource_managers::Level& level) const;
    Q_INVOKABLE void cancelPending();

    Q_INVOKABLE int getTotalScoreCount() const;
};
} // namespace qml_components

#endif // RHYTHMGAME_SCOREDB_H
