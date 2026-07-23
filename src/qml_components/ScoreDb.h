//
// Created by bobini on 07.10.23.
//

#ifndef RHYTHMGAME_SCOREDB_H
#define RHYTHMGAME_SCOREDB_H

#include <functional>
#include <QThreadPool>
#include <QVariantMap>
#include "db/SqliteCppDb.h"
#include "gameplay_logic/BmsScore.h"
#include "resource_managers/Tables.h"
#include "support/PendingReply.h"
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
class ScoreStatsResult
{
    Q_GADGET
    Q_PROPERTY(qint64 playCount MEMBER playCount CONSTANT)
    Q_PROPERTY(qint64 clearCount MEMBER clearCount CONSTANT)
    Q_PROPERTY(qint64 failCount MEMBER failCount CONSTANT)
    Q_PROPERTY(qint64 perfectCount MEMBER perfectCount CONSTANT)
    Q_PROPERTY(qint64 greatCount MEMBER greatCount CONSTANT)
    Q_PROPERTY(qint64 goodCount MEMBER goodCount CONSTANT)
    Q_PROPERTY(qint64 badCount MEMBER badCount CONSTANT)
    Q_PROPERTY(qint64 poorCount MEMBER poorCount CONSTANT)
    Q_PROPERTY(qint64 maxCombo MEMBER maxCombo CONSTANT)
  public:
    qint64 playCount{};
    qint64 clearCount{};
    qint64 failCount{};
    qint64 perfectCount{};
    qint64 greatCount{};
    qint64 goodCount{};
    qint64 badCount{};
    qint64 poorCount{};
    qint64 maxCombo{};
};

/**
 * @brief Provides asynchronous access to a profile's score database.
 * @details Queries execute in the thread pool. Each returned PendingReply owns
 * its cancellation; callers retain and cancel only the work they started.
 */
class ScoreDb final : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* scoreDb;
    QThreadPool threadPool;
    auto getScoresForMd5Impl(QList<QString> md5s) const -> ScoreQueryResult;
    auto getScoresForCourseIdImpl(const QList<QString>& courseIds) const
      -> ScoreQueryResult;
    auto getScoreSummaryForMd5Impl(const QList<QString>& md5s) const
      -> QVariantMap;
    auto getFolderScoreSummaryImpl(const QString& folder) const -> QVariantMap;

  public:
    explicit ScoreDb(db::SqliteCppDb* scoreDb);
    Q_INVOKABLE support::PendingReply* getScoresForMd5(
      const QList<QString>& md5s);
    Q_INVOKABLE support::PendingReply* getScoresForCourseId(
      const QList<QString>& courseIds);
    Q_INVOKABLE support::PendingReply* getScores(const QString& folder);
    Q_INVOKABLE support::PendingReply* getScores(
      const resource_managers::Table& table);
    Q_INVOKABLE support::PendingReply* getScores(
      const resource_managers::Level& level);
    Q_INVOKABLE support::PendingReply* getScoreSummary(
      const QString& folder);
    Q_INVOKABLE support::PendingReply* getScoreSummary(
      const resource_managers::Table& table);
    Q_INVOKABLE support::PendingReply* getScoreSummary(
      const resource_managers::Level& level);
    Q_INVOKABLE support::PendingReply* getTotalStats();

    Q_INVOKABLE int getTotalScoreCount() const;
};
} // namespace qml_components

#endif // RHYTHMGAME_SCOREDB_H
