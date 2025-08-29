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
    Q_PROPERTY(qint64 unplayed MEMBER unplayed CONSTANT)
    Q_PROPERTY(QVariantMap scores MEMBER scores CONSTANT)
  public:
    qint64 unplayed{};
    QVariantMap scores;
};
class TableQueryResult
{
    Q_GADGET
    Q_PROPERTY(ScoreQueryResult courseScores MEMBER courseScores CONSTANT)
    Q_PROPERTY(ScoreQueryResult scores MEMBER scores CONSTANT)
public:
    ScoreQueryResult courseScores;
    ScoreQueryResult scores;
};

/**
 * @brief Provides access to the score database of a profile.
 * @details All methods are asynchronous and return a QIfPendingReply.
 * The queries are executed in a thread pool, so multiple queries can be
 * executed in parallel. Use cancelPending() to stop all pending queries.
 */
class ScoreDb final : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* scoreDb;
    mutable QThreadPool threadPool;
    std::stop_source stopSource;
    auto getScoresForMd5Impl(QList<QString> md5s) const -> ScoreQueryResult;
    auto getScoresForCourseIdImpl(const QList<QString>& courseIds) const
      -> ScoreQueryResult;

  public:
    explicit ScoreDb(db::SqliteCppDb* scoreDb);
    Q_INVOKABLE QIfPendingReply<ScoreQueryResult> getScoresForMd5(
    const QList<QString>& md5s) const;
    Q_INVOKABLE QIfPendingReply<ScoreQueryResult> getScoresForCourseId(
      const QList<QString>& courseIds) const;
    Q_INVOKABLE QIfPendingReply<ScoreQueryResult> getScores(
      const QString& folder) const;
    Q_INVOKABLE QIfPendingReply<TableQueryResult> getScores(
      const resource_managers::Table& table) const;
    Q_INVOKABLE QIfPendingReply<ScoreQueryResult> getScores(
      const resource_managers::Level& level) const;
    /**
     * @brief Stop all pending queries and fail them.
     * @details This is useful when quickly browsing directories (faster than
     * the queries can finish) to avoid a backlog of queries.
     */
    Q_INVOKABLE void cancelPending();

    Q_INVOKABLE int getTotalScoreCount() const;
};
} // namespace qml_components

#endif // RHYTHMGAME_SCOREDB_H
